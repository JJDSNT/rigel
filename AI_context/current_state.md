# Current State

- `RiegelContext` is the public host-facing object.
- `RiegelChipset` is the internal root object.
- `RiegelChipset` currently composes:
  - `RiegelAgnus`
  - `RiegelPaula`
- `RiegelAgnus` already owns:
  - `dmacon`
  - `BlitterState`
- `RiegelPaula` already owns:
  - interrupt model (`INTREQ`, `INTENA`, `IPL`)
  - `audio` state stub
  - `disk` state with MMIO-visible registers and minimal DMA service
  - `serial` state stub

# Working Paths

- Blitter path:
  - MMIO -> Agnus -> Blitter registers
  - DMA step -> IRQ publish -> chipset IRQ surface -> Paula interrupts

- Paula disk path:
  - MMIO -> Paula -> disk registers
  - disk DMA service -> chip RAM interface write -> IRQ publish
  - no-media fallback -> countdown -> fake `DSKBLK` IRQ

# Disk Notes

- `disk_reset()` preserves external attachments/configuration:
  - chip RAM interface
  - IRQ sink
  - synthetic media-present flag
- `disk.inserted` currently acts as a minimal "data source present" flag:
  - `inserted = 1` enables DMA service grants
  - `inserted = 0` falls back to no-media countdown behavior
- `riegel_create()` currently enables synthetic disk presence when a chip RAM write callback exists.

# Notes

- `RiegelChipset` intentionally mediates IRQ as a surface.
- Paula currently implements the IRQ policy behind that surface.
- `runtime` should not know chip-specific details; stepping is moving under `chipset`.
