#include "vblank.h"

rigel_u32 agnus_cycles_to_vertb(const beam_state_t *beam)
{
    rigel_u16 hpos        = beam->hpos;
    rigel_u16 vpos        = beam->vpos;
    rigel_u16 line_clocks = beam->line_clocks;
    rigel_u16 frame_lines = beam->frame_lines;

    if (agnus_is_vertb_position(hpos, vpos))
        return 0;

    /* Cycles remaining in the current line after hpos */
    rigel_u32 cycles = (rigel_u32)(line_clocks - hpos);

    /* Lines remaining until vpos wraps to 0 */
    rigel_u32 lines_to_wrap = (vpos < frame_lines)
        ? (rigel_u32)(frame_lines - vpos - 1u)
        : 0u;

    /* Add lines until vpos=0, then AGNUS_VERTB_HPOS clocks into line 0 */
    cycles += lines_to_wrap * line_clocks;
    cycles += AGNUS_VERTB_HPOS;

    return cycles;
}
