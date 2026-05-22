#ifndef RIGEL_AGNUS_BITPLANE_POINTERS_H
#define RIGEL_AGNUS_BITPLANE_POINTERS_H

#include "rigel/rigel_types.h"

/* Bitplane DMA pointers — BPL1PT through BPL6PT.
 * Agnus advances these as it fetches; the host can reload them each frame
 * via MMIO writes to BPLxPTH/BPLxPTL. */

#define BITPLANE_COUNT 6

typedef struct bitplane_pointers {
    rigel_u32 bplpt[BITPLANE_COUNT];
} bitplane_pointers_t;

void bplpt_reset(bitplane_pointers_t *p);
void bplpt_set_hi(bitplane_pointers_t *p, unsigned plane, rigel_u16 val);
void bplpt_set_lo(bitplane_pointers_t *p, unsigned plane, rigel_u16 val);

/* Advance pointer by two bytes after a fetch word */
void bplpt_advance(bitplane_pointers_t *p, unsigned plane);

#endif
