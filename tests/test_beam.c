#include "agnus/beam.h"

int main(void)
{
    beam_state_t beam = { 0 };
    beam_reset(&beam);

    beam_step(&beam, 4);
    if (beam.hpos != 4 || beam.vpos != 0) {
        return 1;
    }

    beam_step(&beam, (rigel_u16)(RIGEL_BEAM_DEFAULT_LINE_CLOCKS - 4));
    if (beam.hpos != 0 || beam.vpos != 1) {
        return 1;
    }

    beam_reset(&beam);
    beam_step(&beam, (rigel_u16)(RIGEL_BEAM_DEFAULT_LINE_CLOCKS * RIGEL_BEAM_DEFAULT_FRAME_LINES));
    if (beam.hpos != 0 || beam.vpos != 0 || beam.frame_count != 1) {
        return 1;
    }

    /* LOL toggle: first line after reset is 228 CCKs (lol=1 toggled in) */
    beam_reset(&beam);
    beam.lol_toggle = 1u;
    beam_step(&beam, RIGEL_BEAM_DEFAULT_LINE_CLOCKS);
    if (beam.hpos != 0 || beam.vpos != 1 || beam.lol != 1u) {
        return 1;
    }

    /* LOF toggle: frame_count increments and lof toggles at end of first frame */
    beam_reset(&beam);
    beam.lof_toggle = 1u;
    beam_step(&beam, (rigel_u16)(RIGEL_BEAM_DEFAULT_LINE_CLOCKS * RIGEL_BEAM_DEFAULT_FRAME_LINES));
    if (beam.frame_count != 1 || beam.lof != 1u) {
        return 1;
    }

    return beam_cycles_until_line_end(&beam) == RIGEL_BEAM_DEFAULT_LINE_CLOCKS ? 0 : 1;
}
