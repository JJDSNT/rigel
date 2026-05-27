#ifndef RIGEL_DENISE_FRAMEBUFFER_H
#define RIGEL_DENISE_FRAMEBUFFER_H

#include "denise/denise_state.h"

void rigel_denise_framebuffer_reset(rigel_denise_output_state_t *output);
void rigel_denise_framebuffer_sync_from_beam(RigelDenise *denise, const beam_state_t *beam);
void rigel_denise_framebuffer_set_target(rigel_denise_output_state_t *output,
                                         const rigel_framebuffer_target_t *target);

#endif
