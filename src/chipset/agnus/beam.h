#ifndef BEAM_H
#define BEAM_H

#include "rigel/rigel_types.h"

typedef struct beam_state {
    rigel_u16 hpos;
    rigel_u16 vpos;
    rigel_u16 line_clocks;
    rigel_u16 frame_lines;
    rigel_u16 visible_y_start;
    rigel_u16 visible_y_stop;
    /* Interlace state — 0 in progressive (non-interlaced) mode */
    rigel_u8 lof;        /* long odd frame: current frame has frame_lines+1 lines */
    rigel_u8 lof_toggle; /* 1 = toggle lof at each frame boundary */
    rigel_u8 lol;        /* long odd line: current line has line_clocks+1 CCKs */
    rigel_u8 lol_toggle; /* 1 = toggle lol at each line boundary */
    rigel_u64 frame_count;
} beam_state_t;

enum {
    RIGEL_BEAM_DEFAULT_LINE_CLOCKS = 227,
    RIGEL_BEAM_DEFAULT_FRAME_LINES = 262,
    RIGEL_BEAM_DEFAULT_VISIBLE_Y_START = 26,
    RIGEL_BEAM_DEFAULT_VISIBLE_Y_STOP = 255
};

void beam_reset(beam_state_t *beam);
void beam_step(beam_state_t *beam, rigel_u16 clocks);
bool beam_in_vblank(const beam_state_t *beam);
bool beam_in_visible_area(const beam_state_t *beam);
rigel_u16 beam_cycles_until_line_end(const beam_state_t *beam);

#endif
