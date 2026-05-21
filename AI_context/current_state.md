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

# Notes

- `RiegelChipset` intentionally mediates IRQ as a surface.
- Paula currently implements the IRQ policy behind that surface.
- `runtime` should not know chip-specific details; stepping is moving under `chipset`.
