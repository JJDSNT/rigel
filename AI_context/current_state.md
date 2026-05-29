# Current State

- `RigelContext` is the public host-facing object.
- `RigelChipset` is the internal root object.
- current architectural direction is hybrid:
  - `domains/` for hardware execution domains
  - `chipset/` for composition, MMIO routing and internal wiring
  - `runtime/` for execution policy
  - `bus/` for access surfaces
  - `rtc/` as an auxiliary peripheral outside custom MMIO
- the first explicit execution-domain boundaries now exist under `src/domains/`:
  - `beam`
  - `dma`
  - `copper`
  - `blitter`
  - `interrupt`
  - `disk`
  - `serial`
  - `audio`
  - `input`
- these domains are currently single-thread coordination surfaces, not parallel runtimes
- `DMACON` ownership now sits under the DMA domain state instead of the top level of `RigelAgnus`
- Agnus MMIO reads/writes for `DMACON` already go through DMA-domain helpers
- chip-facing MMIO now enters directly through Agnus/Denise/Paula entrypoints instead of separate `*regs` wrapper files
- Paula interrupt policy (`INTREQ`, `INTENA`, `IPL`) now lives in the interrupt domain, with `paula_interrupts.*` acting as the compatibility/wiring surface
- Paula disk MMIO, stepping and DMA-service routing now go through the disk domain, with `paula_regs.c` and `paula_state.c` reduced to composition/wiring roles
- Paula serial is now a real migrated domain, not a placeholder:
  - `SERDAT`
  - `SERPER`
  - `SERDATR`
  - TX stepping
  - RBF/TBE IRQ publication
- Paula audio is now a real migrated domain, not a placeholder:
  - `AUDxLCH/LCL/LEN/PER/VOL/DAT`
  - per-channel state
  - basic stepping and mixing
  - `DMACON` propagation from chipset into the audio domain
- Paula input is now a real migrated domain, not a placeholder:
  - `JOY0DAT/JOY1DAT`
  - `POT0DAT/POT1DAT`
  - `POTGO/POTGOR`
  - host-facing setters for joystick data and POT buttons
- `src/chipset/paula/paula_old/` is no longer required as functional reference material and has been removed.
- `RigelChipset` currently composes:
  - `RigelAgnus`
  - `RigelDenise`
  - `RigelPaula`
  - `RigelRTC`
  - `FloppyDrive[4]`
- `RigelAgnus` already owns:
  - `beam`
  - `dma`
  - `copper`
  - `bitplanes`
  - `BlitterState`
  - `agnus_slot_scheduler_t scheduler` (Approach C foundation)
- `copper` now has a timing-aware path:
  - `COP1LC/COP2LC`
  - `COPJMP1/COPJMP2`
  - internal wait target bound to beam position with VP/HP mask from IR2
  - `RIGEL_EVENT_COPPER` when the beam reaches an armed wait point under `DMAEN|COPEN`
  - Chip RAM fetch path for copper list words
  - full decode path:
    - `MOVE` writes to custom register space
    - `WAIT` arms a beam-bound wait point with correct masked comparison
    - `SKIP` evaluates beam condition immediately; skips or does not skip next instruction
- timing/Approach C active:
  - `beam_in_vblank()` corrected to use Agnus VBL zone (lines 0–25), not Denise display window
  - `rigel_get_next_deadline()` aggregates blitter + beam_line_end + VERTB + copper_wait via `agnus_deadlines_t`
  - slot scheduler IS the inner loop of `rigel_agnus_step()`: drives beam CCK-by-CCK, dispatches copper and blitter at correct hardware slots
  - copper dispatched at COPPER slots only (steals FREE slots); blitter dispatched at BLITTER slots (steals FREE slots; also CPU slots if nasty)
  - beam is canonical position source; scheduler derives its position from beam after each step
  - `rigel_get_bus_state()` uses `agnus_slot_scheduler_current_owner()` and `slot_to_bus_owner()` for slot-accurate bus reporting
  - VERTB interrupt (`AGNUS_INTB_VERTB`) fires at vpos=0, hpos=1 from within the slot scheduler after each beam step
  - DDFSTRT/DDFSTOP wired: MMIO writes update `scheduler.ddfstrt`/`ddfstop` and invalidate the slot table; bitplane fetch range is now derived from these fields rather than a fixed constant
- `RigelPaula` already owns:
  - interrupt model (`INTREQ`, `INTENA`, `IPL`)
  - `audio` state
  - `disk` state with MMIO-visible registers and minimal DMA service
  - `serial` state
  - `input` state
