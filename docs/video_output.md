# Video Output

## Responsibility split

**Rigel produces correct classic video.** The host presents video.

```
Rigel / chipset:                  Host:
  bitplane DMA fetch                Rigel pixels → framebuffer
  BPLCONx (mode, resolution)        scale, aspect ratio
  dual playfield                    real vsync
  HAM / EHB                         RGB565 / RGBA8888 for hardware
  sprites (priority, collision)     overlay / debug UI
  palette (COLOR00–COLOR31)
  raster output line by line
```

Planar→chunky is not a format conversion — it is the materialisation of what
Denise actually does during the DMA slot sequence. This logic belongs to the
chipset.

## Current implemented baseline

```c
bool rigel_denise_get_current_scanline(const RigelContext *ctx,
                                       rigel_denise_scanline_t *scanline);
```

The project currently exposes the current scanline assembled by Denise for
inspection and headless testing. This already allows validating copper-driven
effects and palette changes without a graphics backend, while the full
frame/scanline API matures.

The compositor (`rigel_denise_compositor_tick`) is called once per `rigel_step`
with the final beam position. It:
- calls `compose_line()` on leaving a visible scanline (line-exit)
- calls `compose_line()` on entering a visible scanline from a non-visible one (line-enter)

This handles the case where a single `rigel_step` crosses multiple raster lines.

## Planned API

```c
typedef enum rigel_pixel_format {
    RIGEL_PIXEL_INDEXED_8BIT,   /* index 0–31 (OCS) / 0–255 (AGA) — pre-palette */
    RIGEL_PIXEL_RGBA8888,       /* ready for the host to copy */
    RIGEL_PIXEL_RGB565
} rigel_pixel_format_t;

typedef struct rigel_frame_delta {
    uint64_t dirty_lines[4];  /* bitmask 1 bit/line — covers 256 lines */
    bool     full_redraw;     /* palette changed globally, resolution switched, etc. */
} rigel_frame_delta_t;

typedef enum rigel_frame_flags {
    RIGEL_FRAME_INTERLACE_ODD   = 1 << 0,
    RIGEL_FRAME_INTERLACE_EVEN  = 1 << 1,
    RIGEL_FRAME_COPPER_ACTIVE   = 1 << 2,
    RIGEL_FRAME_SPRITES_ACTIVE  = 1 << 3,
    RIGEL_FRAME_HAM             = 1 << 4,
    RIGEL_FRAME_DUAL_PLAYFIELD  = 1 << 5
} rigel_frame_flags_t;

typedef struct rigel_frame {
    uint32_t              width;
    uint32_t              height;
    uint32_t              pitch;
    uint64_t              frame_count;   /* detect dropped frames */
    rigel_pixel_format_t  format;
    const void           *pixels;
    rigel_frame_flags_t   flags;         /* what happened this frame */
    rigel_frame_delta_t   delta;         /* which lines changed */
} rigel_frame_t;

const rigel_frame_t    *rigel_get_frame(const RigelContext *ctx);
const rigel_scanline_t *rigel_get_scanline(const RigelContext *ctx, uint32_t y);
```

The pixel format is configured at `rigel_create` time via `rigel_config_t`,
not per frame. This lets the chipset optimise its internal pipeline for a
single target format.

## Buffer lifetime

`pixels` is valid between `RIGEL_EVENT_FRAME_READY` and the next `rigel_step`
that advances past the start of the next frame. The host must copy within that
window, or use the pointer directly if there is no concurrency.

## Dirty lines

`delta.dirty_lines` is a bitmask of which lines had output different from the
previous frame. Correct implementation: Denise marks lines during generation
(a line with active DMA = dirty), not by post-generation comparison.

`delta.full_redraw = true` when something invalidates the whole frame — resolution
change via BPLCONx, copper writing COLOR00 on line 0, etc. In that case the host
must redraw everything, regardless of the bitmask.

## Why `rigel_get_scanline` matters

Copper-synchronous effects change palette or resolution mid-frame. A host that
only receives the complete frame cannot reproduce this without reconstructing
the raster. `rigel_get_scanline` exposes line-by-line output for copper-aware hosts.

It is also the correct path for interlace: odd/even fields are accessible
individually without post-processing on the host.

## `RIGEL_PIXEL_INDEXED_8BIT`

This is Denise's natural output before applying the COLOR registers. Hosts that
want full palette control (shader, HDR, debug analysis) need this format. More
useful than `INDEXED_12BIT` because the chunky index has already been resolved
by the sprite and playfield priority engine.

## What does not belong in Rigel

```c
rigel_planar_to_chunky_for_sdl()   /* no */
rigel_scale_2x()                   /* no */
rigel_apply_scanline_filter()      /* no */
```

Any function that mentions the target host belongs in the host.

## Roadmap

```
MVP:
  - rigel_config_t accepts rigel_pixel_format_t (default RGBA8888)
  - Rigel delivers a complete frame in RGBA8888 after RIGEL_EVENT_FRAME_READY
  - rigel_get_frame() returns a pointer to the internal buffer

Later:
  - rigel_get_scanline() for copper-aware hosts and interlace
  - RIGEL_PIXEL_INDEXED_8BIT for hosts with their own colour pipeline
  - Write target via config (zero-copy for Pi/PiStorm streaming)
  - Internal double buffering
```
