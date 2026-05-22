#ifndef RIGEL_DENISE_DUMPS_H
#define RIGEL_DENISE_DUMPS_H

#include "denise/denise_state.h"

/* Denise register and state dump — for debugging and test assertions.
 * All functions write to stderr and are safe to call at any time. */

void denise_dump_regs(const RigelDenise *denise);
void denise_dump_palette(const RigelDenise *denise);
void denise_dump_sprites(const RigelDenise *denise);
void denise_dump_video(const RigelDenise *denise);

#endif
