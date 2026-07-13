/*
 * chip8.h - CHIP-8 CPU core state and interface.
 *
 * This layer is pure emulation: memory, registers, fetch-decode-execute,
 * framebuffer, and keypad state. It contains NO SDL calls (negative
 * acceptance criterion: opcodes must not touch SDL). The only path to the
 * screen is the 64x32 framebuffer, which the platform layer reads.
 */
#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ---- Machine geometry (original COSMAC VIP CHIP-8) ---- */
#define CHIP8_MEMORY_SIZE   4096
#define CHIP8_STACK_SIZE    16
#define CHIP8_NUM_REGS      16
#define CHIP8_NUM_KEYS      16

#define CHIP8_DISPLAY_WIDTH  64
#define CHIP8_DISPLAY_HEIGHT 32
#define CHIP8_DISPLAY_PIXELS (CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT)

/* Memory map: 0x000-0x1FF interpreter area, fontset conventionally at
 * 0x050-0x09F, ROM loaded at 0x200. */
#define CHIP8_FONTSET_ADDR  0x050
#define CHIP8_FONTSET_SIZE  80      /* 16 chars * 5 bytes */
#define CHIP8_PROGRAM_START 0x200
#define CHIP8_MAX_ROM_SIZE  (CHIP8_MEMORY_SIZE - CHIP8_PROGRAM_START) /* 3584 */

/* ---- VIP-classic quirk profile (compile-time constants) ----
 * Fixed to the original COSMAC VIP semantics. Separated out so they can be
 * flipped after learning without hunting through the opcode code. */
#define QUIRK_VF_RESET          1  /* 8XY1/8XY2/8XY3 reset VF to 0 */
#define QUIRK_MEMORY_INCREMENT_I 1 /* FX55/FX65 leave I = I + X + 1 */
#define QUIRK_SHIFT_VY          1  /* 8XY6/8XYE shift VY into VX */
#define QUIRK_CLIPPING          1  /* DXYN clips sprite pixels at edges */
#define QUIRK_DISPLAY_WAIT      1  /* DXYN runs at most once per frame */
#define QUIRK_JUMP_V0           1  /* BNNN jumps to NNN + V0 */

/* Reason a cycle stopped the machine (all print diagnostics to stderr). */
typedef enum {
    CHIP8_RUN = 0,
    CHIP8_HALT_STACK_OVERFLOW,
    CHIP8_HALT_STACK_UNDERFLOW,
    CHIP8_HALT_UNKNOWN_OPCODE
} chip8_status_t;

typedef struct {
    uint8_t  memory[CHIP8_MEMORY_SIZE];
    uint8_t  V[CHIP8_NUM_REGS];          /* V0-VF general registers */
    uint16_t I;                          /* index register */
    uint16_t pc;                         /* program counter */
    uint16_t stack[CHIP8_STACK_SIZE];
    uint8_t  sp;                         /* stack pointer (next free slot) */

    uint8_t  delay_timer;
    uint8_t  sound_timer;

    /* 64x32 monochrome framebuffer: each byte is 0 or 1. Sole draw path. */
    uint8_t  display[CHIP8_DISPLAY_PIXELS];
    /* Current keypad state: keypad[i] != 0 means CHIP-8 key i is held. */
    uint8_t  keypad[CHIP8_NUM_KEYS];

    /* FX0A state: key index remembered on press, registered on release.
     * -1 means "no key pressed yet". */
    int      fx0a_key;

    bool     drew;                       /* set true when a DXYN executed */
    chip8_status_t status;               /* CHIP8_RUN while alive */
} chip8_t;

/* Initialize: zero state, load fontset at 0x050, PC=0x200, seed RNG. */
void chip8_init(chip8_t *c);

/* Load ROM file into memory at 0x200.
 * Returns 0 on success. On failure prints an explicit error to stderr and
 * returns a nonzero exit code suitable for main(): missing file, oversized
 * ROM, or read error. */
int chip8_load_rom(chip8_t *c, const char *path);

/* Execute exactly one fetch-decode-execute cycle. On a fatal condition
 * (unknown opcode, stack over/underflow) prints a diagnostic including PC
 * and sets c->status; the caller should stop running when status != RUN. */
void chip8_cycle(chip8_t *c);

/* Decrement delay and sound timers by one if nonzero (called at 60Hz). */
void chip8_tick_timers(chip8_t *c);

#endif /* CHIP8_H */
