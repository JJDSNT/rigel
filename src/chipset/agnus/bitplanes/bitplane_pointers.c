#include "bitplane_pointers.h"

void bplpt_reset(bitplane_pointers_t *p)
{
    for (unsigned i = 0; i < BITPLANE_COUNT; i++)
        p->bplpt[i] = 0;
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
