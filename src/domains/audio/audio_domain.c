#include "domains/audio/audio_domain.h"

static bool rigel_audio_domain_decode_reg(rigel_u32 addr, int *channel, rigel_u32 *offset)
{
    rigel_u32 slot;

    if (addr < RIGEL_REG_AUD0LCH || addr > RIGEL_REG_AUD3DAT) {
        return false;
    }

    slot = addr - RIGEL_REG_AUD0LCH;
    if ((slot & 0x0fu) > 0x0au) {
        return false;
    }

    if (channel != NULL) {
        *channel = (int)(slot >> 4);
    }
    if (offset != NULL) {
        *offset = slot & 0x0fu;
    }

    return true;
}

bool rigel_audio_domain_owns_reg(rigel_u32 addr)
{
    return rigel_audio_domain_decode_reg(addr, NULL, NULL);
}

void rigel_audio_domain_reset(audio_state_t *audio)
{
    audio_reset(audio);
}

void rigel_audio_domain_set_dmacon(audio_state_t *audio, rigel_u16 dmacon)
{
    audio_set_dmacon(audio, dmacon);
}

void rigel_audio_domain_step(audio_state_t *audio, rigel_u32 cycles)
{
    audio_step(audio, cycles);
}

void rigel_audio_domain_write_reg(audio_state_t *audio, rigel_u32 addr, rigel_u16 value)
{
    int channel;
    rigel_u32 offset;

    if (!rigel_audio_domain_decode_reg(addr, &channel, &offset)) {
        return;
    }

    switch (offset) {
    case 0x0:
        audio_write_lch(audio, channel, value);
        return;
    case 0x2:
        audio_write_lcl(audio, channel, value);
        return;
    case 0x4:
        audio_write_len(audio, channel, value);
        return;
    case 0x6:
        audio_write_per(audio, channel, value);
        return;
    case 0x8:
        audio_write_vol(audio, channel, value);
        return;
    case 0x0a:
        audio_write_dat(audio, channel, value);
        return;
    default:
        return;
    }
}
