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

static void usage(const char *prog) {
    fprintf(stderr, "usage: %s <rom>\n", prog);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    chip8_t chip8;
    chip8_init(&chip8);

    int rc = chip8_load_rom(&chip8, argv[1]);
    if (rc != 0)
        return rc;

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
