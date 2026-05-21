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
  - `RigelPaula`
- `RigelAgnus` already owns:
  - `dmacon`
  - `BlitterState`
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
- RTC is now considered part of the library scope, but not part of the custom chip family.

# Working Paths

- Blitter path:
  - MMIO -> Agnus -> Blitter registers
  - DMA step -> IRQ publish -> chipset IRQ surface -> Paula interrupts

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
- only `DF0` is currently wired into `Paula disk` DMA; `DF1..DF3` are exposed to the host but not yet selected by chipset control paths.

# Notes

- `RigelChipset` intentionally mediates IRQ as a surface.
- Paula currently implements the IRQ policy behind that surface.
- `runtime` should not know chip-specific details; stepping is moving under `chipset`.
- `Rigel` is not being designed as multicore-first.
- `Rigel` should be concurrency-aware internally, but deterministic and single-thread for the classic chipset path.
- domains are meant to express ownership and temporal boundaries, not immediate thread boundaries.
- `RigelAgnus` now owns explicit `beam`, `dma`, `copper` and `blitter` state, with stepping routed through domain wrappers.
