#ifndef AUDIO_H
#define AUDIO_H

typedef struct audio_state {
    unsigned channels;
} audio_state_t;

void audio_reset(audio_state_t *audio);

#endif
