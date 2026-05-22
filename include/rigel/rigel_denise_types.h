#ifndef RIGEL_DENISE_TYPES_H
#define RIGEL_DENISE_TYPES_H

#include "rigel_types.h"

typedef enum rigel_denise_mode_flags {
    RIGEL_DENISE_MODE_DUALPF = 1u << 0,
    RIGEL_DENISE_MODE_HAM    = 1u << 1,
    RIGEL_DENISE_MODE_EHB    = 1u << 2
} rigel_denise_mode_flags_t;

typedef struct rigel_denise_video_desc {
    rigel_u16 display_width;
    rigel_u16 display_height;
    rigel_u16 visible_x_start;
    rigel_u16 visible_x_stop;
    rigel_u16 visible_y_start;
    rigel_u16 visible_y_stop;
} rigel_denise_video_desc_t;

typedef struct rigel_denise_debug_state {
    rigel_u32 frame_counter;
    rigel_u32 scanline_counter;
    rigel_u16 active_mode_flags;
    rigel_u16 last_color_index;
} rigel_denise_debug_state_t;

#endif
