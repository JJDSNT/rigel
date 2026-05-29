# Architecture

## Overview

Rigel is a C library for the classic Amiga chipset, designed for temporal fidelity,
ownership separation, and clean integration with an external host.

Core goals:
- Deterministic behaviour
- Single-threaded execution by default
- Clear internal boundaries between subsystems
- Portability across hosts and runtimes

Rigel is concurrency-aware internally but not multicore-first. For the classic path,
correctness and determinism matter more than anticipated parallelism.

## Internal layers

```
include/rigel/       ← public surface (host speaks only here)
src/chipset/         ← composition, MMIO routing, internal wiring
src/domains/         ← hardware state machines
src/bus/             ← access interfaces and Chip RAM callbacks
src/runtime/         ← host execution policy
src/rtc/             ← auxiliary peripheral outside custom MMIO
src/debug/           ← optional host logging and structured trace routing
src/simd/            ← optional internal SIMD helpers with scalar fallback
```

## Public surface

Organized into thematic headers, all re-exported by `rigel.h`:

| Header                  | Contents                                            |
|-------------------------|-----------------------------------------------------|
| `rigel_time.h`          | Temporal API: step, deadline, step_result           |
| `rigel_bus.h`           | Bus observation: classic bus state                  |
| `rigel_events.h`        | Event bitmask (`rigel_event_flags_t`)               |
| `rigel_irq.h`           | INTREQ, INTENA, IPL                                 |
| `rigel_mmio.h`          | custom_read16 / custom_write16                      |
| `rigel_audio.h`         | Mixed stereo sample (`rigel_get_audio_sample`)      |
| `rigel_denise_video.h`  | Frame and scanline output (`rigel_get_frame`, etc.) |
| `rigel_floppy.h`        | Insert, eject, drive status                         |
| `rigel_input.h`         | Joystick / pot injection                            |
| `rigel_rtc.h`           | RTC model and registers                             |
| `rigel_config.h`        | Context creation configuration                      |
| `rigel_snapshot.h`      | Save / load state (preliminary)                     |
| `rigel_custom.h`        | Register validity and domain query helpers          |

`rigel_config_t` also carries optional logging hooks. Text logs use
`log_fn`; runtime traces use `log_event_fn` with structured numeric fields so
bare-metal hosts do not need `stdio` formatting in chipset paths.

## Temporal API

Two main contracts:

**"when does something relevant happen" → for scheduling:**
```c
rigel_cycle_t       rigel_get_time(const RigelContext *ctx);
rigel_cycle_t       rigel_get_next_deadline(const RigelContext *ctx);
rigel_step_result_t rigel_step(RigelContext *ctx, rigel_cycle_t cycles);
rigel_step_result_t rigel_step_until(RigelContext *ctx, rigel_cycle_t target_time);
```

**"who owns the bus right now" → for contention and wait states:**
```c
rigel_bus_state_t rigel_get_bus_state(const RigelContext *ctx);
rigel_cycle_t     rigel_get_next_bus_change(const RigelContext *ctx);
bool              rigel_cpu_can_access_chip_ram(const RigelContext *ctx);
bool              rigel_cpu_can_access_custom(const RigelContext *ctx);
rigel_cycle_t     rigel_get_cpu_resume_time(const RigelContext *ctx);
```

`cpu_would_stall` is a field in `rigel_bus_state_t`, not a standalone function.

## Domains

`src/domains/` contains the classic hardware state machines:

```
beam       → raster position (hpos/vpos)
dma        → DMACON, slot arbitration
copper     → copper program and WAITs
blitter    → operations and blitter DMA
interrupt  → INTREQ, INTENA, IPL
disk       → disk DMA and MMIO
serial     → SERDAT, SERPER, TX/RX
audio      → 4 channels, DMA, period
input      → joystick, POT
```

Domains exist to separate state, make temporal boundaries explicit, and reduce
coupling. They are not threads — they are ownership surfaces.

## Chipset layer

`src/chipset/` is the compositional root. It holds:
- `RigelChipset`: root context containing Agnus, Paula, Denise, RTC, FloppyDrives
- MMIO routing: `rigel_custom_read16/write16` → correct domain
- Per-chip MMIO entrypoints
- Internal wiring: IRQ sink, DMA grant, Chip RAM callbacks
- Mediation: domains do not depend on each other directly

```
RigelChipset
  ├── RigelAgnus
  │     ├── beam_state_t
  │     ├── dma_state_t
  │     ├── copper_state_t
  │     ├── bitplanes_state_t
  │     └── BlitterState
  ├── RigelDenise
  │     ├── registers
  │     ├── palette
  │     ├── render
  │     ├── sprites
  │     ├── video
  │     └── output/debug
  ├── RigelPaula
  │     ├── RigelInterruptDomain
  │     ├── audio domain
  │     ├── disk domain
  │     ├── serial domain
  │     └── input domain
  ├── RigelRTC
  └── FloppyDrive[4]
```

## DMA slot scheduler

Agnus divides each raster line into fixed slots. The slot scheduler
(`agnus_slot_scheduler_t`) dispatches the correct domain at each CCK:

| Slot type  | Domain call                            |
|------------|----------------------------------------|
| REFRESH    | transparent, no domain call            |
| DISK       | `rigel_disk_domain_step_slot(ctx)`     |
| AUDIO_0–3  | `rigel_audio_domain_step_slot(ctx, n)` |
| SPRITE_0–7 | sprite DMA (TODO)                      |
| BITPLANE   | `bitplane_fetch_step()`                |
| COPPER     | `rigel_copper_service_step_program()`  |
| BLITTER    | `rigel_agnus_blitter_step_dma()`       |

The slot table is rebuilt whenever DMACON changes or a new line starts.
Disk and audio DMA require `DMAEN` (master enable) plus their channel
enable bits in DMACON; the slot scheduler enforces this automatically.

## Video output

Denise produces pixels from bitplanes, sprites, HAM/EHB and palette.
This logic belongs to the chipset because planar→chunky is not a format
conversion — it is Denise's actual execution during the DMA slot sequence.

The host receives a `rigel_frame_t` with ready pixels, metadata (`flags`) and
dirty tracking (`delta`). See `docs/video_output.md`.

Internal SIMD helpers may accelerate buffer fills/copies in the video path, but
they are optional implementation detail (`RIGEL_ENABLE_SIMD`) and do not change
timing, public structs, or host-visible semantics.

## Boundary

Rigel does not own:
- the global memory map
- the CPU
- ROM / Fast RAM
- platform devices
- presentation logic (scale, real vsync, SDL, OpenGL)

The host integrates all of that. See `docs/integration.md`.
