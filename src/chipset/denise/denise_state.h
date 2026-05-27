#ifndef RIGEL_DENISE_STATE_H
#define RIGEL_DENISE_STATE_H

#include "rigel/rigel_denise_types.h"
#include "rigel/rigel_types.h"
#include "agnus/agnus_config.h"
#include "agnus/beam.h"
#include "denise/sprites/collisions.h"
#include "denise/sprites/sprites.h"

enum {
    RIGEL_DENISE_MAX_SCANLINE_PIXELS = 1024,
    RIGEL_DENISE_MAX_PLANE_WORDS     = 64,
    RIGEL_DENISE_MAX_LINES           = 312   /* PAL raster height */
};

typedef struct rigel_denise_register_file {
    rigel_u16 color[32];
    rigel_u16 bplcon0;
    rigel_u16 bplcon1;
    rigel_u16 bplcon2;
    rigel_u16 bplcon3;
    rigel_u16 diwstrt;
    rigel_u16 diwstop;
    rigel_u16 diwhigh;
} rigel_denise_register_file_t;

typedef struct rigel_denise_palette_state {
    rigel_u32 rgb32[32];
    rigel_u16 rgb565[32];
} rigel_denise_palette_state_t;

/* Full per-sprite state lives in denise_sprites_state_t (from sprites/sprites.h). */

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
    rigel_u16 scanline_rgb565[RIGEL_DENISE_MAX_SCANLINE_PIXELS];
    /* [0] and [1] are back/front buffers; front_idx is the completed frame.
     * Denise writes to frame_rgba[1^front_idx]; host reads frame_rgba[front_idx].
     * Swap at frame boundary: front_idx ^= 1. */
    rigel_u32 frame_rgba[2][RIGEL_DENISE_MAX_LINES][RIGEL_DENISE_MAX_SCANLINE_PIXELS];
    rigel_u16 frame_rgb565[2][RIGEL_DENISE_MAX_LINES][RIGEL_DENISE_MAX_SCANLINE_PIXELS];
    rigel_u8  front_idx;
    bool visible_scanline;
    bool scanline_dirty;
    bool frame_dirty;
    /* Bitplane line buffer — words fetched per plane for the current scanline */
    rigel_u16 plane_words[6][RIGEL_DENISE_MAX_PLANE_WORDS];
    rigel_u16 plane_word_count;
    /* DDF start in lores pixel units (= CCKs); set by Agnus on DDFSTRT writes.
     * Used by compositor to place bitplane words at absolute scanline positions. */
    rigel_u16 ddfstrt_lores;
    /* Frame metadata: pending = accumulating for current frame;
     * completed = snapshotted at frame boundary for the host to read. */
    rigel_u32 pending_flags;
    rigel_u64 pending_dirty[5];
    rigel_u32 completed_flags;
    rigel_u64 completed_dirty[5];
    bool has_write_target;
    rigel_framebuffer_target_t write_target;
} rigel_denise_output_state_t;

typedef struct RigelDenise {
    agnus_chip_rev_t chip_rev;
    rigel_denise_register_file_t regs;
    rigel_denise_palette_state_t palette;
    denise_sprites_state_t sprites;
    collision_state_t coll;
    rigel_denise_video_state_t video;
    rigel_denise_output_state_t output;
    rigel_denise_debug_state_t debug;
} RigelDenise;

void rigel_denise_reset(RigelDenise *denise);
void rigel_denise_set_chip_rev(RigelDenise *denise, agnus_chip_rev_t rev);
void rigel_denise_step(RigelDenise *denise, const beam_state_t *beam, rigel_u32 cycles);
void rigel_denise_set_framebuffer_target(RigelDenise *denise, const rigel_framebuffer_target_t *target);

#endif
