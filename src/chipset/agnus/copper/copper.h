#ifndef RIGEL_AGNUS_COPPER_H
#define RIGEL_AGNUS_COPPER_H

#include <stdbool.h>
#include "rigel/rigel_types.h"

typedef struct copper_state {
    rigel_u32 cop1lc;
    rigel_u32 cop2lc;
    rigel_u32 program_counter;
    rigel_u16 wait_hpos;
    rigel_u16 wait_vpos;
    rigel_u16 wait_vpmask;   /* VP mask from IR2[15:8]; 0xFF = compare all 8 vpos bits */
    rigel_u16 wait_hpmask;   /* HP mask from IR2[7:1];  0xFE = compare all 7 hpos bits */
    bool waiting;
    bool enabled;
    bool triggered;
    bool fetch_pending;
} copper_state_t;

/* Masked beam comparison used by both WAIT (domain step) and SKIP (service decode).
 * Returns true when the beam is at or past the given wait position after masking. */
static inline bool copper_beam_cmp(
    rigel_u16 vpos, rigel_u16 hpos,
    rigel_u16 wait_vpos, rigel_u16 wait_hpos,
    rigel_u16 vpmask, rigel_u16 hpmask)
{
    rigel_u16 beam16 = (rigel_u16)(((rigel_u16)(vpos & vpmask) << 8) | (hpos & hpmask & 0xFEu));
    rigel_u16 tgt16  = (rigel_u16)(((rigel_u16)(wait_vpos & vpmask) << 8) | (wait_hpos & hpmask & 0xFEu));
    return beam16 >= tgt16;
}

void copper_reset(copper_state_t *copper);
void copper_set_pointer_hi(rigel_u32 *ptr, rigel_u16 value);
void copper_set_pointer_lo(rigel_u32 *ptr, rigel_u16 value);

#endif
