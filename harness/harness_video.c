#include "harness_video.h"

#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "rigel/rigel_denise_video.h"
#include "rigel/rigel_input.h"
#include "rigel/rigel_keyboard.h"

struct harness_video {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;
    int           tex_w;
    int           tex_h;
    int           scale;

    /* Amiga mouse counters are 8-bit quadrature pairs; the host accumulates
     * relative motion into them and publishes the pair as JOY0DAT. */
    uint8_t mouse_x;
    uint8_t mouse_y;
    bool    mouse_grabbed;

    bool quit;
};

/* -------------------------------------------------------------------------
 * Keyboard
 *
 * SDL scancodes are USB HID usage IDs, so this maps HID -> Amiga rawkey
 * directly. Rawkeys follow the Amiga Hardware Reference Manual, Appendix E.
 * ------------------------------------------------------------------------- */

static bool map_scancode(SDL_Scancode sc, uint8_t *rawkey)
{
    switch (sc) {
    /* Letters */
    case SDL_SCANCODE_A: *rawkey = 0x20u; return true;
    case SDL_SCANCODE_B: *rawkey = 0x35u; return true;
    case SDL_SCANCODE_C: *rawkey = 0x33u; return true;
    case SDL_SCANCODE_D: *rawkey = 0x22u; return true;
    case SDL_SCANCODE_E: *rawkey = 0x12u; return true;
    case SDL_SCANCODE_F: *rawkey = 0x23u; return true;
    case SDL_SCANCODE_G: *rawkey = 0x24u; return true;
    case SDL_SCANCODE_H: *rawkey = 0x25u; return true;
    case SDL_SCANCODE_I: *rawkey = 0x17u; return true;
    case SDL_SCANCODE_J: *rawkey = 0x26u; return true;
    case SDL_SCANCODE_K: *rawkey = 0x27u; return true;
    case SDL_SCANCODE_L: *rawkey = 0x28u; return true;
    case SDL_SCANCODE_M: *rawkey = 0x37u; return true;
    case SDL_SCANCODE_N: *rawkey = 0x36u; return true;
    case SDL_SCANCODE_O: *rawkey = 0x18u; return true;
    case SDL_SCANCODE_P: *rawkey = 0x19u; return true;
    case SDL_SCANCODE_Q: *rawkey = 0x10u; return true;
    case SDL_SCANCODE_R: *rawkey = 0x13u; return true;
    case SDL_SCANCODE_S: *rawkey = 0x21u; return true;
    case SDL_SCANCODE_T: *rawkey = 0x14u; return true;
    case SDL_SCANCODE_U: *rawkey = 0x16u; return true;
    case SDL_SCANCODE_V: *rawkey = 0x34u; return true;
    case SDL_SCANCODE_W: *rawkey = 0x11u; return true;
    case SDL_SCANCODE_X: *rawkey = 0x32u; return true;
    case SDL_SCANCODE_Y: *rawkey = 0x15u; return true;
    case SDL_SCANCODE_Z: *rawkey = 0x31u; return true;

    /* Digit row */
    case SDL_SCANCODE_1: *rawkey = 0x01u; return true;
    case SDL_SCANCODE_2: *rawkey = 0x02u; return true;
    case SDL_SCANCODE_3: *rawkey = 0x03u; return true;
    case SDL_SCANCODE_4: *rawkey = 0x04u; return true;
    case SDL_SCANCODE_5: *rawkey = 0x05u; return true;
    case SDL_SCANCODE_6: *rawkey = 0x06u; return true;
    case SDL_SCANCODE_7: *rawkey = 0x07u; return true;
    case SDL_SCANCODE_8: *rawkey = 0x08u; return true;
    case SDL_SCANCODE_9: *rawkey = 0x09u; return true;
    case SDL_SCANCODE_0: *rawkey = 0x0Au; return true;

    /* Punctuation */
    case SDL_SCANCODE_MINUS:        *rawkey = 0x0Bu; return true;
    case SDL_SCANCODE_EQUALS:       *rawkey = 0x0Cu; return true;
    case SDL_SCANCODE_BACKSLASH:    *rawkey = 0x0Du; return true;
    case SDL_SCANCODE_LEFTBRACKET:  *rawkey = 0x1Au; return true;
    case SDL_SCANCODE_RIGHTBRACKET: *rawkey = 0x1Bu; return true;
    case SDL_SCANCODE_SEMICOLON:    *rawkey = 0x29u; return true;
    case SDL_SCANCODE_APOSTROPHE:   *rawkey = 0x2Au; return true;
    case SDL_SCANCODE_GRAVE:        *rawkey = 0x00u; return true;
    case SDL_SCANCODE_COMMA:        *rawkey = 0x38u; return true;
    case SDL_SCANCODE_PERIOD:       *rawkey = 0x39u; return true;
    case SDL_SCANCODE_SLASH:        *rawkey = 0x3Au; return true;

    /* Editing and control */
    case SDL_SCANCODE_SPACE:     *rawkey = 0x40u; return true;
    case SDL_SCANCODE_BACKSPACE: *rawkey = 0x41u; return true;
    case SDL_SCANCODE_TAB:       *rawkey = 0x42u; return true;
    case SDL_SCANCODE_RETURN:    *rawkey = 0x44u; return true;
    case SDL_SCANCODE_ESCAPE:    *rawkey = 0x45u; return true;
    case SDL_SCANCODE_DELETE:    *rawkey = 0x46u; return true;
    case SDL_SCANCODE_CAPSLOCK:  *rawkey = 0x62u; return true;
    case SDL_SCANCODE_LCTRL:     *rawkey = 0x63u; return true;
    case SDL_SCANCODE_LSHIFT:    *rawkey = 0x60u; return true;
    case SDL_SCANCODE_RSHIFT:    *rawkey = 0x61u; return true;
    case SDL_SCANCODE_LALT:      *rawkey = 0x64u; return true;
    case SDL_SCANCODE_RALT:      *rawkey = 0x65u; return true;
    case SDL_SCANCODE_LGUI:      *rawkey = 0x66u; return true; /* Left Amiga */
    case SDL_SCANCODE_RGUI:      *rawkey = 0x67u; return true; /* Right Amiga */
    case SDL_SCANCODE_HELP:      *rawkey = 0x5Fu; return true;

    /* Cursor */
    case SDL_SCANCODE_UP:    *rawkey = 0x4Cu; return true;
    case SDL_SCANCODE_DOWN:  *rawkey = 0x4Du; return true;
    case SDL_SCANCODE_RIGHT: *rawkey = 0x4Eu; return true;
    case SDL_SCANCODE_LEFT:  *rawkey = 0x4Fu; return true;

    /* Function row */
    case SDL_SCANCODE_F1:  *rawkey = 0x50u; return true;
    case SDL_SCANCODE_F2:  *rawkey = 0x51u; return true;
    case SDL_SCANCODE_F3:  *rawkey = 0x52u; return true;
    case SDL_SCANCODE_F4:  *rawkey = 0x53u; return true;
    case SDL_SCANCODE_F5:  *rawkey = 0x54u; return true;
    case SDL_SCANCODE_F6:  *rawkey = 0x55u; return true;
    case SDL_SCANCODE_F7:  *rawkey = 0x56u; return true;
    case SDL_SCANCODE_F8:  *rawkey = 0x57u; return true;
    case SDL_SCANCODE_F9:  *rawkey = 0x58u; return true;
    case SDL_SCANCODE_F10: *rawkey = 0x59u; return true;

    /* Keypad */
    case SDL_SCANCODE_KP_0:        *rawkey = 0x0Fu; return true;
    case SDL_SCANCODE_KP_1:        *rawkey = 0x1Du; return true;
    case SDL_SCANCODE_KP_2:        *rawkey = 0x1Eu; return true;
    case SDL_SCANCODE_KP_3:        *rawkey = 0x1Fu; return true;
    case SDL_SCANCODE_KP_4:        *rawkey = 0x2Du; return true;
    case SDL_SCANCODE_KP_5:        *rawkey = 0x2Eu; return true;
    case SDL_SCANCODE_KP_6:        *rawkey = 0x2Fu; return true;
    case SDL_SCANCODE_KP_7:        *rawkey = 0x3Du; return true;
    case SDL_SCANCODE_KP_8:        *rawkey = 0x3Eu; return true;
    case SDL_SCANCODE_KP_9:        *rawkey = 0x3Fu; return true;
    case SDL_SCANCODE_KP_PERIOD:   *rawkey = 0x3Cu; return true;
    case SDL_SCANCODE_KP_ENTER:    *rawkey = 0x43u; return true;
    case SDL_SCANCODE_KP_MINUS:    *rawkey = 0x4Au; return true;
    case SDL_SCANCODE_KP_DIVIDE:   *rawkey = 0x5Cu; return true;
    case SDL_SCANCODE_KP_MULTIPLY: *rawkey = 0x5Du; return true;
    case SDL_SCANCODE_KP_PLUS:     *rawkey = 0x5Eu; return true;

    default: return false;
    }
}

