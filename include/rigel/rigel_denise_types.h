#ifndef RIGEL_DENISE_TYPES_H
#define RIGEL_DENISE_TYPES_H

#include "rigel_config.h"
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

typedef struct rigel_denise_scanline {
    rigel_u64 frame_counter;
    rigel_u16 y;
    rigel_u16 width;
    const rigel_u32 *pixels_rgba;
    rigel_u32 last_rgb32;
    bool visible;
    bool dirty;
} rigel_denise_scanline_t;

typedef enum rigel_frame_flags {
    RIGEL_FRAME_HAM            = 1u << 0,  /* HAM6 active on at least one visible line */
    RIGEL_FRAME_DUAL_PLAYFIELD = 1u << 1,  /* dual-playfield active on at least one line */
    RIGEL_FRAME_SPRITES_ACTIVE = 1u << 2,  /* at least one sprite was armed this frame */
    RIGEL_FRAME_COPPER_ACTIVE  = 1u << 3,  /* reserved — copper mid-frame writes */
    RIGEL_FRAME_INTERLACE_ODD  = 1u << 4,  /* reserved — odd interlace field */
    RIGEL_FRAME_INTERLACE_EVEN = 1u << 5   /* reserved — even interlace field */
} rigel_frame_flags_t;

/* Per-frame dirty tracking. Each bit in dirty_lines represents one raster line.
 * Bit n of dirty_lines[n/64] corresponds to raster line n (0 = top of field).
 * A set bit means compose_line ran for that line (i.e., it was in the visible window).
 * full_redraw is reserved for future use (palette invalidation, resolution change). */
typedef struct rigel_frame_delta {
    rigel_u64 dirty_lines[5]; /* covers 320 bits — sufficient for 312 PAL lines */
    bool      full_redraw;
} rigel_frame_delta_t;

typedef struct rigel_frame {
    rigel_u32             width;       /* visible pixels per row */
    rigel_u32             height;      /* visible rows */
    rigel_u32             pitch;       /* bytes between row starts */
    rigel_u64             frame_count;
    rigel_pixel_format_t  format;
    const void           *pixels;      /* valid until next rigel_step */
    rigel_frame_flags_t   flags;       /* what was active during this frame */
    rigel_frame_delta_t   delta;       /* which raster lines were composed */
} rigel_frame_t;

typedef struct rigel_denise_debug_state {
    rigel_u32 frame_counter;
    rigel_u32 scanline_counter;
    rigel_u16 active_mode_flags;
    rigel_u16 last_color_index;
    rigel_u16 beam_hpos;
    rigel_u16 beam_vpos;
    rigel_u16 current_scanline;
    rigel_u16 current_pixel;
    rigel_u32 last_rgb32;
    bool visible_scanline;
} rigel_denise_debug_state_t;

#endif
