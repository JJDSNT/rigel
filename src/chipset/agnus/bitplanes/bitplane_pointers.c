#include "bitplane_pointers.h"
#include <stdint.h>

void bplpt_reset(bitplane_pointers_t *p)
{
    unsigned i;
    for (i = 0; i < BITPLANE_COUNT; i++)
        p->bplpt[i] = 0;
}

void bplpt_apply_modulo(bitplane_pointers_t *p, unsigned depth,
                        rigel_i16 bpl1mod, rigel_i16 bpl2mod)
{
    unsigned i;
    for (i = 0; i < depth && i < BITPLANE_COUNT; i++) {
        rigel_i16 mod = (i & 1u) ? bpl2mod : bpl1mod;
        p->bplpt[i] = (rigel_u32)((int32_t)p->bplpt[i] + (int32_t)mod) & 0x001FFFFEu;
    }
}

void bplpt_set_hi(bitplane_pointers_t *p, unsigned plane, rigel_u16 val)
{
    if (plane >= BITPLANE_COUNT) return;
    p->bplpt[plane] = (p->bplpt[plane] & 0x0000FFFFu) | ((rigel_u32)val << 16);
}

void bplpt_set_lo(bitplane_pointers_t *p, unsigned plane, rigel_u16 val)
{
    if (plane >= BITPLANE_COUNT) return;
    p->bplpt[plane] = (p->bplpt[plane] & 0xFFFF0000u) | (val & 0xFFFEu);
}

void bplpt_advance(bitplane_pointers_t *p, unsigned plane)
{
    if (plane >= BITPLANE_COUNT) return;
    p->bplpt[plane] = (p->bplpt[plane] + 2) & 0x001FFFFEu;
}