/* -------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

harness_video_t *harness_video_open(const char *title, int scale)
{
    harness_video_t *v;

    if (scale < 1) scale = 2;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return NULL;
    }

    v = (harness_video_t *)calloc(1, sizeof(*v));
    if (v == NULL) {
        SDL_Quit();
        return NULL;
    }
    v->scale = scale;

    /* Sized for a lores PAL frame; the window follows the first real frame. */
    v->window = SDL_CreateWindow(title ? title : "rigel-harness",
                                 SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 320 * scale, 256 * scale,
                                 SDL_WINDOW_RESIZABLE);
    if (v->window == NULL) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        harness_video_close(v);
        return NULL;
    }

    v->renderer = SDL_CreateRenderer(v->window, -1, SDL_RENDERER_ACCELERATED);
    if (v->renderer == NULL)
        v->renderer = SDL_CreateRenderer(v->window, -1, SDL_RENDERER_SOFTWARE);
    if (v->renderer == NULL) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        harness_video_close(v);
        return NULL;
    }

    SDL_RenderSetLogicalSize(v->renderer, 320, 256);
    return v;
}

void harness_video_close(harness_video_t *v)
{
    if (v == NULL) return;
    if (v->texture)  SDL_DestroyTexture(v->texture);
    if (v->renderer) SDL_DestroyRenderer(v->renderer);
    if (v->window)   SDL_DestroyWindow(v->window);
    free(v);
    SDL_Quit();
}

