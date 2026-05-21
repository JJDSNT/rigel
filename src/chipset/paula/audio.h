#ifndef RIGEL_PAULA_AUDIO_H
#define RIGEL_PAULA_AUDIO_H

#include <stdbool.h>

#include "rigel/rigel_types.h"

enum {
    RIGEL_PAULA_AUDIO_CHANNELS = 4
};

typedef void (*rigel_audio_irq_raise_fn)(void *opaque, rigel_u16 mask);

typedef struct rigel_audio_irq_sink {
    void *opaque;
    rigel_audio_irq_raise_fn raise;
} rigel_audio_irq_sink_t;

typedef struct rigel_audio_channel {
    rigel_u16 audlen;
    rigel_u16 audper;
    rigel_u16 audvol;
    rigel_u16 auddat;
    rigel_u32 audlc;
    rigel_u32 current_ptr;
    rigel_u16 current_length;
    rigel_u16 period_counter;
    int16_t current_sample;
    bool dma_enabled;
    bool data_pending;
    rigel_u8 pending_lo;
    bool has_pending_lo;
    rigel_u16 dma_word;
    bool dma_word_ready;
} rigel_audio_channel_t;

typedef struct audio_state {
    rigel_audio_channel_t ch[RIGEL_PAULA_AUDIO_CHANNELS];
    int16_t mixed_left;
    int16_t mixed_right;
    rigel_u16 dmacon;
    rigel_audio_irq_sink_t irq;
} audio_state_t;

void audio_reset(audio_state_t *audio);
void audio_set_irq_sink(audio_state_t *audio, rigel_audio_irq_sink_t sink);
void audio_set_dmacon(audio_state_t *audio, rigel_u16 dmacon);
void audio_step(audio_state_t *audio, rigel_u32 cycles);
void audio_write_lch(audio_state_t *audio, int channel, rigel_u16 value);
void audio_write_lcl(audio_state_t *audio, int channel, rigel_u16 value);
void audio_write_len(audio_state_t *audio, int channel, rigel_u16 value);
void audio_write_per(audio_state_t *audio, int channel, rigel_u16 value);
void audio_write_vol(audio_state_t *audio, int channel, rigel_u16 value);
void audio_write_dat(audio_state_t *audio, int channel, rigel_u16 value);
int16_t audio_left(const audio_state_t *audio);
int16_t audio_right(const audio_state_t *audio);

#endif
