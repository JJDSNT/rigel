#ifndef RIGEL_HARNESS_VIDEO_H
#define RIGEL_HARNESS_VIDEO_H

#include <stdbool.h>
#include <stdint.h>

#include "harness.h"

/*
 * SDL2 presentation and input for the harness front-end.
 *
 * Only rigel-harness links this; the harness core stays SDL-free so the
 * timing tests build with no external dependency.
 */

typedef struct harness_video harness_video_t;

/*
 * SDL audio. The chipset produces samples as fast as the machine runs, which
 * is not the same pace the sound card consumes them, so the two are joined by
 * a ring buffer: harness_audio_push fills it from the emulation thread and
 * SDL's callback drains it.
 *
 * Returns the rate actually opened, or 0 if no device could be opened — the
 * caller should then leave the audio sink uninstalled.
 */
uint32_t harness_audio_open(uint32_t rate_hz);
void     harness_audio_close(void);
void     harness_audio_push(void *opaque, int16_t left, int16_t right);

harness_video_t *harness_video_open(const char *title, int scale);
void             harness_video_close(harness_video_t *v);

/* Present the frame Denise just completed. Safe to call with a frame whose
 * dimensions changed since the last call. */
void harness_video_present(harness_video_t *v, const rigel_frame_t *frame);

/*
 * Drain the SDL event queue, translating input into Rigel keyboard, mouse and
 * fire-button state. Returns false once the user has asked to quit.
 *
 * Grab the mouse with the right button or F12; Escape releases it.
 */
bool harness_video_pump(harness_video_t *v, harness_t *h);

#endif
