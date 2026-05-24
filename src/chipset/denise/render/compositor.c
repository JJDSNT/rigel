#include "denise/render/compositor.h"

#include <stdint.h>

#include "denise/output/framebuffer.h"
#include "denise/output/planar.h"
#include "denise/render/dualpf.h"
#include "denise/render/ehb.h"
#include "denise/render/ham.h"
#include "denise/sprites/collisions.h"
#include "denise/sprites/sprites.h"
#include "rigel/rigel_denise_types.h"

static void compose_line(RigelDenise *denise)
{
    rigel_denise_output_state_t *output = &denise->output;
    const rigel_u32 *palette = denise->palette.rgb32;
    unsigned depth    = (denise->regs.bplcon0 >> 12) & 0x7u;
    rigel_u16 bplcon2 = denise->regs.bplcon2;
    bool is_ham  = (denise->regs.bplcon0 & 0x0800u) != 0u;
    bool is_dual = (denise->regs.bplcon0 & 0x0400u) != 0u;
    bool is_ehb  = !is_ham && !is_dual && depth == 6u && !(bplcon2 & 0x0040u);
    unsigned scroll = denise->regs.bplcon1 & 0x0Fu;
    unsigned pf1p   = bplcon2 & 0x7u;
    unsigned pf2p   = (bplcon2 >> 3) & 0x7u;
    rigel_u16 w, px;

    /* Per-pixel state for rendering, priority, and collision detection. */
    uint8_t pf_color[1024];    /* 0 = transparent */
    uint8_t pf_prio[1024];     /* PFxP for sprite-vs-PF priority */
    uint8_t pf_active[1024];   /* bit 0 = PF1 active, bit 1 = PF2 active */
    uint8_t spr_active[1024];  /* bit N = sprite N has non-transparent pixel */

    if (!output->visible_scanline || output->scanline_width == 0) return;

    for (px = 0; px < output->scanline_width; px++) {
        output->scanline_rgba[px] = palette[0];
        pf_color[px]   = 0;
        pf_prio[px]    = (uint8_t)pf1p;
        pf_active[px]  = 0;
        spr_active[px] = 0;
    }

    if (depth > 0 && depth <= 6 && output->plane_word_count > 0) {
        if (is_ham) {
            /* HAM6: each pixel depends on the previous; state advances even
             * for pixels outside the visible window (scroll, right clip). */
            rigel_u32 prev_rgb = palette[0];
            for (w = 0; w < output->plane_word_count; w++) {
                rigel_u16 block_words[6];
                uint8_t   pixels_chunky[16];
                unsigned  p;
                for (p = 0; p < 6; p++)
                    block_words[p] = (p < depth) ? output->plane_words[p][w] : 0u;
                planar_to_chunky(block_words, depth, pixels_chunky);
                for (px = 0; px < 16; px++) {
                    int screen_x = (int)(w * 16u + px) - (int)scroll;
                    prev_rgb = ham6_decode_pixel(pixels_chunky[px], prev_rgb, palette);
                    if (screen_x < 0 || (unsigned)screen_x >= output->scanline_width)
                        continue;
                    output->scanline_rgba[(unsigned)screen_x] = prev_rgb;
                    pf_color[(unsigned)screen_x]  = 0xFFu; /* opaque sentinel */
                    pf_active[(unsigned)screen_x] = 1u;    /* PF1 active */
                    /* pf_prio already set to pf1p in init loop */
                }
            }
        } else if (is_dual) {
            /* Dual playfield: odd planes → PF1 (colors 1–7),
             *                 even planes → PF2 (colors 9–15). */
            for (w = 0; w < output->plane_word_count; w++) {
                rigel_u16 block_words[6];
                uint8_t   pixels_chunky[16];
                unsigned  p;
                for (p = 0; p < 6; p++)
                    block_words[p] = (p < depth) ? output->plane_words[p][w] : 0u;
                planar_to_chunky(block_words, depth, pixels_chunky);
                for (px = 0; px < 16; px++) {
                    int screen_x = (int)(w * 16u + px) - (int)scroll;
                    dualpf_result_t dpf;
                    bool use_pf2;
                    uint8_t ci;
                    if (screen_x < 0 || (unsigned)screen_x >= output->scanline_width)
                        continue;
                    dpf = dualpf_decode(pixels_chunky[px]);
                    /* PF2 wins when both opaque and pf2p >= pf1p (tie → PF2). */
                    use_pf2 = dpf.pf2_index && (!dpf.pf1_index || pf1p <= pf2p);
                    ci = use_pf2 ? dpf.pf2_index : dpf.pf1_index;
                    pf_color[(unsigned)screen_x]  = ci;
                    pf_prio[(unsigned)screen_x]   = (uint8_t)(use_pf2 ? pf2p : pf1p);
                    pf_active[(unsigned)screen_x] =
                        (uint8_t)((dpf.pf1_index ? 1u : 0u) | (dpf.pf2_index ? 2u : 0u));
                    if (ci)
                        output->scanline_rgba[(unsigned)screen_x] = palette[ci];
                }
            }
        } else {
            /* Normal single playfield (includes EHB with 6 planes). */
            for (w = 0; w < output->plane_word_count; w++) {
                rigel_u16 block_words[6];
                uint8_t   pixels_chunky[16];
                unsigned  p;
                for (p = 0; p < 6; p++)
                    block_words[p] = (p < depth) ? output->plane_words[p][w] : 0u;
                planar_to_chunky(block_words, depth, pixels_chunky);
                for (px = 0; px < 16; px++) {
                    int screen_x = (int)(w * 16u + px) - (int)scroll;
                    uint8_t ci;
                    if (screen_x < 0 || (unsigned)screen_x >= output->scanline_width)
                        continue;
                    ci = is_ehb
                        ? (pixels_chunky[px] & 0x3Fu)
                        : (pixels_chunky[px] & 0x1Fu);
                    pf_color[(unsigned)screen_x]  = ci;
                    pf_active[(unsigned)screen_x] = (ci != 0) ? 1u : 0u;
                    output->scanline_rgba[(unsigned)screen_x] =
                        is_ehb ? ehb_resolve_color(ci, palette) : palette[ci];
                }
            }
        }
    }

    /* Sprite overlay — iterate 7→0 so lower-numbered (higher-priority) sprites
     * overwrite higher-numbered ones when they share a pixel.
     * Attached pairs (odd sprite CTL bit 7): even sprite renders the combined
     * 4bpp pixel; the odd sprite is skipped (its data is consumed by the even).
     * A sprite pair p beats PFx when the PF pixel is transparent or pair+PFxP < 4. */
    {
        unsigned spr;
        rigel_u16 x_start = denise->video.visible_x_start;
        for (spr = DENISE_SPRITE_COUNT; spr-- > 0; ) {
            const denise_sprite_t *sp = &denise->sprites.sp[spr];
            unsigned pair = spr / 2u;
            bool is_odd          = (spr & 1u) != 0u;
            bool attached_odd    = is_odd  && denise_sprite_is_attached(&denise->sprites, spr);
            bool attached_even   = !is_odd && denise_sprite_is_attached(&denise->sprites, spr + 1u);
            rigel_u16 hstart;
            unsigned px_idx;

            /* Odd sprite of an attached pair: rendered via the even partner */
            if (attached_odd) continue;
            if (!sp->armed) continue;

            hstart = denise_sprite_hstart(sp);
            for (px_idx = 0; px_idx < 16u; px_idx++) {
                rigel_u16 amiga_x = (rigel_u16)(hstart + px_idx);
                rigel_u16 scan_px;
                uint8_t pix;
                rigel_u32 color;
                if (amiga_x < x_start) continue;
                scan_px = (rigel_u16)(amiga_x - x_start);
                if (scan_px >= output->scanline_width) break;
                if (attached_even) {
                    /* 4bpp attached pair — pix is already the final palette index */
                    pix = denise_sprite_attached_pixel(&denise->sprites, spr, amiga_x);
                    if (pix == 0) continue;
                    color = palette[pix];
                } else {
                    /* Normal 2bpp sprite — map to sub-palette at 16 + pair*4 */
                    pix = denise_sprite_pixel(&denise->sprites, spr, amiga_x);
                    if (pix == 0) continue;
                    color = palette[(16u + pair * 4u + pix) & 31u];
                }
                /* Accumulate for collision detection (before priority discard) */
                spr_active[scan_px] |= (uint8_t)(1u << spr);

                if (pf_color[scan_px] == 0 || pair + pf_prio[scan_px] < 4u)
                    output->scanline_rgba[scan_px] = color;
            }
        }
    }

    /* Collision detection — per-pixel, driven by CLXCON enable bits. */
    if (denise->coll.clxcon != 0) {
        unsigned cx;
        for (cx = 0; cx < (unsigned)output->scanline_width; cx++) {
            if (spr_active[cx] == 0 && pf_active[cx] < 2u) continue;
            collision_check_pixel(&denise->coll, spr_active[cx],
                                  (pf_active[cx] & 1u) != 0,
                                  (pf_active[cx] & 2u) != 0);
        }
    }

    output->scanline_dirty = true;

    /* Accumulate frame-level flags and mark this raster line dirty. */
    if (is_ham)  output->pending_flags |= (rigel_u32)RIGEL_FRAME_HAM;
    if (is_dual) output->pending_flags |= (rigel_u32)RIGEL_FRAME_DUAL_PLAYFIELD;
    {
        unsigned spr;
        for (spr = 0; spr < DENISE_SPRITE_COUNT; spr++) {
            if (denise->sprites.sp[spr].armed) {
                output->pending_flags |= (rigel_u32)RIGEL_FRAME_SPRITES_ACTIVE;
                break;
            }
        }
    }
    {
        rigel_u16 y = output->beam_vpos;
        if (y < RIGEL_DENISE_MAX_LINES)
            output->pending_dirty[y / 64u] |= (rigel_u64)1u << (y % 64u);
    }
}

void rigel_denise_compositor_tick(RigelDenise *denise, const beam_state_t *beam, rigel_u32 cycles)
{
    bool line_changed;
    bool was_visible;

    (void)cycles;

    if (denise == NULL || beam == NULL) {
        return;
    }

    line_changed = (denise->output.beam_vpos != beam->vpos) ||
                   (denise->output.frame_counter != beam->frame_count);
    was_visible = denise->output.visible_scanline;

    if (line_changed && was_visible) {
        compose_line(denise);
    }

    rigel_denise_framebuffer_sync_from_beam(denise, beam);

    if (line_changed && !was_visible && denise->output.visible_scanline) {
        compose_line(denise);
    }

    denise->output.last_rgb = denise->palette.rgb32[denise->debug.last_color_index & 31u];
}
