# Denise

`Denise` is being introduced in `Rigel` as the visual composition and output side of the classic chipset.

Current direction:

- `Agnus` owns time, DMA cadence, and fetch scheduling
- `Denise` consumes already-timed state and turns it into display-facing composition state

This means `Denise` should gradually become the owner of:

- color registers and palette expansion
- sprite composition state
- playfield mode interpretation
- display window state
- scanline/output-facing staging

Current implemented baseline:

- palette writes already expand to RGB32
- playfield mode flags already derive from `BPLCON0`
- display window state now derives width/height and visible bounds from `DIWSTRT/DIWSTOP`
- output state now mirrors current beam position into visible-scanline state and a minimal scanline buffer

It should not become the owner of:

- DMA arbitration
- bitplane fetch scheduling
- sprite DMA policy
- master beam timing

The scaffold is intentionally split into:

- `registers/`
- `render/`
- `sprites/`
- `palette/`
- `video/`
- `output/`
- `debug/`

That split is meant to prepare a stronger implementation without collapsing back into a monolithic `denise.c`.
