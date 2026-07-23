/*
 * main.c - CLI shell and the frame-paced main loop.
 *
 * Timing model: the two clocks are independent named constants.
 *   CHIP8_CPU_HZ   - instructions executed per second (700 IPS)
 *   CHIP8_TIMER_HZ - delay/sound timer decrement rate (60 Hz)
 * The loop runs one frame at CHIP8_TIMER_HZ; each frame it executes
 * CHIP8_CPU_HZ/CHIP8_TIMER_HZ instructions and ticks the timers once. Changing
 * CHIP8_CPU_HZ therefore changes instruction density but never timer speed.
 */
#include "chip8.h"
#include "platform.h"

#include <stdio.h>
#include <stdlib.h>

#define CHIP8_CPU_HZ    700   /* instructions per second */
#define CHIP8_TIMER_HZ  60    /* timer + frame rate */
#define DISPLAY_SCALE   10    /* integer pixel scale: 640x320 window */

#define DUMP_SCALE       8       /* headless snapshot scale: 512x256 BMP */
#define DUMP_DEFAULT_CYCLES 600000L /* CPU cycles before dumping (settle time) */

static void usage(const char *prog) {
    fprintf(stderr, "usage: %s <rom>\n", prog);
}

#define DUMP_KEY_HOLD_CYCLES 40000L /* how long a scripted key is held down */

#define KEYSEQ_MAX 64 /* max scripted key events (CHIP8_DUMP_KEYSEQ) */

/* One scripted key transition, applied at the start of frame `frame`. */
typedef struct {
    long frame;
    int  key;   /* 0x0..0xF */
    int  down;  /* 1 = press, 0 = release */
} keyev_t;

/* Parse CHIP8_DUMP_KEYSEQ: comma list of "frame:key:d" / "frame:key:u"
 * (frame decimal, key hex). Returns event count (0 on unset/parse failure).
 * Example: "60:5:d,70:5:u" presses key 5 at frame 60, releases at 70. */
static int parse_keyseq(const char *s, keyev_t *ev, int max) {
    int n = 0;
    if (!s || !*s)
        return 0;
    while (*s && n < max) {
        char *end = NULL;
        long frame = strtol(s, &end, 10);
        if (end == s || *end != ':' || frame < 0)
            return 0;
        s = end + 1;
        long key = strtol(s, &end, 16);
        if (end == s || *end != ':' || key < 0 || key >= CHIP8_NUM_KEYS)
            return 0;
        s = end + 1;
        if (*s != 'd' && *s != 'u')
            return 0;
        ev[n].frame = frame;
        ev[n].key = (int)key;
        ev[n].down = (*s == 'd');
        n++;
        s++;
        if (*s == ',')
            s++;
        else if (*s != '\0')
            return 0;
    }
    return n;
}

/* Extra headless instrumentation, all env-gated and off by default. */
typedef struct {
    keyev_t ev[KEYSEQ_MAX]; /* CHIP8_DUMP_KEYSEQ: scripted key transitions */
    int     ev_count;
    long    dump_every;     /* CHIP8_DUMP_EVERY: BMP every N frames */
    FILE   *beep_log;       /* CHIP8_BEEP_LOG: per-frame sound_timer trace */
} headless_opts_t;

/* Headless dump mode (CHIP8_DUMP set): run the CPU for `cycles` instructions
 * with timers ticking at the normal CPU:timer ratio (no real-time pacing, no
 * window), then write the framebuffer to `path` as a BMP. Returns process exit
 * code. Leaves the interactive path untouched.
 *
 * If `key` is in 0x0..0xF, that hex key is injected as PRESSED for the first
 * DUMP_KEY_HOLD_CYCLES cycles, then RELEASED for the remainder. This drives
 * both the "is key down" (EX9E/EXA1) and FX0A (registers on release) paths so
 * a menu-driven ROM (e.g. quirks platform picker) can advance. key < 0 = none.
 * (Use either CHIP8_DUMP_KEY or CHIP8_DUMP_KEYSEQ, not both: DUMP_KEY rewrites
 * its key every cycle and would fight scripted transitions on the same key.)
 *
 * `opt` adds scripted key events (applied at frame boundaries), periodic BMP
 * dumps ("<path>.f<frame>.bmp"), and a per-frame beep trace
 * ("frame sound_timer pixels_lit" lines) - the hooks the interactive-item
 * verification (keypad / beep / gameplay) drives. */
