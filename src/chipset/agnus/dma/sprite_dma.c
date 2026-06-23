#include "sprite_dma.h"

#if RIGEL_ENABLE_STDLIB_ENV
#include <stdio.h>
#include <stdlib.h>

static bool sprite_dma_trace_enabled(void)
{
    static int enabled = -1;

    if (enabled < 0) {
        const char *env = getenv("RIGEL_SPRITE_DMA_TRACE");
        enabled = (env != NULL && env[0] != '\0' && env[0] != '0') ? 1 : 0;
    }

    return enabled != 0;
}
#endif

void sprite_dma_reset(sprite_dma_state_t *s)
{
    unsigned i;
    for (i = 0; i < SPRITE_DMA_CHANNELS; i++) {
        s->sp[i].base_ptr   = 0;
        s->sp[i].ptr        = 0;
        s->sp[i].armed      = false;
        s->sp[i].terminated = false;
        s->sp[i].fetch_ctrl = true;
        s->sp[i].vstart     = 0;
        s->sp[i].vstop      = 0;
        s->sp[i].w0         = 0;
    }
}

void sprite_dma_set_ptr_hi(sprite_dma_state_t *s, unsigned sp, rigel_u16 val)
{
    if (sp >= SPRITE_DMA_CHANNELS) return;
    s->sp[sp].base_ptr = (s->sp[sp].base_ptr & 0x0000FFFFu) | ((rigel_u32)val << 16);
    s->sp[sp].ptr = s->sp[sp].base_ptr;
    s->sp[sp].armed = false;
    s->sp[sp].terminated = false;
    s->sp[sp].fetch_ctrl = true;
}

void sprite_dma_set_ptr_lo(sprite_dma_state_t *s, unsigned sp, rigel_u16 val)
{
    if (sp >= SPRITE_DMA_CHANNELS) return;
    s->sp[sp].base_ptr = (s->sp[sp].base_ptr & 0xFFFF0000u) | val;
    s->sp[sp].ptr = s->sp[sp].base_ptr;
    s->sp[sp].armed = false;
    s->sp[sp].terminated = false;
    s->sp[sp].fetch_ctrl = true;
}

void sprite_dma_frame_start(sprite_dma_state_t *s)
{
    unsigned i;

    if (s == NULL) {
        return;
    }

    for (i = 0; i < SPRITE_DMA_CHANNELS; i++) {
        s->sp[i].ptr = s->sp[i].base_ptr;
        s->sp[i].armed = false;
        s->sp[i].terminated = false;
        s->sp[i].fetch_ctrl = true;
        s->sp[i].vstart = 0;
        s->sp[i].vstop = 0;
        s->sp[i].w0 = 0;
    }
}

bool sprite_dma_slot(sprite_dma_state_t *s, unsigned sp,
                     rigel_u16 vpos, bool is_b,
                     rigel_chip_ram_if_t mem,
                     rigel_u16 *out_w0, rigel_u16 *out_w1,
                     bool *out_is_ctrl)
{
    sprite_dma_channel_t *ch;
    rigel_u16 w1;
    bool idle;

    if (sp >= SPRITE_DMA_CHANNELS || mem.read16 == NULL) {
        if (is_b) { *out_w0 = 0; *out_w1 = 0; *out_is_ctrl = true; }
        return is_b;
    }

    ch = &s->sp[sp];

    if (ch->terminated) {
        return false;
    }

    /* Between the initial control fetch and VSTART the channel is idle on
     * real hardware: no DMA cycle happens and the pointer does not move.
     * Fetching resumes at VSTART (data words, one pair per line) or VSTOP
     * (a new control pair, allowing chained sprite bands). Without this
     * gate, every idle line between frame start and VSTART was incorrectly
     * treated as a real fetch, racing the pointer past the sprite's actual
     * image data before the beam ever reached it — so DATA/DATB ended up
     * reading unrelated chip RAM and the sprite never showed anything. */
    idle = ch->armed && vpos < ch->vstart;

    if (!is_b) {
        /* A-slot: decide ctrl vs data for this line, latch first word */
        if (idle)
            return false;
        ch->fetch_ctrl = !ch->armed || vpos >= ch->vstop;
        ch->w0 = mem.read16(mem.opaque, ch->ptr);
        ch->ptr += 2u;
        return false;
    }

    if (idle)
        return false;

    /* B-slot: latch second word and deliver the pair */
    w1 = mem.read16(mem.opaque, ch->ptr);
    ch->ptr += 2u;

    *out_w0      = ch->w0;
    *out_w1      = w1;
    *out_is_ctrl = ch->fetch_ctrl;

    if (ch->fetch_ctrl) {
        if (ch->w0 == 0u && w1 == 0u) {
            ch->armed = false;
            ch->terminated = true;
            ch->fetch_ctrl = true;
            return false;
        }

        /* Extract vstart/vstop from pos (w0) and ctl (w1) */
        ch->vstart = (rigel_u16)(((ch->w0 >> 8) & 0xFFu) | (((w1 >> 2) & 1u) << 8));
        ch->vstop  = (rigel_u16)(((w1 >> 8) & 0xFFu) | (((w1 >> 1) & 1u) << 8));
        ch->armed  = true;
    }

#if RIGEL_ENABLE_STDLIB_ENV
    if (sp == 0u && sprite_dma_trace_enabled()) {
        fprintf(stderr,
                "[SPRITE0-DMA] vpos=%u ptr=%06x ctrl=%d w0=%04x w1=%04x"
                " armed=%d vstart=%u vstop=%u\n",
                (unsigned)vpos, (unsigned)ch->ptr, (int)ch->fetch_ctrl,
                (unsigned)ch->w0, (unsigned)w1, (int)ch->armed,
                (unsigned)ch->vstart, (unsigned)ch->vstop);
    }
#endif

    return true;
}
