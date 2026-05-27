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
    rigel_pixel_format_t format; /* RGBA8888 or RGB565 */
    const void      *pixels;     /* first visible pixel in `format` */
} rigel_frame_t;

bool rigel_get_frame(const RigelContext *ctx, rigel_frame_t *frame);
```

Call after `RIGEL_EVENT_FRAME_READY`. `pixels` points into Denise's internal
frame buffer (312 lines × 1024 pixels) and is valid until the next `rigel_step`
that advances past the start of the next frame.

`width` and `height` are the visible region as derived from `DIWSTRT`/`DIWSTOP`.
`pitch` is the full internal buffer stride, not `width * bytes_per_pixel`.
For RGBA8888 it is `1024 * sizeof(uint32_t)`; for RGB565 it is
`1024 * sizeof(uint16_t)`. Use `pitch` to advance rows.

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

**Pixel formats** — `RIGEL_PIXEL_RGBA8888` is the default and
`RIGEL_PIXEL_RGB565` is available via `rigel_config_t.pixel_format`.
`RIGEL_PIXEL_INDEXED_8BIT` remains future work because it needs Denise to retain
post-priority, pre-palette chunky indices during composition.

### Zero-copy write target

Hosts that own a framebuffer can provide it at `rigel_create` time:

```c
static uint16_t framebuffer[HEIGHT][WIDTH];

rigel_config_t cfg = {0};
cfg.pixel_format = RIGEL_PIXEL_RGB565;
cfg.framebuffer.pixels = framebuffer;
cfg.framebuffer.width = WIDTH;
cfg.framebuffer.height = HEIGHT;
cfg.framebuffer.pitch = WIDTH * sizeof(uint16_t);
cfg.framebuffer.format = RIGEL_PIXEL_RGB565;
cfg.framebuffer.little_endian = true;
```

When configured, Denise writes each completed visible scanline directly into
that target using the target pitch. RGB565 is carried through the Denise output
pipeline with a cached RGB565 scanline/palette path, so a RGB565 write target
does not need to reconvert the completed RGBA frame. RGB565 uses:

```c
rgb565 = ((r8 >> 3) << 11) | ((g8 >> 2) << 5) | (b8 >> 3);
```

With `little_endian = true`, the two bytes are stored in little-endian order.
This matches SDL's `SDL_PIXELFORMAT_RGB565` on little-endian hosts and simple
bare-metal `uint16_t *` framebuffers. The internal double-buffered frame remains
available through `rigel_get_frame()`.

`libamivideo` was evaluated for this layer. It is useful as a standalone
planar/palette conversion reference, but Rigel already performs Denise
composition internally; framebuffer format conversion is kept local to avoid
coupling the chipset core to a viewport conversion adapter. It is therefore not
vendored by this repository.

**Write target** — zero-copy path for PiStorm/Pi streaming where the host
supplies a framebuffer pointer at config time.

## What does not belong in Rigel

```c
rigel_planar_to_chunky_for_sdl()   /* no */
rigel_scale_2x()                   /* no */
rigel_apply_scanline_filter()      /* no */
```

Any function that mentions the target host belongs in the host.
