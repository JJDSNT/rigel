#ifndef RIGEL_AUDIO_H
#define RIGEL_AUDIO_H

#include "rigel_types.h"

typedef struct RigelContext RigelContext;

typedef struct rigel_audio_sample {
    rigel_i16 left;
    rigel_i16 right;
} rigel_audio_sample_t;

/*
 * Returns the current mixed stereo sample from Paula's four channels.
 * Call after rigel_step / rigel_step_until at whatever rate the host audio
 * callback requires. Pair with RIGEL_EVENT_HBLANK for a natural ~15 kHz tick,
 * or drive by stepping to sample-aligned cycle counts for higher-rate output.
 */
rigel_audio_sample_t rigel_get_audio_sample(const RigelContext *ctx);

#endif
