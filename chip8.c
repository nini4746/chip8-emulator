/*
 * chip8.c - CHIP-8 CPU core. All 35 original opcodes, VIP semantics.
 * No SDL here by design.
 */
#include "chip8.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Standard CHIP-8 fontset: 16 characters (0-F), 5 bytes each. */
static const uint8_t fontset[CHIP8_FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
    0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
    0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
    0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
    0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
    0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
    0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
    0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
    0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
    0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
    0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
    0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
    0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
    0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
    0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
    0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
};

/* Keep every memory index inside the 4KB space so a rogue I/PC can never
 * escape the process (negative acceptance criterion: mask, don't trample). */
static inline uint16_t mem_addr(uint16_t addr) {
    return (uint16_t)(addr & 0x0FFF);
}

void chip8_init(chip8_t *c) {
    memset(c, 0, sizeof(*c));
    memcpy(&c->memory[CHIP8_FONTSET_ADDR], fontset, CHIP8_FONTSET_SIZE);
    c->pc = CHIP8_PROGRAM_START;
    c->fx0a_key = -1;
    c->status = CHIP8_RUN;
    srand((unsigned)time(NULL));
}

int chip8_load_rom(chip8_t *c, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "error: cannot open ROM '%s'\n", path);
        return 2;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: cannot seek ROM '%s'\n", path);
        fclose(f);
        return 2;
    }
    long size = ftell(f);
    if (size < 0) {
        fprintf(stderr, "error: cannot size ROM '%s'\n", path);
        fclose(f);
        return 2;
    }
    if (size == 0) {
        fprintf(stderr, "error: ROM '%s' is empty\n", path);
        fclose(f);
        return 3;
    }
    if (size > CHIP8_MAX_ROM_SIZE) {
        fprintf(stderr,
                "error: ROM '%s' is %ld bytes, exceeds max %d bytes\n",
                path, size, CHIP8_MAX_ROM_SIZE);
        fclose(f);
        return 3;
    }
    rewind(f);
    size_t got = fread(&c->memory[CHIP8_PROGRAM_START], 1, (size_t)size, f);
    fclose(f);
    if (got != (size_t)size) {
        fprintf(stderr, "error: short read on ROM '%s'\n", path);
        return 2;
    }
    return 0;
}

void chip8_tick_timers(chip8_t *c) {
    if (c->delay_timer > 0) c->delay_timer--;
    if (c->sound_timer > 0) c->sound_timer--;
}

/* Push a return address; halt with diagnostics on overflow. */
static void stack_push(chip8_t *c, uint16_t value) {
    if (c->sp >= CHIP8_STACK_SIZE) {
        fprintf(stderr, "fatal: stack overflow (sp=%u) at PC=0x%03X\n",
                c->sp, (unsigned)(c->pc - 2));
        c->status = CHIP8_HALT_STACK_OVERFLOW;
        return;
    }
    c->stack[c->sp++] = value;
}

/* Pop a return address; halt with diagnostics on underflow. */
static uint16_t stack_pop(chip8_t *c) {
    if (c->sp == 0) {
        fprintf(stderr, "fatal: stack underflow at PC=0x%03X\n",
                (unsigned)(c->pc - 2));
        c->status = CHIP8_HALT_STACK_UNDERFLOW;
        return 0;
    }
    return c->stack[--c->sp];
}

static void unknown_opcode(chip8_t *c, uint16_t opcode) {
    fprintf(stderr, "fatal: unknown opcode 0x%04X at PC=0x%03X\n",
            opcode, (unsigned)(c->pc - 2));
    c->status = CHIP8_HALT_UNKNOWN_OPCODE;
}

/* DXYN: XOR-draw an N-byte sprite at (VX,VY). VF = pixel-off collision. */
static void op_draw(chip8_t *c, uint8_t vx, uint8_t vy, uint8_t n) {
    uint8_t x0 = c->V[vx] % CHIP8_DISPLAY_WIDTH;
    uint8_t y0 = c->V[vy] % CHIP8_DISPLAY_HEIGHT;
    c->V[0xF] = 0;

    for (uint8_t row = 0; row < n; row++) {
        int py = y0 + row;
#if QUIRK_CLIPPING
        if (py >= CHIP8_DISPLAY_HEIGHT) break;   /* clip, don't wrap */
#else
        py %= CHIP8_DISPLAY_HEIGHT;
#endif
        uint8_t sprite = c->memory[mem_addr((uint16_t)(c->I + row))];
        for (uint8_t col = 0; col < 8; col++) {
            if (!(sprite & (0x80 >> col))) continue;
            int px = x0 + col;
#if QUIRK_CLIPPING
            if (px >= CHIP8_DISPLAY_WIDTH) break; /* clip, don't wrap */
#else
            px %= CHIP8_DISPLAY_WIDTH;
#endif
            int idx = py * CHIP8_DISPLAY_WIDTH + px;
            if (c->display[idx]) c->V[0xF] = 1;
            c->display[idx] ^= 1;
        }
    }
    c->drew = true;
}