- internal `floppy` module now exists as a separate peripheral:
  - `FloppyDrive`
  - MFM track encoder helpers
- public floppy surface now exists in the API:
  - `DF0..DF3`
  - insert/eject
  - status query per drive
- CIA-B PRB/DDRB selects DF0-DF3 through `/SELx`; Paula disk DMA follows the
  first selected drive and public status reports `dma_active` for that selected
  drive.
- CIA-B TOD receives a synthetic floppy `/INDEX` pulse when the active drive has
  media and motor on.
- RTC is now considered part of the library scope, but not part of the custom chip family.
- Denise is now being scaffolded as a visual composition/output subsystem.
- Denise direction:
  - Agnus owns time, DMA cadence, and fetch scheduling
  - Denise owns display-facing registers, palette, composition state, and output staging
- Denise now has a real scanline composition path:
  - display window width/height are derived from `DIWSTRT/DIWSTOP`
  - framebuffer/output state tracks current beam position, visible scanline, current pixel, and last RGB
  - `plane_words[6][64]` line buffer accumulates bitplane DMA words fetched by Agnus during each scanline
  - on line exit (transition from visible to next): `planar_to_chunky` + palette-map into `scanline_rgba`
  - on entering a visible scanline from a non-visible one: immediate background fill so callers see valid data
  - public inspection API exposes `scanline_rgba` (RIGEL_DENISE_MAX_SCANLINE_PIXELS=1024 pixels) and `last_rgb`
- Agnus bitplane DMA fetch is wired end-to-end:
  - `BPL1PTH–BPL6PTL` (0x0E0–0x0F6) MMIO writes route to `bplpt_set_hi/lo()` in Agnus
  - at each `AGNUS_SLOT_BITPLANE` CCK, `bitplane_fetch_step()` reads chip RAM, stores the word, advances the pointer
  - fetched word is written to `denise->output.plane_words[plane][widx]`
  - `fetch_plane_index` tracks interleaved plane cycling (0→depth-1 per word); `plane_word_count` increments per word-group
- Frame double-buffering implemented:
  - `frame_rgba[2][MAX_LINES][MAX_PIXELS]` + `front_idx` in `denise_output_state_t`
  - Denise writes to `frame_rgba[1^front_idx]`; at frame boundary `front_idx ^= 1`
  - Host reads `frame_rgba[front_idx]` via `rigel_get_frame` / `rigel_get_scanline`
- BPL1MOD/BPL2MOD (0x108/0x10A) fully wired:
  - `bplmod[2]` (signed) in `bitplane_pointers_t`
  - Set via MMIO; applied to each active plane pointer at end of every non-VBL display line by the slot scheduler
- DIWSTRT/DIWSTOP cross-domain: both registers live in the Denise MMIO domain; `rigel_denise_write_reg` propagates to Agnus raster + slot scheduler (same pattern as BPLCON0 HIRES)
- BPLCON0/1/2 register constants moved to `rigel_custom.h` (were split between public header and local enums in Denise)
- Audio deadline wired: `audio_cycles_to_next_event()` contributes to `rigel_get_next_deadline()`
- Disk deadline wired: `disk_cycles_to_next_event()` contributes to `rigel_get_next_deadline()`
- Blitter LINE mode active: `blitter_step_dma` dispatches to `blitter_line_step` per DMA slot instead of reference execute
- Raster config implemented: `raster_reset`, `raster_in_display_window`, `raster_in_fetch_window` now functional (was all TODO stubs)
- Refresh DMA wired: `refresh_dma_step` called at each `AGNUS_SLOT_REFRESH` CCK
- Copper MOVE/WAIT/SKIP fully implemented: `copper_exec_move` routes via `custom_regs_write16`; `copper_exec_skip_test` uses `copper_beam_cmp`; `copper_wait_arm` stores VP/HP with masks
- Denise render unit tests added to CTest (22 tests total):
  - `test_priority`: 8 cases covering sprite/playfield priority from BPLCON2
  - `test_ham`: HAM6 (ctrl bits[5:4]) and HAM8 (ctrl bits[7:6]) decode
  - `test_dualpf`: plane splitting + priority resolution
  - `test_sprites`: hstart/vstart/vstop, pixel shifting, transparency, attached detection
- Priority fix: `denise_priority_resolve` was double-mapping sprite pixel index; corrected to pass palette index as-is
- Video mode surface tests cover OCS PAL/NTSC, lores/hires, interlace intent,
  HAM6/EHB and ECS unsupported-mode guards.
