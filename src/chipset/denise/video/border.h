#ifndef RIGEL_DENISE_BORDER_H
#define RIGEL_DENISE_BORDER_H

#include <stdbool.h>
#include "rigel/rigel_types.h"

/* Border output — pixels outside the display window receive COLOR00.
 *
 * The border colour is always palette index 0 (COLOR00). On hardware,
 * the border is drawn both horizontally (between DIW edges and the blanking
 * intervals) and vertically (lines outside VSTART..VSTOP).
 *
 * This module answers "are we in the border right now?" given beam position
 * and the active display window settings. */

typedef struct border_state {
    rigel_u16 diwstrt;
    rigel_u16 diwstop;
} border_state_t;

void border_reset(border_state_t *b);
void border_set_diwstrt(border_state_t *b, rigel_u16 val);
void border_set_diwstop(border_state_t *b, rigel_u16 val);

/* Returns true if (hpos, vpos) is in the border (outside the display window). */
bool border_active(const border_state_t *b, rigel_u16 hpos, rigel_u16 vpos);

#endif
