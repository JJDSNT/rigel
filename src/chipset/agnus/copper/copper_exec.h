#ifndef RIGEL_AGNUS_COPPER_EXEC_H
#define RIGEL_AGNUS_COPPER_EXEC_H

#include <stdbool.h>
#include "rigel/rigel_types.h"

/* Copper instruction execution — MOVE and SKIP.
 *
 * A copper instruction is two words (IR1, IR2):
 *   MOVE:  IR1 bit 0 = 0; IR1 = dest register offset; IR2 = value
 *   WAIT:  IR1 bit 0 = 1; IR2 bit 0 = 0
 *   SKIP:  IR1 bit 0 = 1; IR2 bit 0 = 1 */

typedef struct RigelContext RigelContext;

/* Execute a MOVE instruction — writes IR2 to the custom register at IR1.
 * Respects COPCON danger bit for low-register access. */
void copper_exec_move(RigelContext *ctx, rigel_u16 ir1, rigel_u16 ir2);

/* Test whether a SKIP condition (IR1, IR2) is already satisfied.
 * Returns true if the copper should skip the next instruction. */
bool copper_exec_skip_test(rigel_u16 ir1, rigel_u16 ir2,
                           rigel_u16 hpos, rigel_u16 vpos);

#endif