- Runtime traces now go through structured `rigel_log_event_t` events rather
  than direct formatting in chipset paths. Bare-metal builds can set
  `RIGEL_ENABLE_STDIO_LOG=OFF`; `test_baremetal_no_stdio` checks that
  `librigel.a` has no `printf`/`fprintf`/`snprintf`/`stderr` references.
- Optional SIMD video-buffer helpers live under `src/simd/` with SSE2/NEON
  backends and scalar fallback; public API and timing remain deterministic.

- Dead code removed: `pixel_pipeline.c/h` (compositor implementa o pipeline inline em `compose_line`), `scanline.c/h` (API nunca chamada), `agnus/dma/dma.c/h` e `agnus/timing/beam.c/h` (stubs de forwarding sem callers), `agnus/bitplanes/display_window.c/h` (substituído por raster.c)
- `agnus_irq.c` reduzido a `agnus_irq_raise_vblank`; `raise_blitter_done` e `raise_copper` eram dead code (blitter usa `BlitterIrqSink`, copper não levanta IRQ pelo agnus_irq path)
- Todos os comentários TODO obsoletos limpos (deadline.h, slot_scheduler.c, sprite_attach.c)

- Denise keeps its internal split as the canonical direction for visual work:
  - `registers/`
  - `render/`
  - `sprites/`
  - `palette/`
  - `video/`
  - `output/`
  - `debug/`
- Agnus scaffolding was intentionally reduced back toward the documented architecture:
  - `domains/` remains the real place for beam, DMA, copper, and blitter coordination
  - `src/chipset/agnus/` stays focused on composition, MMIO entrypoints, state ownership, and Agnus-specific code
- speculative chipset-level helper headers that suggested a parallel DMA/event API were removed
- important rule kept explicit:
  - there is no `agnus/sprites/`
  - sprite fetch belongs under Agnus DMA
  - sprite interpretation/composition belongs outside Agnus

# Working Paths

- Blitter path:
  - MMIO -> Agnus -> Blitter registers
  - DMA step -> IRQ publish -> chipset IRQ surface -> Paula interrupts

- Copper path:
  - MMIO -> Agnus copper pointers/jumps
  - beam advances in Agnus
  - copper fetches words from Chip RAM through host callbacks
  - copper executes minimal `MOVE` / arms minimal `WAIT`
  - copper domain wakes when beam reaches the armed wait point
  - core publishes `RIGEL_EVENT_COPPER`
  - Denise current-scanline inspection can now observe copper-driven color changes without a display

- Paula disk path:
  - MMIO -> Paula -> disk registers
  - disk DMA service -> chip RAM interface write -> IRQ publish
  - no-media fallback -> countdown -> fake `DSKBLK` IRQ
  - media-present path -> `FloppyDrive` ADF track -> MFM track buffer -> DMA words

# Disk Notes

- `disk_reset()` preserves external attachments/configuration:
  - chip RAM interface
  - IRQ sink
  - attached `FloppyDrive`
- `disk` now distinguishes:
  - attached drive with media -> servable DMA path
  - no drive / no media -> countdown fallback path
- track data is currently generated from the selected drive track using the internal MFM encoder.
- `RigelChipset` currently owns 4 internal drives.
- public floppy status currently exposes:
  - `has_media`
  - `motor_on`
  - `ready`
  - `track0`
  - `disk_changed`
  - `write_protected`
  - `dma_active`
  - `cylinder`
  - `side`
- DF0-DF3 are selected through CIA-B control paths; Paula disk DMA follows the
  selected drive.

# Notes

- `RigelChipset` intentionally mediates IRQ as a surface.
- Paula currently implements the IRQ policy behind that surface.
- `runtime` should not know chip-specific details; stepping is moving under `chipset`.
- `Rigel` is not being designed as multicore-first.
- `Rigel` should be concurrency-aware internally, but deterministic and single-thread for the classic chipset path.
- domains are meant to express ownership and temporal boundaries, not immediate thread boundaries.
- `RigelAgnus` now owns explicit `beam`, `dma`, `copper`, `bitplanes`, and `blitter` state, with stepping still routed through domains where appropriate.
- the public timing surface now exposes:
  - `rigel_get_clock_hz()`
  - `rigel_get_line_cycles()`
  - `rigel_get_frame_cycles()`
  - `rigel_cycles_to_us()`
  - `rigel_us_to_cycles()`
- this is intended to let hosts monitor drift and frame pacing early without depending on internal state layout
