# Pending work

Items that are known-incomplete or deferred. See `api_status.md` for the
full per-subsystem breakdown.

---

## Chipset / library

### `RIGEL_PIXEL_INDEXED_8BIT` is not implemented

`rigel_config_t.pixel_format` supports `RGBA8888` and `RGB565`. Indexed output
is still missing because it requires Denise to retain post-priority,
pre-palette chunky indices, not just convert the completed RGBA frame.

### `rigel_snapshot_t` captures only 3 fields

`cycles`, `intreq`, `intena` — the rest of the chipset state (copper, blitter,
audio, disk, beam) is not captured. Do not use for save state until this is
expanded. Blocked on internal state stabilising.

### Full ECS support is incomplete

Rigel has an ECS feature gate, Agnus/Denise IDs, minimal `BEAMCON0` PAL/NTSC
handling, `DIWHIGH`, and a 1 MiB ECS Chip RAM window. Full ECS still needs
programmable beam timing, SuperHires/Productivity modes, 2 MiB Agnus variants,
and real `BPLCON3` semantics. See `AI_context/ecs_support.md`.

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
