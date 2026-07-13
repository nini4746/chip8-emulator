/*
 * platform.c - SDL2 backend for window, input, and beep.
 */
#include "platform.h"
#include "chip8.h"   /* for display geometry constants only */

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BEEP_FREQ_HZ   440
#define AUDIO_SAMPLE_HZ 44100
#define AUDIO_AMPLITUDE 6000

struct platform {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;
    SDL_AudioDeviceID audio;

    /* Audio callback state (touched from the audio thread). */
    volatile int beep_on;
    double phase;
};

/* Map an SDL scancode to a CHIP-8 keypad index, or -1 if unmapped.
 *   1 2 3 C        1 2 3 4
 *   4 5 6 D   <-   Q W E R
 *   7 8 9 E        A S D F
 *   A 0 B F        Z X C V
 */
static int scancode_to_key(SDL_Scancode sc) {
    switch (sc) {
    case SDL_SCANCODE_1: return 0x1;
    case SDL_SCANCODE_2: return 0x2;
    case SDL_SCANCODE_3: return 0x3;
    case SDL_SCANCODE_4: return 0xC;
    case SDL_SCANCODE_Q: return 0x4;
    case SDL_SCANCODE_W: return 0x5;
    case SDL_SCANCODE_E: return 0x6;
    case SDL_SCANCODE_R: return 0xD;
    case SDL_SCANCODE_A: return 0x7;
    case SDL_SCANCODE_S: return 0x8;
    case SDL_SCANCODE_D: return 0x9;
    case SDL_SCANCODE_F: return 0xE;
    case SDL_SCANCODE_Z: return 0xA;
    case SDL_SCANCODE_X: return 0x0;
    case SDL_SCANCODE_C: return 0xB;
    case SDL_SCANCODE_V: return 0xF;
    default:             return -1;
    }
}

/* Square-wave generator. Emits silence unless beep_on is set. */
static void audio_callback(void *userdata, Uint8 *stream, int len) {
    struct platform *p = (struct platform *)userdata;
    Sint16 *out = (Sint16 *)stream;
    int samples = len / (int)sizeof(Sint16);
    double step = (double)BEEP_FREQ_HZ / AUDIO_SAMPLE_HZ;

    if (!p->beep_on) {
        memset(stream, 0, (size_t)len);
        return;
    }
    for (int i = 0; i < samples; i++) {
        out[i] = (p->phase < 0.5) ? (Sint16)AUDIO_AMPLITUDE
                                  : (Sint16)(-AUDIO_AMPLITUDE);
        p->phase += step;
        if (p->phase >= 1.0) p->phase -= 1.0;
    }
}

platform_t *platform_create(int scale) {
    if (scale < 1) scale = 1;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "error: SDL_Init failed: %s\n", SDL_GetError());
        return NULL;
    }

    platform_t *p = calloc(1, sizeof(*p));
    if (!p) {
        fprintf(stderr, "error: out of memory\n");
        SDL_Quit();
        return NULL;
    }

    p->window = SDL_CreateWindow(
        "CHIP-8",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        CHIP8_DISPLAY_WIDTH * scale, CHIP8_DISPLAY_HEIGHT * scale,
        SDL_WINDOW_SHOWN);
    if (!p->window) {
        fprintf(stderr, "error: SDL_CreateWindow failed: %s\n", SDL_GetError());
        goto fail;
    }

    /* No vsync: the main loop paces frames itself (60Hz), keeping timer
     * speed independent of the display refresh rate. */
    p->renderer = SDL_CreateRenderer(p->window, -1, SDL_RENDERER_ACCELERATED);
    if (!p->renderer)
        p->renderer = SDL_CreateRenderer(p->window, -1, 0);
    if (!p->renderer) {
        fprintf(stderr, "error: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        goto fail;
    }

    p->texture = SDL_CreateTexture(
        p->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        CHIP8_DISPLAY_WIDTH, CHIP8_DISPLAY_HEIGHT);
    if (!p->texture) {
        fprintf(stderr, "error: SDL_CreateTexture failed: %s\n", SDL_GetError());
        goto fail;
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = AUDIO_SAMPLE_HZ;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = audio_callback;
    want.userdata = p;
    p->audio = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (p->audio == 0) {
        /* Audio is not fatal - the emulator still runs silently. */
        fprintf(stderr, "warning: audio unavailable: %s\n", SDL_GetError());
    } else {
        SDL_PauseAudioDevice(p->audio, 0); /* start; callback emits silence */
    }

    return p;

fail:
    platform_destroy(p);
    return NULL;
}

void platform_poll_input(platform_t *p, uint8_t keypad[16], bool *quit) {
    (void)p;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            *quit = true;
            break;
        case SDL_KEYDOWN:
            if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                *quit = true;
                break;
            }
            {
                int k = scancode_to_key(e.key.keysym.scancode);
                if (k >= 0) keypad[k] = 1;
            }
            break;
        case SDL_KEYUP: {
            int k = scancode_to_key(e.key.keysym.scancode);
            if (k >= 0) keypad[k] = 0;
            break;
        }
        default:
            break;
        }
    }
}

