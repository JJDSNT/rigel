#ifndef RIGEL_AUDIO_DOMAIN_H
#define RIGEL_AUDIO_DOMAIN_H

#include "paula/audio.h"
#include "rigel/rigel_custom.h"

bool rigel_audio_domain_owns_reg(rigel_u32 addr);
void rigel_audio_domain_reset(audio_state_t *audio);
void rigel_audio_domain_set_dmacon(audio_state_t *audio, rigel_u16 dmacon);
void rigel_audio_domain_step(audio_state_t *audio, rigel_u32 cycles);
void rigel_audio_domain_write_reg(audio_state_t *audio, rigel_u32 addr, rigel_u16 value);

#endif
