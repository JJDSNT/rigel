#include "sprites.h"

void denise_sprites_reset(denise_sprites_state_t *s)
{
    for (unsigned i = 0; i < DENISE_SPRITE_COUNT; i++) {
        s->sp[i].pos    = 0;
        s->sp[i].ctl    = 0;
        s->sp[i].data   = 0;
        s->sp[i].datb   = 0;
        s->sp[i].armed  = false;
        s->sp[i].visible = false;
    }
    s->active_mask   = 0;
    s->attached_mask = 0;
}

void denise_sprite_receive_ctrl(denise_sprites_state_t *s, unsigned sp,
                                rigel_u16 pos, rigel_u16 ctl)
{
    if (sp >= DENISE_SPRITE_COUNT) return;
    s->sp[sp].pos   = pos;
    s->sp[sp].ctl   = ctl;
    s->sp[sp].armed = true;
    /* Bit 7 of CTL: attach bit — only valid for odd sprites */
    if (sp & 1u) {
        if (ctl & (1u << 7))
            s->attached_mask |= (rigel_u16)(1u << sp);
        else
            s->attached_mask &= (rigel_u16)~(1u << sp);
    }
}

void denise_sprite_receive_data(denise_sprites_state_t *s, unsigned sp,
                                rigel_u16 data, rigel_u16 datb)
{
    if (sp >= DENISE_SPRITE_COUNT) return;
    s->sp[sp].data = data;
    s->sp[sp].datb = datb;
}

uint8_t denise_sprite_pixel(const denise_sprites_state_t *s, unsigned sp, rigel_u16 hpos)
{
    const denise_sprite_t *ch;
    rigel_u16 hstart;
    unsigned offset, shift, data_bit, datb_bit;

    if (sp >= DENISE_SPRITE_COUNT) return 0;
    ch = &s->sp[sp];
    if (!ch->armed) return 0;

    hstart = denise_sprite_hstart(ch);
    if (hpos < hstart || hpos >= hstart + 16u) return 0;

    offset   = (unsigned)(hpos - hstart);
    shift    = 15u - offset;
    data_bit = (ch->data >> shift) & 1u;
    datb_bit = (ch->datb >> shift) & 1u;
    return (uint8_t)((datb_bit << 1) | data_bit);
}

bool denise_sprite_is_attached(const denise_sprites_state_t *s, unsigned odd_sp)
{
    if (odd_sp & 1u) return (s->attached_mask >> odd_sp) & 1u;
    return false;
}

rigel_u16 denise_sprite_hstart(const denise_sprite_t *sp)
{
    /* HSTART[8:1] from pos[7:0], HSTART[0] from ctl[0] */
    return (rigel_u16)(((sp->pos & 0xFFu) << 1) | (sp->ctl & 1u));
}

rigel_u16 denise_sprite_vstart(const denise_sprite_t *sp)
{
    /* VSTART[7:0] from pos[15:8], VSTART[8] from ctl[2] */
    return (rigel_u16)(((sp->pos >> 8) & 0xFFu) | (((sp->ctl >> 2) & 1u) << 8));
}

rigel_u16 denise_sprite_vstop(const denise_sprite_t *sp)
{
    /* VSTOP[7:0] from ctl[15:8], VSTOP[8] from ctl[1] */
    return (rigel_u16)(((sp->ctl >> 8) & 0xFFu) | (((sp->ctl >> 1) & 1u) << 8));
}
