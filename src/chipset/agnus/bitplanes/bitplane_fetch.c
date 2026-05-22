#include "bitplane_fetch.h"

/* TODO(bitplanes): implement fetch step — read word at bplpt[plane], advance pointer */

void bitplane_fetch_reset(bitplane_fetch_state_t *f)
{
    for (unsigned i = 0; i < 6; i++)
        f->data[i] = 0;
}

void bitplane_fetch_step(bitplane_fetch_state_t *f,
                         bitplane_pointers_t *ptrs,
                         unsigned plane,
                         rigel_chip_ram_if_t mem)
{
    (void)f;
    (void)ptrs;
    (void)plane;
    (void)mem;
    /* TODO */
}

rigel_u16 bitplane_fetch_data(const bitplane_fetch_state_t *f, unsigned plane)
{
    if (plane >= 6) return 0;
    return f->data[plane];
}
