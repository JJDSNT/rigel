#include "paula/audio.h"

#include <string.h>
#include <stddef.h>

static bool audio_valid_channel(int channel)
{
    return channel >= 0 && channel < RIGEL_PAULA_AUDIO_CHANNELS;
}

static rigel_audio_channel_t *audio_channel(audio_state_t *audio, int channel)
{
    if (audio == NULL || !audio_valid_channel(channel)) {
        return NULL;
    }

    return &audio->ch[channel];
}

static bool audio_channel_dma_enabled(const audio_state_t *audio, int channel)
{
    return audio != NULL && ((audio->dmacon & (1u << channel)) != 0);
}

static void audio_mix(audio_state_t *audio)
{
    int32_t left = 0;
    int32_t right = 0;

    if (audio == NULL) {
        return;
    }

    left += (audio->ch[0].current_sample * audio->ch[0].audvol) / 64;
    left += (audio->ch[3].current_sample * audio->ch[3].audvol) / 64;
    right += (audio->ch[1].current_sample * audio->ch[1].audvol) / 64;
    right += (audio->ch[2].current_sample * audio->ch[2].audvol) / 64;

    if (left > 32767) {
        left = 32767;
    } else if (left < -32768) {
        left = -32768;
    }

    if (right > 32767) {
        right = 32767;
    } else if (right < -32768) {
        right = -32768;
    }

    audio->mixed_left = (int16_t)left;
    audio->mixed_right = (int16_t)right;
}

void audio_reset(audio_state_t *audio)
{
    rigel_audio_irq_sink_t irq;

    if (audio == NULL) {
        return;
    }

    irq = audio->irq;
    (void)memset(audio, 0, sizeof(*audio));
    audio->irq = irq;
}

void audio_set_irq_sink(audio_state_t *audio, rigel_audio_irq_sink_t sink)
{
    if (audio == NULL) {
        return;
    }

    audio->irq = sink;
}

void audio_set_dmacon(audio_state_t *audio, rigel_u16 dmacon)
{
    if (audio == NULL) {
        return;
    }

    audio->dmacon = dmacon;
}

void audio_step(audio_state_t *audio, rigel_u32 cycles)
{
    int channel;

    if (audio == NULL) {
        return;
    }

    for (channel = 0; channel < RIGEL_PAULA_AUDIO_CHANNELS; ++channel) {
        rigel_audio_channel_t *state = &audio->ch[channel];
        rigel_u32 remaining = cycles;

        state->dma_enabled = audio_channel_dma_enabled(audio, channel);

        if (!state->dma_enabled && !state->data_pending) {
            continue;
        }

        if (state->audper == 0) {
            state->audper = 1;
        }

        if (state->period_counter == 0) {
            state->period_counter = state->audper;
        }

        while (remaining >= state->period_counter) {
            remaining -= state->period_counter;
            state->period_counter = state->audper;

            if (state->data_pending) {
                state->current_sample = (int16_t)((int8_t)(state->auddat >> 8)) << 8;
                state->data_pending = false;
            } else if (state->has_pending_lo) {
                state->current_sample = (int16_t)((int8_t)state->pending_lo) << 8;
                state->has_pending_lo = false;
            } else if (state->dma_word_ready) {
                state->current_sample = (int16_t)((int8_t)(state->dma_word >> 8)) << 8;
                state->pending_lo = (rigel_u8)(state->dma_word & 0xffu);
                state->has_pending_lo = true;
                state->dma_word_ready = false;
            }
        }

        state->period_counter = (rigel_u16)(state->period_counter - remaining);
    }

    audio_mix(audio);
}

void audio_write_lch(audio_state_t *audio, int channel, rigel_u16 value)
{
    rigel_audio_channel_t *state = audio_channel(audio, channel);

    if (state == NULL) {
        return;
    }

    state->audlc = (state->audlc & 0x0000ffffu) | ((rigel_u32)value << 16);
}

void audio_write_lcl(audio_state_t *audio, int channel, rigel_u16 value)
{
    rigel_audio_channel_t *state = audio_channel(audio, channel);

    if (state == NULL) {
        return;
    }

    state->audlc = (state->audlc & 0xffff0000u) | (value & 0xfffeu);
}

void audio_write_len(audio_state_t *audio, int channel, rigel_u16 value)
{
    rigel_audio_channel_t *state = audio_channel(audio, channel);

    if (state == NULL) {
        return;
    }

    state->audlen = value;
    if (state->current_length == 0) {
        state->current_length = value;
    }
}

void audio_write_per(audio_state_t *audio, int channel, rigel_u16 value)
{
    rigel_audio_channel_t *state = audio_channel(audio, channel);

    if (state == NULL) {
        return;
    }

    state->audper = value != 0 ? value : 1;
}

void audio_write_vol(audio_state_t *audio, int channel, rigel_u16 value)
{
    rigel_audio_channel_t *state = audio_channel(audio, channel);

    if (state == NULL) {
        return;
    }

    state->audvol = (rigel_u16)(value & 0x007fu);
    if (state->audvol > 64) {
        state->audvol = 64;
    }
}

void audio_write_dat(audio_state_t *audio, int channel, rigel_u16 value)
{
    rigel_audio_channel_t *state = audio_channel(audio, channel);

    if (state == NULL) {
        return;
    }

    state->auddat = value;
    state->data_pending = true;
}

int16_t audio_left(const audio_state_t *audio)
{
    return audio != NULL ? audio->mixed_left : 0;
}

int16_t audio_right(const audio_state_t *audio)
{
    return audio != NULL ? audio->mixed_right : 0;
}
