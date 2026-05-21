#include "paula/audio.h"

#include <stddef.h>

void audio_reset(audio_state_t *audio)
{
    if (audio == NULL) {
        return;
    }

    audio->channels = 4;
}
