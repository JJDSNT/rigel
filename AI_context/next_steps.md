# Next Steps

1. Consolidate the new internal layout
- finish the transition from legacy `src/agnus|denise|paula` to `src/chipset/*`
- keep the build green while cleaning naming residue and temporary compatibility glue
- avoid broad semantic changes while this structural move is still settling
- continue moving logic behind `src/domains/*` instead of growing `agnus_state.c` into a coordinator blob

2. Continue `paula_disk` migration
- improve `DSKBYTR` / `DSKDATR` / `DSKSYNC` / `ADKCON` fidelity
- start reusing more of the old track/sync path now that `FloppyDrive` is wired in
- decide how drive selection should flow from Paula/CIA into `DF0..DF3`
- keep the public floppy API stable while internals move from fixed `DF0` wiring to real selection

3. Introduce execution domains incrementally
- deepen the first wave that already exists:
  - `beam`
  - `dma`
  - `copper`
  - `blitter`
  - `interrupt`
  - `disk`
  - `serial`
  - `audio`
  - `input`
- keep them single-thread and deterministic
- use domains to clarify ownership and stepping, not to force early parallelism
- likely next detail:
  - move more Agnus-local policy out of `agnus_state.c`
  - define clearer per-domain reset/step/service hooks
  - keep checking whether each extracted domain removes real ambiguity or only adds file count

4. Expand `RigelPaula`
- keep each subdomain behind narrow surfaces like `paula_interrupts`
- next Paula work is now mostly deepening fidelity, not creating first-cut surfaces:
  - audio DMA fetch/service integration
  - richer serial host-facing integration
  - disk drive selection path from CIA/Paula to `DF0..DF3`
  - optional input/CIA integration cleanup now that `paula_old/` is gone

5. Deepen `RigelAgnus`
- introduce more explicit substructures:
  - `beam`
  - `copper`
  - `bitplanes`
- keep MMIO routing through Agnus-owned handlers

6. Revisit `RigelChipset`
- once Paula and Agnus have stronger ownership, reduce leftover shared state
- keep chipset as mediation/orchestration layer, not policy owner

7. RTC follow-up
- keep RTC outside custom MMIO and outside the custom-chip family
- decide later whether RTC needs a richer host-backed configuration surface
