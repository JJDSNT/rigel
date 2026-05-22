#ifndef RIGEL_AGNUS_BITPLANE_FETCH_H
#define RIGEL_AGNUS_BITPLANE_FETCH_H

#include "rigel/rigel_types.h"
#include "bitplane_pointers.h"

/* Bitplane DMA fetch — drives BPLxPT pointers through chip RAM during the
 * display window, feeding data words to Denise in real time.
 *
 * Fetch timing is determined by DDFSTRT/DDFSTOP and the number of active
 * planes (BPLCON0 BPU field). Each fetch advances the relevant BPLxPT. */

typedef struct bitplane_fetch_state {
    rigel_u16 data[6];  /* last fetched word per plane, pending Denise delivery */
} bitplane_fetch_state_t;

void bitplane_fetch_reset(bitplane_fetch_state_t *f);

/* Called each time a bitplane DMA slot is granted.
 * `plane` is 0-based. Advances the pointer and stores the fetched word. */
void bitplane_fetch_step(bitplane_fetch_state_t *f,
                         bitplane_pointers_t *ptrs,
                         unsigned plane,
                         rigel_chip_ram_if_t mem);

/* Returns the fetched data word for `plane` (for forwarding to Denise). */
rigel_u16 bitplane_fetch_data(const bitplane_fetch_state_t *f, unsigned plane);

#endif