/* -------------------------------------------------------------------------
 * Presentation
 * ------------------------------------------------------------------------- */

void harness_video_present(harness_video_t *v, const rigel_frame_t *frame)
{
    if (v == NULL || frame == NULL || frame->pixels == NULL) return;
    if (frame->width == 0u || frame->height == 0u) return;

    if (v->texture == NULL ||
        v->tex_w != (int)frame->width || v->tex_h != (int)frame->height) {
        if (v->texture) SDL_DestroyTexture(v->texture);
        /* Rigel's RGBA8888 target holds host-native 0x00RRGGBB words, which is
         * SDL's ARGB8888 with the alpha byte ignored. */
        v->texture = SDL_CreateTexture(v->renderer,
                                       SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       (int)frame->width, (int)frame->height);
        if (v->texture == NULL) return;
        v->tex_w = (int)frame->width;
        v->tex_h = (int)frame->height;
        SDL_RenderSetLogicalSize(v->renderer, v->tex_w, v->tex_h);
        SDL_SetWindowSize(v->window, v->tex_w * v->scale, v->tex_h * v->scale);
    }

    SDL_UpdateTexture(v->texture, NULL, frame->pixels, (int)frame->pitch);
    SDL_RenderClear(v->renderer);
    SDL_RenderCopy(v->renderer, v->texture, NULL, NULL);
    SDL_RenderPresent(v->renderer);
}

/* -------------------------------------------------------------------------
 * Input
 * ------------------------------------------------------------------------- */

static void set_grab(harness_video_t *v, bool on)
{
    if (v->mouse_grabbed == on) return;
    v->mouse_grabbed = on;
    SDL_SetRelativeMouseMode(on ? SDL_TRUE : SDL_FALSE);
}

