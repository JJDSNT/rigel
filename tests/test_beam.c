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

    return beam_cycles_until_line_end(&beam) == RIGEL_BEAM_DEFAULT_LINE_CLOCKS ? 0 : 1;
}
