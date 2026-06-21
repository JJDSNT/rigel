#include "paula/audio.h"

#include "debug/log.h"

#include <string.h>
#include <stddef.h>

/* AUD0-3 timing trace: AUDxPER write, AUDxLC/LEN reload, DMA word fetch,
 * period elapsed, AUDxDAT write, IRQ raise — all 4 channels. Bounded by a
 * per-event-type counter (shared across all 4 channels per call site),
 * matching the existing convention in chipset/agnus (copper_exec.c,
 * slot_scheduler.c uses 512), since Rigel's own native/test builds default
 * to stderr when no host log_event_fn is set. 512 is far too low here:
 * the period-tick event alone can fire every ~200-900 CCK per channel, so
 * it exhausts within the first fraction of a second of boot, well before
 * any interesting timing window — unlike the narrow, late-triggered
 * conditions copper_exec.c/slot_scheduler.c guard. Sized instead so a
 * multi-second bring-up capture (~10M+ CCK) doesn't go dark. */
#define AUDIO_TRACE_LIMIT 1000000u

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

    audio->mixed_left  = (int16_t)left;
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
    bool sample_elapsed = false;

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
            sample_elapsed = true;

            {
                static unsigned trace_count = 0u;
                if (trace_count < AUDIO_TRACE_LIMIT) {
                    rigel_log_event_t event = {
                        RIGEL_LOG_EVENT_AUDIO_PERIOD,
                        "audio_period",
                        { (rigel_u32)channel, (rigel_u32)state->audper },
                        2u
                    };
                    rigel_log_event(&event);
                    trace_count++;
                }
            }

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
    if (sample_elapsed) {
        audio->sample_ready = true;
    }
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

    {
        static unsigned trace_count = 0u;
        if (trace_count < AUDIO_TRACE_LIMIT) {
            rigel_log_event_t event = {
                RIGEL_LOG_EVENT_AUDIO_PER_WRITE,
                "audio_per_write",
                { (rigel_u32)channel, (rigel_u32)value, (rigel_u32)state->audper },
                3u
            };
            rigel_log_event(&event);
            trace_count++;
        }
    }
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

    {
        static unsigned trace_count = 0u;
        if (trace_count < AUDIO_TRACE_LIMIT) {
            rigel_log_event_t event = {
                RIGEL_LOG_EVENT_AUDIO_DAT_WRITE,
                "audio_dat_write",
                { (rigel_u32)channel, (rigel_u32)value },
                2u
            };
            rigel_log_event(&event);
            trace_count++;
        }
    }
}

int16_t audio_left(const audio_state_t *audio)
{
    return audio != NULL ? audio->mixed_left : 0;
}

int16_t audio_right(const audio_state_t *audio)
{
    return audio != NULL ? audio->mixed_right : 0;
}

void audio_dma_step_slot(audio_state_t *audio, int channel, rigel_chip_ram_if_t mem)
{
    /* AUDx IRQ bits: channels 0-3 map to INTREQ bits 7-10 */
    static const rigel_u16 irq_bits[RIGEL_PAULA_AUDIO_CHANNELS] = {
        0x0080u, 0x0100u, 0x0200u, 0x0400u
    };
    rigel_audio_channel_t *ch;

    if (audio == NULL || !audio_valid_channel(channel) || mem.read16 == NULL) {
        return;
    }

    ch = &audio->ch[channel];
    if (!audio_channel_dma_enabled(audio, channel) || ch->dma_word_ready) {
        return;
    }

    /* Initialize pointer on first use or after reload */
    if (ch->current_length == 0) {
        if (ch->audlen == 0) {
            return;
        }
        ch->current_ptr    = ch->audlc;
        ch->current_length = ch->audlen;

        {
            static unsigned trace_count = 0u;
            if (trace_count < AUDIO_TRACE_LIMIT) {
                rigel_log_event_t event = {
                    RIGEL_LOG_EVENT_AUDIO_RELOAD,
                    "audio_reload",
                    { (rigel_u32)channel, ch->audlc, (rigel_u32)ch->audlen },
                    3u
                };
                rigel_log_event(&event);
                trace_count++;
            }
        }
    }

    ch->dma_word       = mem.read16(mem.opaque, ch->current_ptr & ~1u);
    ch->dma_word_ready = true;

    {
        static unsigned trace_count = 0u;
        if (trace_count < AUDIO_TRACE_LIMIT) {
            rigel_log_event_t event = {
                RIGEL_LOG_EVENT_AUDIO_FETCH,
                "audio_fetch",
                { (rigel_u32)channel, ch->current_ptr & ~1u,
                  (rigel_u32)ch->dma_word, (rigel_u32)ch->current_length },
                4u
            };
            rigel_log_event(&event);
            trace_count++;
        }
    }

    ch->current_ptr   += 2u;
    ch->current_length--;

    if (ch->current_length == 0) {
        {
            static unsigned trace_count = 0u;
            if (trace_count < AUDIO_TRACE_LIMIT) {
                rigel_log_event_t event = {
                    RIGEL_LOG_EVENT_AUDIO_IRQ,
                    "audio_irq",
                    { (rigel_u32)channel, (rigel_u32)irq_bits[channel] },
                    2u
                };
                rigel_log_event(&event);
                trace_count++;
            }
        }
        if (audio->irq.raise != NULL) {
            audio->irq.raise(audio->irq.opaque, irq_bits[channel]);
        }
    }
}

rigel_u32 audio_cycles_to_next_event(const audio_state_t *audio)
{
    rigel_u32 min = 0xFFFFFFFFu;
    int ch;

    if (audio == NULL) {
        return 0xFFFFFFFFu;
    }

    for (ch = 0; ch < RIGEL_PAULA_AUDIO_CHANNELS; ch++) {
        const rigel_audio_channel_t *c = &audio->ch[ch];
        if (audio_channel_dma_enabled(audio, ch) ||
            c->data_pending ||
            c->has_pending_lo ||
            c->dma_word_ready) {
            rigel_u32 next = c->period_counter;
            if (next == 0u) {
                next = c->audper != 0u ? c->audper : 1u;
            }
            if (next < min) {
                min = next;
            }
        }
    }

    return min;
}
