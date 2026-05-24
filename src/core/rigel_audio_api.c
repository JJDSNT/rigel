#include "rigel/rigel_audio.h"

#include "core/rigel_context.h"
#include "chipset/paula/audio.h"

rigel_audio_sample_t rigel_get_audio_sample(const RigelContext *ctx)
{
    rigel_audio_sample_t sample;

    sample.left  = 0;
    sample.right = 0;

    if (ctx == NULL) {
        return sample;
    }

    sample.left  = audio_left(&ctx->chipset.paula.audio);
    sample.right = audio_right(&ctx->chipset.paula.audio);
    return sample;
}
