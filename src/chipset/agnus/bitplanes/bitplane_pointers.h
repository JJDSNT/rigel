#ifndef RIGEL_AGNUS_BITPLANE_POINTERS_H
#define RIGEL_AGNUS_BITPLANE_POINTERS_H

#include "rigel/rigel_types.h"

/* Bitplane DMA pointers — BPL1PT through BPL6PT.
 * Agnus advances these as it fetches; the host can reload them each frame
 * via MMIO writes to BPLxPTH/BPLxPTL.
 *
 * bplmod[0] = BPL1MOD — applied to odd bitplanes (BPL1,3,5 = indices 0,2,4)
 * bplmod[1] = BPL2MOD — applied to even bitplanes (BPL2,4,6 = indices 1,3,5)
 * at the end of each horizontal line (in horizontal blank). */

#define BITPLANE_COUNT 6

typedef struct bitplane_pointers {
    rigel_u32 bplpt[BITPLANE_COUNT];
    rigel_i16 bplmod[2];   /* signed — negative modulo scrolls backwards */
} bitplane_pointers_t;

void bplpt_reset(bitplane_pointers_t *p);
void bplpt_set_hi(bitplane_pointers_t *p, unsigned plane, rigel_u16 val);
void bplpt_set_lo(bitplane_pointers_t *p, unsigned plane, rigel_u16 val);

/* Advance pointer by two bytes after a fetch word */
void bplpt_advance(bitplane_pointers_t *p, unsigned plane);

/* Store BPL1MOD (idx=0) or BPL2MOD (idx=1) */
void bplpt_set_modulo(bitplane_pointers_t *p, unsigned idx, rigel_u16 val);

/* Add the appropriate modulo to each active plane at end of line.
 * depth = number of active planes from BPLCON0[14:12]. */
void bplpt_apply_modulo(bitplane_pointers_t *p, unsigned depth);

#endif
