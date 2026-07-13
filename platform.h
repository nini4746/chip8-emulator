/*
 * platform.h - SDL2 I/O layer: window/rendering, keypad input, beep audio.
 * This is the only place SDL is touched. It reads the CHIP-8 framebuffer and
 * keypad; it never reaches into CPU internals beyond those.
 */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct platform platform_t;

/* Create window (WIDTH*scale x HEIGHT*scale), renderer, texture, and audio.
 * Returns NULL on failure (message already printed to stderr). */
platform_t *platform_create(int scale);

/* Pump SDL events. Updates the 16-entry keypad state array (1=held).
 * Sets *quit to true on ESC, window close, or SDL_QUIT. */
void platform_poll_input(platform_t *p, uint8_t keypad[16], bool *quit);

/* Draw the 64x32 framebuffer (each byte 0/1) scaled to the window. */
void platform_render(platform_t *p, const uint8_t *framebuffer);

/* Turn the square-wave beep on or off. */
void platform_set_beep(platform_t *p, bool on);

/* Wall-clock milliseconds since SDL init (for frame pacing). */
uint32_t platform_ticks_ms(void);

/* Sleep this thread for the given milliseconds (yields the CPU). */
void platform_delay_ms(uint32_t ms);

void platform_destroy(platform_t *p);

#endif /* PLATFORM_H */
