# Current State

- `RigelContext` is the public host-facing object.
- `RigelChipset` is the internal root object.
- `RigelChipset` currently composes:
  - `RigelAgnus`
  - `RigelPaula`
- `RigelAgnus` already owns:
  - `dmacon`
  - `BlitterState`
- `RigelPaula` already owns:
  - interrupt model (`INTREQ`, `INTENA`, `IPL`)
  - `audio` state stub
  - `disk` state with MMIO-visible registers and minimal DMA service
  - `serial` state stub
- internal `floppy` module now exists as a separate peripheral:
  - `FloppyDrive`
  - MFM track encoder helpers
- public floppy surface now exists in the API:
  - `DF0..DF3`
  - insert/eject
  - status query per drive

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
