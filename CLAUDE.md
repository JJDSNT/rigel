# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Rigel is a C library for classic Amiga chipset and peripheral emulation. It provides a deterministic, single-threaded hardware-facing core. The host application owns the CPU, memory map, ROM, and platform integration; Rigel owns the custom chip behavior.

## Build and test

```sh
cmake -S . -B build          # configure (tests on by default)
cmake --build build          # build librigel.a and test executables
ctest --test-dir build --output-on-failure   # run all tests
./build/test_floppy          # run a single test by name
```

Musashi integration harness (off by default):
```sh
cmake -S . -B build-harness -DRIGEL_BUILD_HARNESS=ON -DRIGEL_BUILD_TESTS=OFF
cmake --build build-harness
```

## Architecture

Three layers:

**Public surface** (`include/rigel/`) — `rigel.h` is the entry point.

Key headers:
- `rigel_time.h` — Temporal API: `rigel_step`, `rigel_step_until`, `rigel_get_time`, `rigel_get_next_deadline`, `rigel_step_result_t`
- `rigel_bus.h` — Bus observation: `rigel_get_bus_state`, `rigel_bus_state_t`, `cpu_would_stall`, `next_change`
- `rigel_events.h` — Event bitmask (`rigel_event_flags_t`): IRQ_CHANGED, FRAME_READY, BLIT_DONE, VBLANK, etc.
- `rigel_mmio.h` — `rigel_custom_read16 / rigel_custom_write16`
- `rigel_irq.h` — `rigel_get_intreq / rigel_get_intena / rigel_get_ipl`

**Chipset layer** (`src/chipset/`) — composition and MMIO routing: `agnus/`, `denise/`, `paula/`.

**Domain layer** (`src/domains/`) — hardware state machines: beam, dma, copper, blitter, interrupt, disk, serial, audio, input.

## Key design constraints

- **Host owns global memory**: Rigel receives Chip RAM callbacks, never the map.
- **Determinism via stepping**: all state advances through `rigel_step` / `rigel_step_until`.
- **IRQ delivery is the host's job**: Rigel exposes IPL; host delivers to CPU on `RIGEL_EVENT_IRQ_CHANGED`.
- **Bus state is advisory**: `cpu_would_stall` reflects the classic chipset condition; the host applies it to its CPU core.
- **C11, no extensions**: `-Wall -Wextra -Wpedantic`.

## Temporal API — important notes

`rigel_step` returns `rigel_step_result_t` (not void). Code that ignores the return value compiles without changes.

`rigel_get_next_deadline()` is currently approximate (blitter `cycles_remaining` or 1 scanline fallback). The correct implementation requires a slot scheduler in Agnus and a complete beam model (line/frame wrapping). Mark new deadline contributions with `TODO` pointing to this.

## Video output (planned)

Rigel will expose `rigel_get_frame(ctx)` after `RIGEL_EVENT_FRAME_READY`. The pixel format is configured at `rigel_create` time. Planar→chunky conversion belongs in the chipset (it is Denise's execution, not a format conversion). Host-side presentation (scale, vsync, SDL/OpenGL) does not belong in Rigel.

## AI_context/

`AI_context/` contains design decisions and architectural notes from development sessions. Check it before making changes to public APIs or cross-cutting concerns:
- `temporal_bus_api.md` — Temporal API + Bus Observation design and rationale
- `dma_slot_timing.md` — DMA slot sequence as timing foundation, step approach tradeoffs
- `video_output.md` — video pipeline, frame struct, dirty tracking design

## Tests

Standalone C executables, plain `main()`, return 0 on success. Register new suites in `CMakeLists.txt` under `RIGEL_BUILD_TESTS`.

## Docs

`docs/` is the reference. Read before changing host-integration contracts:
- `architecture.md`, `integration.md`, `timing_model.md`, `video_output.md`, `irq_model.md`, `memory_map.md`