void platform_render(platform_t *p, const uint8_t *framebuffer) {
    /* Convert the 0/1 framebuffer into RGBA8888 pixels. */
    static uint32_t pixels[CHIP8_DISPLAY_PIXELS];
    for (int i = 0; i < CHIP8_DISPLAY_PIXELS; i++)
        pixels[i] = framebuffer[i] ? 0xFFFFFFFFu : 0x000000FFu;

    SDL_UpdateTexture(p->texture, NULL, pixels,
                      CHIP8_DISPLAY_WIDTH * (int)sizeof(uint32_t));
    SDL_RenderClear(p->renderer);
    SDL_RenderCopy(p->renderer, p->texture, NULL, NULL);
    SDL_RenderPresent(p->renderer);
}

int platform_dump_bmp(const uint8_t *framebuffer, int scale, const char *path) {
    if (scale < 1) scale = 1;
    const int w = CHIP8_DISPLAY_WIDTH * scale;
    const int h = CHIP8_DISPLAY_HEIGHT * scale;

    /* 24-bit RGB (no alpha) so the output is a plain BMP that common tools
     * (e.g. macOS `sips`) can read; 32-bit RGBA yields a V4-header BMP some
     * decoders reject. */
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(
        0, w, h, 24, SDL_PIXELFORMAT_RGB24);
    if (!surf) {
        fprintf(stderr, "error: SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_LockSurface(surf);
    uint8_t *rows = (uint8_t *)surf->pixels;
    for (int y = 0; y < h; y++) {
        const int sy = y / scale;
        uint8_t *row = rows + (size_t)y * surf->pitch;
        for (int x = 0; x < w; x++) {
            const int sx = x / scale;
            uint8_t v = framebuffer[sy * CHIP8_DISPLAY_WIDTH + sx] ? 0xFF : 0x00;
            row[x * 3 + 0] = v;
            row[x * 3 + 1] = v;
            row[x * 3 + 2] = v;
        }
    }
    SDL_UnlockSurface(surf);

    int rc = SDL_SaveBMP(surf, path);
    SDL_FreeSurface(surf);
    if (rc != 0) {
        fprintf(stderr, "error: SDL_SaveBMP failed: %s\n", SDL_GetError());
        return -1;
    }
    return 0;
}

void platform_set_beep(platform_t *p, bool on) {
    if (p->audio == 0) return;
    p->beep_on = on ? 1 : 0;
}

uint32_t platform_ticks_ms(void) {
    return SDL_GetTicks();
}

void platform_delay_ms(uint32_t ms) {
    SDL_Delay(ms);
}

void platform_destroy(platform_t *p) {
    if (!p) return;
    if (p->audio) SDL_CloseAudioDevice(p->audio);
    if (p->texture) SDL_DestroyTexture(p->texture);
    if (p->renderer) SDL_DestroyRenderer(p->renderer);
    if (p->window) SDL_DestroyWindow(p->window);
    free(p);
    SDL_Quit();
}
