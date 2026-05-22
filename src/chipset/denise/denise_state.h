#ifndef RIGEL_DENISE_STATE_H
#define RIGEL_DENISE_STATE_H

#include "rigel/rigel_denise_types.h"
#include "rigel/rigel_types.h"
#include "agnus/beam.h"

enum {
    RIGEL_DENISE_MAX_SCANLINE_PIXELS = 1024
};

typedef struct rigel_denise_register_file {
    rigel_u16 color[32];
    rigel_u16 bplcon0;
    rigel_u16 bplcon1;
    rigel_u16 bplcon2;
    rigel_u16 diwstrt;
    rigel_u16 diwstop;
} rigel_denise_register_file_t;

typedef struct rigel_denise_palette_state {
    rigel_u32 rgb32[32];
} rigel_denise_palette_state_t;

typedef struct rigel_denise_sprite_state {
    rigel_u16 active_mask;
    rigel_u16 attached_mask;
} rigel_denise_sprite_state_t;

typedef struct rigel_denise_video_state {
    rigel_u16 width;
    rigel_u16 height;
    rigel_u16 visible_x_start;
    rigel_u16 visible_x_stop;
    rigel_u16 visible_y_start;
    rigel_u16 visible_y_stop;
} rigel_denise_video_state_t;

typedef struct rigel_denise_output_state {
    rigel_u64 frame_counter;
    rigel_u16 beam_hpos;
    rigel_u16 beam_vpos;
    rigel_u16 current_scanline;
    rigel_u16 current_pixel;
    rigel_u16 scanline_width;
    rigel_u32 last_rgb;
    rigel_u32 scanline_rgba[RIGEL_DENISE_MAX_SCANLINE_PIXELS];
    bool visible_scanline;
    bool scanline_dirty;
    bool frame_dirty;
} rigel_denise_output_state_t;

typedef struct RigelDenise {
    rigel_denise_register_file_t regs;
    rigel_denise_palette_state_t palette;
    rigel_denise_sprite_state_t sprites;
    rigel_denise_video_state_t video;
    rigel_denise_output_state_t output;
    rigel_denise_debug_state_t debug;
} RigelDenise;

void rigel_denise_reset(RigelDenise *denise);
void rigel_denise_step(RigelDenise *denise, const beam_state_t *beam, rigel_u32 cycles);

#endif
