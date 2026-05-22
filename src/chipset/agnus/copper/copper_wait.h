#ifndef RIGEL_AGNUS_COPPER_WAIT_H
#define RIGEL_AGNUS_COPPER_WAIT_H

#include <stdbool.h>
#include "rigel/rigel_types.h"
#include "agnus/copper/copper.h"

/* Copper WAIT instruction — stalls the copper until beam >= (vpos, hpos).
 *
 * IR1 encodes the target beam position:
 *   IR1[15:8] = VP (vertical position to compare)
 *   IR1[7:1]  = HP (horizontal position to compare, bits[8:1])
 *   IR1[0]    = 1  (marks this as WAIT/SKIP, not MOVE)
 * IR2 encodes the comparison mask and blitter-disable bit. */

/* Arms the copper to wait for the given beam position.
 * Stores the extracted VP/HP in `copper` and sets `waiting = true`. */
void copper_wait_arm(copper_state_t *copper, rigel_u16 ir1, rigel_u16 ir2);

/* Returns true if the armed wait condition is satisfied by (hpos, vpos). */
bool copper_wait_satisfied(const copper_state_t *copper,
                           rigel_u16 hpos, rigel_u16 vpos);

#endif
