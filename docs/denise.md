# Denise

Denise is the visual composition and output side of the classic chipset in Rigel.

## Ownership

- **Agnus** owns time, DMA cadence, and fetch scheduling.
- **Denise** consumes already-timed state and turns it into display-facing
  composition state.

Denise owns:
- colour registers and palette expansion (writes expand to RGB32 immediately)
- sprite composition state
- playfield mode interpretation
- display window state
- scanline/output-facing staging

Denise does not own:
- DMA arbitration
- bitplane fetch scheduling
- sprite DMA policy
- master beam timing

## Implemented baseline

- Palette writes expand immediately to RGB32.
- Playfield mode flags derive from `BPLCON0`.
- Display window state derives width/height and visible bounds from `DIWSTRT`/`DIWSTOP`.
- Output state mirrors current beam position into visible-scanline state and a scanline buffer.
- **Compositor** (`rigel_denise_compositor_tick`): called once per `rigel_step` with
  the final beam position. Handles both line-exit and line-enter transitions so that
  `compose_line` is called correctly even when a single step crosses multiple raster
  lines.
- **`compose_line`**: fills `scanline_rgba` from `plane_words` and the palette;
  sets `scanline_dirty = true`. Exposed to the host via
  `rigel_denise_get_current_scanline`.

## Internal structure

The scaffold is split into:

```
registers/   — BPLCON0/1, DIWSTRT/STOP, DDFSTRT/STOP
render/       — compositor, compose_line
sprites/      — (reserved, not yet implemented)
palette/      — COLOR00–COLOR31 and RGB32 expansion
video/        — beam tracking, visible-area state
output/       — scanline buffer, dirty flag
debug/        — last colour index, debug overlays
```

This split prepares a stronger implementation without collapsing into a monolithic
`denise.c`.
