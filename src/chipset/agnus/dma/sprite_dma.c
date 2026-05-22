#include "sprite_dma.h"

/* TODO(dma): implement sprite DMA fetch during hblank slots */

void sprite_dma_reset(sprite_dma_state_t *s)
{
    for (unsigned i = 0; i < SPRITE_DMA_CHANNELS; i++) {
        s->sp[i].ptr   = 0;
        s->sp[i].armed = false;
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

void sprite_dma_step(sprite_dma_state_t *s, unsigned sp,
                     rigel_chip_ram_if_t mem,
                     rigel_u16 *out_w0, rigel_u16 *out_w1)
{
    (void)s;
    (void)sp;
    (void)mem;
    *out_w0 = 0;
    *out_w1 = 0;
    /* TODO */
}