bool harness_video_pump(harness_video_t *v, harness_t *h)
{
    RigelContext *ctx;
    SDL_Event ev;

    if (v == NULL || h == NULL) return false;
    ctx = harness_rigel(h);

    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            v->quit = true;
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            uint8_t rawkey;
            bool down = (ev.type == SDL_KEYDOWN);

            if (ev.key.repeat) break;

            /* Host-side controls, not passed to the guest. */
            if (down && ev.key.keysym.scancode == SDL_SCANCODE_F12) {
                set_grab(v, !v->mouse_grabbed);
                break;
            }
            if (down && ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE &&
                v->mouse_grabbed) {
                set_grab(v, false);
                break;
            }

            if (map_scancode(ev.key.keysym.scancode, &rawkey))
                rigel_keyboard_inject(ctx, rawkey, down);
            break;
        }

        case SDL_MOUSEMOTION:
            if (!v->mouse_grabbed) break;
            v->mouse_x = (uint8_t)(v->mouse_x + (int8_t)ev.motion.xrel);
            v->mouse_y = (uint8_t)(v->mouse_y + (int8_t)ev.motion.yrel);
            rigel_input_set_joydat(ctx, 0u,
                                   (rigel_u16)(((rigel_u16)v->mouse_y << 8) |
                                               v->mouse_x));
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            bool down = (ev.type == SDL_MOUSEBUTTONDOWN);

            if (ev.button.button == SDL_BUTTON_LEFT) {
                if (down && !v->mouse_grabbed) {
                    /* First click inside the window takes the mouse rather
                     * than reaching the guest. */
                    set_grab(v, true);
                    break;
                }
                rigel_input_set_fire(ctx, 0u, down);
            } else if (ev.button.button == SDL_BUTTON_RIGHT) {
                rigel_input_set_pot_button_x(ctx, 0u, down);
            } else if (ev.button.button == SDL_BUTTON_MIDDLE) {
                rigel_input_set_pot_button_y(ctx, 0u, down);
            }
            break;
        }

        default:
            break;
        }
    }

    return !v->quit;
}

/* -------------------------------------------------------------------------
 * Audio
 * ------------------------------------------------------------------------- */

enum {
    /* Roughly a third of a second at 48 kHz. Deep enough to ride the jitter of
     * an emulator that runs in bursts, shallow enough that input still feels
     * attached to the sound. */
    AUDIO_RING_FRAMES = 16384
};

typedef struct audio_ring {
    int16_t   buf[AUDIO_RING_FRAMES * 2];
    unsigned  head;          /* written by the emulation side */
    unsigned  tail;          /* read by the SDL callback      */
    SDL_AudioDeviceID dev;
    uint64_t  underruns;
    int16_t   last_l, last_r;
} audio_ring_t;

static audio_ring_t g_audio;

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    audio_ring_t *a = (audio_ring_t *)userdata;
    int16_t *out = (int16_t *)stream;
    int frames = len / (int)(2 * sizeof(int16_t));
    int i;

    for (i = 0; i < frames; i++) {
        unsigned tail = a->tail;
        if (tail == a->head) {
            /* Underrun: hold the last sample rather than emitting silence,
             * which clicks far more audibly than a short freeze. */
            a->underruns++;
            out[i * 2]     = a->last_l;
            out[i * 2 + 1] = a->last_r;
            continue;
        }
        a->last_l = a->buf[tail * 2];
        a->last_r = a->buf[tail * 2 + 1];
        out[i * 2]     = a->last_l;
        out[i * 2 + 1] = a->last_r;
        a->tail = (tail + 1u) % AUDIO_RING_FRAMES;
    }
}

uint32_t harness_audio_open(uint32_t rate_hz)
{
    SDL_AudioSpec want, have;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        SDL_Log("SDL audio init failed: %s", SDL_GetError());
        return 0;
    }

    SDL_memset(&want, 0, sizeof(want));
    want.freq     = (int)rate_hz;
    want.format   = AUDIO_S16SYS;
    want.channels = 2;
    want.samples  = 1024;
    want.callback = audio_callback;
    want.userdata = &g_audio;

    g_audio.dev = SDL_OpenAudioDevice(NULL, 0, &want, &have,
                                      SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (g_audio.dev == 0) {
        SDL_Log("SDL_OpenAudioDevice failed: %s", SDL_GetError());
        return 0;
    }

    SDL_PauseAudioDevice(g_audio.dev, 0);
    return (uint32_t)have.freq;
}

void harness_audio_close(void)
{
    if (g_audio.dev != 0) {
        SDL_CloseAudioDevice(g_audio.dev);
        g_audio.dev = 0;
    }
}

void harness_audio_push(void *opaque, int16_t left, int16_t right)
{
    audio_ring_t *a = &g_audio;
    unsigned next;

    (void)opaque;

    next = (a->head + 1u) % AUDIO_RING_FRAMES;
    if (next == a->tail) {
        /* Ring full: the machine is ahead of the sound card. Dropping the
         * newest sample keeps latency bounded instead of letting it grow. */
        return;
    }
    a->buf[a->head * 2]     = left;
    a->buf[a->head * 2 + 1] = right;
    a->head = next;
}
