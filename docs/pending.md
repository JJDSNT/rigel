# Pending work

Items that are known-incomplete or deferred. See `api_status.md` for the
full per-subsystem breakdown.

---

## Chipset / library

### `RIGEL_EVENT_AUDIO_READY` does not fire

The deadline contribution from the audio domain is wired into
`rigel_get_next_deadline()`, but the event itself is never raised.
`rigel_get_audio_sample()` polling still works.

### Pixel format is not configurable

The frame buffer is always RGBA8888. `RGB565` and `INDEXED_8BIT` are planned
in `rigel_config_t` but not implemented.

### `rigel_snapshot_t` captures only 3 fields

`cycles`, `intreq`, `intena` — the rest of the chipset state (copper, blitter,
audio, disk, beam) is not captured. Do not use for save state until this is
expanded. Blocked on internal state stabilising.

### `blitter_line_step` not wired into the active path

`blitter_line_step` (per-slot Bresenham) is implemented but
`blitter_dma.c` still dispatches through `blitter_execute_reference`.
The incremental path needs to replace the reference backend.

### `raster_config_t` / `refresh_dma_state_t` not wired into `agnus_state.h`

`raster.c` and `refresh_dma.c` are built and functional in isolation but
their state structs are not embedded in `RigelAgnus`. The slot scheduler
owns refresh slot placement independently.

### FRAME_READY vs VBLANK semantics undocumented

Both events can fire in the same `rigel_step`. The relationship between them
(VBLANK = hardware signal lines 0–25; FRAME_READY = frame buffer swap) is not
documented in the public headers or integration guide.

---

## Harness (`harness/`)

The Musashi integration harness was written for internal timing tests, not for
running a real ROM. The gaps below must be addressed before KS3.1 can boot.
See `AI_context/ks31_support.md` for the full analysis and code sketches.

| # | Gap | Impact |
|---|---|---|
| 1 | CIA read/write not mapped in Musashi callbacks | critical — any CIA access faults |
| 2 | ROM mirror at `$FC0000` missing | critical — CPU reset vector unreachable |
| 3 | Chip RAM overlay via CIA-A PRA not implemented | critical — exception vectors wrong |
| 4 | `harness_load_rom_file()` not implemented | needed for real ROM testing |
| 5 | CIA-B keyboard ACK (SPMODE toggle) unverified | KS may stall waiting for ACK |
| 6 | Bus stall / chip RAM arbitration (`cpu_would_stall`) | timing only, not a blocker |
| 7 | Slow RAM at `$C00000` not mapped | minor — KS boots without it |
