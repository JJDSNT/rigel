#include "agnus/beam.h"
#include "agnus/timing/vblank.h"

#include <stddef.h>

static rigel_u16 beam_line_hmax(const beam_state_t *beam)
{
    return (rigel_u16)(beam->line_clocks + (beam->lol ? 1u : 0u));
}

static rigel_u16 beam_frame_vmax(const beam_state_t *beam)
{
    return (rigel_u16)(beam->frame_lines + (beam->lof ? 1u : 0u));
}

static void beam_advance_line(beam_state_t *beam)
{
    beam->hpos = 0;
    beam->vpos = (rigel_u16)(beam->vpos + 1u);

    if (beam->lol_toggle)
        beam->lol ^= 1u;
    else
        beam->lol = 0u;

    if (beam->vpos >= beam_frame_vmax(beam)) {
        beam->vpos = 0;
        beam->frame_count += 1u;

        if (beam->lof_toggle)
            beam->lof ^= 1u;
        else
            beam->lof = 0u;

        if (!beam->lol_toggle)
            beam->lol = 0u;
    }
}

void beam_reset(beam_state_t *beam)
{
    if (beam == NULL) {
        return;
    }

    beam->hpos = 0;
    beam->vpos = 0;
    beam->line_clocks = RIGEL_BEAM_DEFAULT_LINE_CLOCKS;
    beam->frame_lines = RIGEL_BEAM_DEFAULT_FRAME_LINES;
    beam->visible_y_start = RIGEL_BEAM_DEFAULT_VISIBLE_Y_START;
    beam->visible_y_stop = RIGEL_BEAM_DEFAULT_VISIBLE_Y_STOP;
    beam->lof = 0;
    beam->lof_toggle = 0;
    beam->lol = 0;
    beam->lol_toggle = 0;
    beam->frame_count = 0;
}

void beam_step(beam_state_t *beam, rigel_u16 clocks)
{
    if (beam == NULL) {
        return;
    }

    if (beam->line_clocks == 0 || beam->frame_lines == 0) {
        beam_reset(beam);
    }

    while (clocks > 0u) {
        rigel_u16 hmax = beam_line_hmax(beam);
        rigel_u16 remaining = (rigel_u16)(hmax - beam->hpos);

        if (remaining == 0u) {
            beam_advance_line(beam);
            continue;
        }

        if (clocks < remaining) {
            beam->hpos = (rigel_u16)(beam->hpos + clocks);
            clocks = 0u;
        } else {
            beam->hpos = (rigel_u16)(beam->hpos + remaining);
            clocks = (rigel_u16)(clocks - remaining);
            if (beam->hpos >= hmax) {
                beam_advance_line(beam);
            }
        }
    }
}

bool beam_in_vblank(const beam_state_t *beam)
{
    if (beam == NULL) {
        return false;
    }

    return agnus_in_vbl_zone(beam->vpos);
}

bool beam_in_visible_area(const beam_state_t *beam)
{
    if (beam == NULL) {
        return false;
    }

    return beam->vpos >= beam->visible_y_start && beam->vpos <= beam->visible_y_stop;
}

rigel_u16 beam_cycles_until_line_end(const beam_state_t *beam)
{
    if (beam == NULL || beam->line_clocks == 0) {
        return 0;
    }

    return (rigel_u16)(beam_line_hmax(beam) - beam->hpos);
}
