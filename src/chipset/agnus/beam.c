#include "agnus/beam.h"
#include "agnus/timing/vblank.h"

#include <stddef.h>

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
    beam->frame_count = 0;
}

void beam_step(beam_state_t *beam, rigel_u16 clocks)
{
    rigel_u32 total;
    rigel_u32 line_advance;
    rigel_u32 frame_advance;

    if (beam == NULL) {
        return;
    }

    if (beam->line_clocks == 0 || beam->frame_lines == 0) {
        beam_reset(beam);
    }

    total = (rigel_u32)beam->hpos + (rigel_u32)clocks;
    line_advance = total / beam->line_clocks;
    beam->hpos = (rigel_u16)(total % beam->line_clocks);

    if (line_advance == 0) {
        return;
    }

    total = (rigel_u32)beam->vpos + line_advance;
    frame_advance = total / beam->frame_lines;
    beam->vpos = (rigel_u16)(total % beam->frame_lines);
    beam->frame_count += frame_advance;
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

    return (rigel_u16)(beam->line_clocks - beam->hpos);
}