void chip8_cycle(chip8_t *c) {
    /* Fetch (opcodes are 2 bytes, big-endian). */
    uint16_t opcode = (uint16_t)(c->memory[mem_addr(c->pc)] << 8
                                 | c->memory[mem_addr(c->pc + 1)]);
    c->pc = mem_addr(c->pc + 2);

    /* Common operand slices. */
    uint8_t  x   = (opcode & 0x0F00) >> 8;
    uint8_t  y   = (opcode & 0x00F0) >> 4;
    uint8_t  n   = opcode & 0x000F;
    uint8_t  nn  = opcode & 0x00FF;
    uint16_t nnn = opcode & 0x0FFF;

    switch (opcode & 0xF000) {
    case 0x0000:
        switch (opcode) {
        case 0x00E0: /* CLS */
            memset(c->display, 0, sizeof(c->display));
            break;
        case 0x00EE: /* RET */
            c->pc = stack_pop(c);
            break;
        default:
            unknown_opcode(c, opcode); /* 0NNN (SYS) and SUPER-CHIP unhandled */
            break;
        }
        break;

    case 0x1000: /* 1NNN: JP addr */
        c->pc = nnn;
        break;

    case 0x2000: /* 2NNN: CALL addr */
        stack_push(c, c->pc);
        if (c->status == CHIP8_RUN) c->pc = nnn;
        break;

    case 0x3000: /* 3XNN: SE Vx, byte */
        if (c->V[x] == nn) c->pc = mem_addr(c->pc + 2);
        break;

    case 0x4000: /* 4XNN: SNE Vx, byte */
        if (c->V[x] != nn) c->pc = mem_addr(c->pc + 2);
        break;

    case 0x5000: /* 5XY0: SE Vx, Vy */
        if (n != 0) { unknown_opcode(c, opcode); break; }
        if (c->V[x] == c->V[y]) c->pc = mem_addr(c->pc + 2);
        break;

    case 0x6000: /* 6XNN: LD Vx, byte */
        c->V[x] = nn;
        break;

    case 0x7000: /* 7XNN: ADD Vx, byte (no carry) */
        c->V[x] = (uint8_t)(c->V[x] + nn);
        break;

    case 0x8000:
        switch (n) {
        case 0x0: /* LD Vx, Vy */
            c->V[x] = c->V[y];
            break;
        case 0x1: /* OR */
            c->V[x] |= c->V[y];
#if QUIRK_VF_RESET
            c->V[0xF] = 0;
#endif
            break;
        case 0x2: /* AND */
            c->V[x] &= c->V[y];
#if QUIRK_VF_RESET
            c->V[0xF] = 0;
#endif
            break;
        case 0x3: /* XOR */
            c->V[x] ^= c->V[y];
#if QUIRK_VF_RESET
            c->V[0xF] = 0;
#endif
            break;
        case 0x4: { /* ADD Vx, Vy (carry) */
            uint16_t sum = (uint16_t)c->V[x] + c->V[y];
            c->V[x] = (uint8_t)sum;      /* result first ... */
            c->V[0xF] = sum > 0xFF;      /* ... then flag (flag wins) */
            break;
        }
        case 0x5: { /* SUB Vx, Vy (borrow) */
            uint8_t flag = c->V[x] >= c->V[y];
            c->V[x] = (uint8_t)(c->V[x] - c->V[y]);
            c->V[0xF] = flag;
            break;
        }
        case 0x6: { /* SHR: VIP shifts VY into VX */
#if QUIRK_SHIFT_VY
            uint8_t value = c->V[y];
#else
            uint8_t value = c->V[x];
#endif
            uint8_t flag = value & 0x1;
            c->V[x] = value >> 1;
            c->V[0xF] = flag;
            break;
        }
        case 0x7: { /* SUBN: Vy - Vx (borrow) */
            uint8_t flag = c->V[y] >= c->V[x];
            c->V[x] = (uint8_t)(c->V[y] - c->V[x]);
            c->V[0xF] = flag;
            break;
        }
        case 0xE: { /* SHL: VIP shifts VY into VX */
#if QUIRK_SHIFT_VY
            uint8_t value = c->V[y];
#else
            uint8_t value = c->V[x];
#endif
            uint8_t flag = (value & 0x80) >> 7;
            c->V[x] = (uint8_t)(value << 1);
            c->V[0xF] = flag;
            break;
        }
        default:
            unknown_opcode(c, opcode);
            break;
        }
        break;

    case 0x9000: /* 9XY0: SNE Vx, Vy */
        if (n != 0) { unknown_opcode(c, opcode); break; }
        if (c->V[x] != c->V[y]) c->pc = mem_addr(c->pc + 2);
        break;

    case 0xA000: /* ANNN: LD I, addr */
        c->I = nnn;
        break;

    case 0xB000: /* BNNN: JP V0, addr (VIP) */
#if QUIRK_JUMP_V0
        c->pc = mem_addr((uint16_t)(nnn + c->V[0]));
#else
        c->pc = mem_addr((uint16_t)(nnn + c->V[x]));
#endif
        break;

    case 0xC000: /* CXNN: RND Vx, byte */
        c->V[x] = (uint8_t)(rand() & 0xFF) & nn;
        break;

    case 0xD000: /* DXYN: draw */
        op_draw(c, x, y, n);
        break;

    case 0xE000:
        switch (nn) {
        case 0x9E: /* SKP Vx: skip if key VX is pressed */
            if (c->keypad[c->V[x] & 0x0F]) c->pc = mem_addr(c->pc + 2);
            break;
        case 0xA1: /* SKNP Vx: skip if key VX is not pressed */
            if (!c->keypad[c->V[x] & 0x0F]) c->pc = mem_addr(c->pc + 2);
            break;
        default:
            unknown_opcode(c, opcode);
            break;
        }
        break;

    case 0xF000:
        switch (nn) {
        case 0x07: /* LD Vx, DT */
            c->V[x] = c->delay_timer;
            break;
        case 0x0A: { /* LD Vx, K: wait for key, register on RELEASE (VIP) */
            if (c->fx0a_key < 0) {
                for (int k = 0; k < CHIP8_NUM_KEYS; k++) {
                    if (c->keypad[k]) { c->fx0a_key = k; break; }
                }
                c->pc = mem_addr(c->pc - 2); /* no key yet: re-run */
            } else if (c->keypad[c->fx0a_key]) {
                c->pc = mem_addr(c->pc - 2); /* still held: wait for release */
            } else {
                c->V[x] = (uint8_t)c->fx0a_key; /* released: register it */
                c->fx0a_key = -1;
            }
            break;
        }
        case 0x15: /* LD DT, Vx */
            c->delay_timer = c->V[x];
            break;
        case 0x18: /* LD ST, Vx */
            c->sound_timer = c->V[x];
            break;
        case 0x1E: /* ADD I, Vx (no VF change - Amiga quirk out of scope) */
            c->I = mem_addr((uint16_t)(c->I + c->V[x]));
            break;
        case 0x29: /* LD F, Vx: I = address of font glyph for VX's low nibble */
            c->I = (uint16_t)(CHIP8_FONTSET_ADDR + (c->V[x] & 0x0F) * 5);
            break;
        case 0x33: { /* LD B, Vx: BCD into I, I+1, I+2 (hundreds/tens/ones) */
            uint8_t v = c->V[x];
            c->memory[mem_addr(c->I)]     = v / 100;
            c->memory[mem_addr(c->I + 1)] = (v / 10) % 10;
            c->memory[mem_addr(c->I + 2)] = v % 10;
            break;
        }
        case 0x55: /* LD [I], Vx: store V0..Vx; VIP leaves I = I + X + 1 */
            for (uint8_t i = 0; i <= x; i++)
                c->memory[mem_addr((uint16_t)(c->I + i))] = c->V[i];
#if QUIRK_MEMORY_INCREMENT_I
            c->I = mem_addr((uint16_t)(c->I + x + 1));
#endif
            break;
        case 0x65: /* LD Vx, [I]: load V0..Vx; VIP leaves I = I + X + 1 */
            for (uint8_t i = 0; i <= x; i++)
                c->V[i] = c->memory[mem_addr((uint16_t)(c->I + i))];
#if QUIRK_MEMORY_INCREMENT_I
            c->I = mem_addr((uint16_t)(c->I + x + 1));
#endif
            break;
        default:
            unknown_opcode(c, opcode);
            break;
        }
        break;

    default:
        unknown_opcode(c, opcode);
        break;
    }
}
