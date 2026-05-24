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

## Implemented API

### Full frame — `rigel_get_frame`

```c
typedef struct rigel_frame {
    rigel_u32       width;       /* visible pixels per row */
    rigel_u32       height;      /* visible rows */
    rigel_u32       pitch;       /* bytes between row starts (use for pointer arithmetic) */
    rigel_u64       frame_count; /* monotone counter — detect dropped frames */
    const rigel_u32 *pixels;     /* RGBA8888, first visible pixel */
} rigel_frame_t;

bool rigel_get_frame(const RigelContext *ctx, rigel_frame_t *frame);
```

Call after `RIGEL_EVENT_FRAME_READY`. `pixels` points into Denise's internal
frame buffer (312 lines × 1024 pixels) and is valid until the next `rigel_step`
that advances past the start of the next frame.

`width` and `height` are the visible region as derived from `DIWSTRT`/`DIWSTOP`.
`pitch` is always `1024 * sizeof(uint32_t)` (the full buffer stride), which is
wider than `width * 4` — use `pitch` to advance rows, not `width * 4`.

Use `rigel_denise_get_video_desc()` to obtain the visible region offsets if
needed for precise placement.

### Per-scanline — `rigel_denise_get_current_scanline`

```c
bool rigel_denise_get_current_scanline(const RigelContext *ctx,
                                       rigel_denise_scanline_t *scanline);
```

Returns the scanline currently being assembled. Useful for headless testing and
for hosts that want to observe copper-driven mid-frame effects as they happen.
The `pixels_rgba` pointer inside the returned struct is valid only until the
next `rigel_step`.

The compositor is called once per `rigel_step`:
- on leaving a visible scanline (line-exit): `compose_line()` runs
- on entering a visible scanline from a non-visible one (line-enter): fills background

This handles the case where a single `rigel_step` crosses multiple raster lines.

## Future extensions

The following are not yet implemented:

**Dirty line tracking** — a bitmask of which lines changed vs. the previous
frame, useful for hosts with a physical framebuffer (Pi, USB display) that do
not want to push a full frame when most lines are unchanged:

```c
typedef struct rigel_frame_delta {
    uint64_t dirty_lines[4];  /* 1 bit per line, covers 256 lines */
    bool     full_redraw;     /* palette or resolution changed globally */
} rigel_frame_delta_t;
```

**Frame flags** — metadata about what happened this frame:

```c
typedef enum rigel_frame_flags {
    RIGEL_FRAME_INTERLACE_ODD   = 1 << 0,
    RIGEL_FRAME_INTERLACE_EVEN  = 1 << 1,
    RIGEL_FRAME_COPPER_ACTIVE   = 1 << 2,
    RIGEL_FRAME_SPRITES_ACTIVE  = 1 << 3,
    RIGEL_FRAME_HAM             = 1 << 4,
    RIGEL_FRAME_DUAL_PLAYFIELD  = 1 << 5
} rigel_frame_flags_t;
```

**Pixel formats** — `RIGEL_PIXEL_INDEXED_8BIT` (pre-palette chunky index) and
`RIGEL_PIXEL_RGB565`, configured at `rigel_create` time. The current output is
always RGBA8888.

**Write target** — zero-copy path for PiStorm/Pi streaming where the host
supplies a framebuffer pointer at config time.

## What does not belong in Rigel

```c
rigel_planar_to_chunky_for_sdl()   /* no */
rigel_scale_2x()                   /* no */
rigel_apply_scanline_filter()      /* no */
```

Any function that mentions the target host belongs in the host.