static int run_headless(chip8_t *chip8, const char *path, long cycles, int key,
                        const headless_opts_t *opt) {
    const int ips_per_frame = CHIP8_CPU_HZ / CHIP8_TIMER_HZ;
    long frames = cycles / ips_per_frame;
    if (frames < 1)
        frames = 1;
    long executed = 0;

    /* Mirror the interactive frame loop so the VIP display-wait quirk is
     * honored headless: at most one DXYN draws per frame, timers tick once
     * per frame. Without this the quirks test mis-detects display-wait. */
    for (long f = 0; f < frames; f++) {
        for (int e = 0; e < opt->ev_count; e++) {
            if (opt->ev[e].frame == f)
                chip8->keypad[opt->ev[e].key] = opt->ev[e].down ? 1 : 0;
        }
        chip8->drew = false;
        for (int i = 0; i < ips_per_frame; i++) {
            if (key >= 0 && key < CHIP8_NUM_KEYS)
                chip8->keypad[key] = (executed < DUMP_KEY_HOLD_CYCLES) ? 1 : 0;
            chip8_cycle(chip8);
            executed++;
            if (chip8->status != CHIP8_RUN)
                goto done;
#if QUIRK_DISPLAY_WAIT
            if (chip8->drew)
                break;
#endif
        }
        chip8_tick_timers(chip8);
        if (opt->beep_log) {
            int lit = 0;
            for (int px = 0; px < CHIP8_DISPLAY_PIXELS; px++)
                lit += chip8->display[px] ? 1 : 0;
            fprintf(opt->beep_log, "%ld %u %d\n", f, chip8->sound_timer, lit);
        }
        if (opt->dump_every > 0 && f > 0 && f % opt->dump_every == 0) {
            char fpath[1024];
            snprintf(fpath, sizeof(fpath), "%s.f%05ld.bmp", path, f);
            if (platform_dump_bmp(chip8->display, DUMP_SCALE, fpath) == 0)
                fprintf(stderr, "[dump] wrote %s\n", fpath);
        }
    }
done:

    if (platform_dump_bmp(chip8->display, DUMP_SCALE, path) != 0)
        return 6;
    fprintf(stderr, "[dump] wrote %s\n", path);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    chip8_t chip8;
    chip8_init(&chip8);

    /* CHIP8_SEED (env-gated): reseed CXNN's RNG after init's srand(time) so
     * scripted verification runs are reproducible. Unset = normal behavior. */
    const char *seed_env = getenv("CHIP8_SEED");
    if (seed_env && seed_env[0] != '\0')
        srand((unsigned)strtoul(seed_env, NULL, 10));

    int rc = chip8_load_rom(&chip8, argv[1]);
    if (rc != 0)
        return rc;

    /* Headless framebuffer dump: env-gated, no window, exits when done.
     * Behavior is identical to before when CHIP8_DUMP is unset. */
    const char *dump_path = getenv("CHIP8_DUMP");
    if (dump_path && dump_path[0] != '\0') {
        long cycles = DUMP_DEFAULT_CYCLES;
        const char *cyc_env = getenv("CHIP8_DUMP_CYCLES");
        if (cyc_env && cyc_env[0] != '\0') {
            char *end = NULL;
            long v = strtol(cyc_env, &end, 10);
            if (end != cyc_env && v > 0)
                cycles = v;
        }
        int key = -1;
        const char *key_env = getenv("CHIP8_DUMP_KEY");
        if (key_env && key_env[0] != '\0') {
            char *end = NULL;
            long k = strtol(key_env, &end, 16);
            if (end != key_env && k >= 0 && k < CHIP8_NUM_KEYS)
                key = (int)k;
        }
        headless_opts_t opt = {0};
        opt.ev_count = parse_keyseq(getenv("CHIP8_DUMP_KEYSEQ"),
                                    opt.ev, KEYSEQ_MAX);
        const char *every_env = getenv("CHIP8_DUMP_EVERY");
        if (every_env && every_env[0] != '\0') {
            char *end = NULL;
            long v = strtol(every_env, &end, 10);
            if (end != every_env && v > 0)
                opt.dump_every = v;
        }
        const char *beep_env = getenv("CHIP8_BEEP_LOG");
        if (beep_env && beep_env[0] != '\0')
            opt.beep_log = fopen(beep_env, "w");
        int hrc = run_headless(&chip8, dump_path, cycles, key, &opt);
        if (opt.beep_log)
            fclose(opt.beep_log);
        return hrc;
    }

    platform_t *plat = platform_create(DISPLAY_SCALE);
    if (!plat)
        return 4;

    const int    ips_per_frame = CHIP8_CPU_HZ / CHIP8_TIMER_HZ;
    const double frame_ms       = 1000.0 / CHIP8_TIMER_HZ;

#ifdef CHIP8_TIMER_LOG
    unsigned long timer_ticks = 0;
    uint32_t start_ms = platform_ticks_ms();
#endif

    bool quit = false;
    double next_frame = (double)platform_ticks_ms();

    while (!quit) {
        platform_poll_input(plat, chip8.keypad, &quit);
        if (quit)
            break;

        /* Run this frame's slice of instructions. VIP display-wait: at most
         * one DXYN draws per frame, after which the CPU idles until vblank. */
        chip8.drew = false;
        for (int i = 0; i < ips_per_frame; i++) {
            chip8_cycle(&chip8);
            if (chip8.status != CHIP8_RUN) { quit = true; break; }
#if QUIRK_DISPLAY_WAIT
            if (chip8.drew) break;
#endif
        }

        /* Timers tick once per frame -> exactly CHIP8_TIMER_HZ per second. */
        chip8_tick_timers(&chip8);
#ifdef CHIP8_TIMER_LOG
        timer_ticks++;
#endif

        platform_set_beep(plat, chip8.sound_timer > 0);
        platform_render(plat, chip8.display);

        if (quit)
            break;

        /* Pace to the next frame boundary by sleeping (no busy-spin). */
        next_frame += frame_ms;
        double now = (double)platform_ticks_ms();
        double remaining = next_frame - now;
        if (remaining > 0.0) {
            platform_delay_ms((uint32_t)remaining);
        } else {
            /* Fell behind: resync so we don't spiral. */
            next_frame = now;
        }
    }

#ifdef CHIP8_TIMER_LOG
    {
        uint32_t elapsed = platform_ticks_ms() - start_ms;
        fprintf(stderr, "[timer-log] %lu timer ticks in %u ms (%.2f Hz)\n",
                timer_ticks, elapsed,
                elapsed ? timer_ticks * 1000.0 / elapsed : 0.0);
    }
#endif

    platform_destroy(plat);
    return (chip8.status == CHIP8_RUN) ? 0 : 5;
}
