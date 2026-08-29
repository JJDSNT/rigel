---
id: ISSUE-0008
title: "No per-line display description: a host with its own blitter cannot render Denise itself"
status: open
priority: low
type: enhancement
owner: unassigned
created_at: 2026-08-29
updated_at: 2026-08-29
tags:
  - denise
  - video
  - api
  - host-integration
related_files:
  - include/rigel/rigel_denise_video.h
  - include/rigel/rigel_denise_types.h
  - src/chipset/denise/
---

## What the API gives today

```c
bool rigel_denise_get_video_desc(const RigelContext *ctx, rigel_denise_video_desc_t *desc);
bool rigel_denise_get_current_scanline(const RigelContext *ctx, rigel_denise_scanline_t *out);
bool rigel_get_scanline(const RigelContext *ctx, rigel_u16 y, rigel_denise_scanline_t *out);
bool rigel_get_frame(const RigelContext *ctx, rigel_frame_t *frame);
```

Finished RGBA, whole frame or per line, with `dirty` and `last_rgb32` on the
scanline and `RIGEL_FRAME_HAM` / `DUAL_PLAYFIELD` / `SPRITES_ACTIVE` /
`COPPER_ACTIVE` on the frame. **This is enough to put a correct picture on a
screen**, and a host should start there. This issue is not a complaint about it.

## What a host with its own hardware cannot do

Some hosts can render an Amiga display faster than a general chipset emulation
can, because they have vector units and a compositor. On a Raspberry Pi the
Hardware Video Scaler will scale a plane, composite several of them and place
sprites for free, and NEON will do planar-to-chunky and palette expansion per
line at a cost that does not scale with colour clocks.

Such a host cannot use any of that, because the API's smallest unit of *input*
is a finished pixel. To render a line itself it would need the description that
produced it:

- bitplane pointers and modulos as the DMA engine currently holds them;
- BPLCON0/1/2 -- depth, hires, lace, HAM, dual playfield, scroll, priority;
- the palette in effect for that line;
- DIWSTRT/DIWSTOP and DDFSTRT/DDFSTOP;
- sprite position, control, data and attachment.

## And a per-line version of the frame flags

A host that renders lines itself can only do it for lines whose description is
stable across the line. A copper writing `BPLCONx` or a colour register mid-line
produces something no description can express, and the host has to fall back to
Rigel's own rendering for that line.

`rigel_frame_t` already carries HAM, dual-playfield, sprite and copper flags,
but at frame granularity -- which answers "did this happen anywhere in the
frame", when the question is "did it happen in *this* line". A per-line flag
saying the line's description changed after it began would let the host mix the
two paths automatically instead of choosing one for the whole frame.

Something like:

```c
typedef struct rigel_denise_line_desc {
    rigel_u32 bpl_ptr[6];      /* as held at the start of the line */
    rigel_u16 bpl1mod, bpl2mod;
    rigel_u16 bplcon0, bplcon1, bplcon2;
    rigel_u16 color[32];
    rigel_u16 diwstrt, diwstop, ddfstrt, ddfstop;
    bool      stable;          /* false = registers changed within the line */
} rigel_denise_line_desc_t;

bool rigel_denise_get_line_desc(const RigelContext *ctx, rigel_u16 y,
                                rigel_denise_line_desc_t *out);
```

Field set is a suggestion. What matters is that a host can reconstruct the line,
and can tell when it must not try.

## Relationship to ISSUE-0006

These overlap and the order matters. ISSUE-0006 is about the per-colour-clock
loop, and `rigel_denise_framebuffer_sync_from_beam` (13.2%) plus
`rigel_denise_compositor_tick` (5.3%) are a large part of it. Rendering Denise
per scanline segment instead of per colour clock -- hypothesis 4 in
`from_bellatrix/rigel_performance_research.md` -- would make Rigel itself much
faster **for every host**, without any new API.

That is worth doing first. This issue only becomes interesting if, after that,
a host can still beat Rigel by using hardware Rigel cannot know about. Treat it
as a question to revisit with a measurement, not as work to schedule.

## Priority

Low, and deliberately so. Bellatrix's own plan (its `AI_context/issues/
ISSUE-0073.md`) starts from `rigel_get_frame()` and only reaches this after
measuring a real workload. Nothing is blocked on it.
