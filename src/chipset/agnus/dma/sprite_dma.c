#include "sprite_dma.h"

void sprite_dma_reset(sprite_dma_state_t *s)
{
    unsigned i;
    for (i = 0; i < SPRITE_DMA_CHANNELS; i++) {
        s->sp[i].ptr        = 0;
        s->sp[i].armed      = false;
        s->sp[i].fetch_ctrl = true;
        s->sp[i].vstart     = 0;
        s->sp[i].vstop      = 0;
        s->sp[i].w0         = 0;
    }
}

void sprite_dma_set_ptr_hi(sprite_dma_state_t *s, unsigned sp, rigel_u16 val)
{
    if (sp >= SPRITE_DMA_CHANNELS) return;
    s->sp[sp].ptr = (s->sp[sp].ptr & 0x0000FFFFu) | ((rigel_u32)val << 16);
}

void sprite_dma_set_ptr_lo(sprite_dma_state_t *s, unsigned sp, rigel_u16 val)
{
    if (sp >= SPRITE_DMA_CHANNELS) return;
    s->sp[sp].ptr = (s->sp[sp].ptr & 0xFFFF0000u) | val;
}

bool sprite_dma_slot(sprite_dma_state_t *s, unsigned sp,
                     rigel_u16 vpos, bool is_b,
                     rigel_chip_ram_if_t mem,
                     rigel_u16 *out_w0, rigel_u16 *out_w1,
                     bool *out_is_ctrl)
{
    sprite_dma_channel_t *ch;
    rigel_u16 w1;

    if (sp >= SPRITE_DMA_CHANNELS || mem.read16 == NULL) {
        if (is_b) { *out_w0 = 0; *out_w1 = 0; *out_is_ctrl = true; }
        return is_b;
    }

    ch = &s->sp[sp];

    if (!is_b) {
        /* A-slot: decide ctrl vs data for this line, latch first word */
        ch->fetch_ctrl = !ch->armed ||
                         vpos < ch->vstart ||
                         vpos >= ch->vstop;
        ch->w0 = mem.read16(mem.opaque, ch->ptr);
        ch->ptr += 2u;
        return false;
    }

    /* B-slot: latch second word and deliver the pair */
    w1 = mem.read16(mem.opaque, ch->ptr);
    ch->ptr += 2u;

    *out_w0      = ch->w0;
    *out_w1      = w1;
    *out_is_ctrl = ch->fetch_ctrl;

    if (ch->fetch_ctrl) {
        /* Extract vstart/vstop from pos (w0) and ctl (w1) */
        ch->vstart = (rigel_u16)(((ch->w0 >> 8) & 0xFFu) | (((w1 >> 2) & 1u) << 8));
        ch->vstop  = (rigel_u16)(((w1 >> 8) & 0xFFu) | (((w1 >> 1) & 1u) << 8));
        ch->armed  = true;
    }

    return true;
}
