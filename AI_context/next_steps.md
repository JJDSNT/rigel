# Next Steps

1. Continue `paula_disk` migration
- improve `DSKBYTR` / `DSKDATR` / `DSKSYNC` / `ADKCON` fidelity
- start reusing more of the old track/sync path now that `FloppyDrive` is wired in
- decide how drive selection should flow from Paula/CIA into `DF0..DF3`
- keep the public floppy API stable while internals move from fixed `DF0` wiring to real selection

2. Expand `RigelPaula`
- choose next subdomain:
  - `serial`
  - `audio`
- keep each subdomain behind narrow surfaces like `paula_interrupts`

3. Deepen `RigelAgnus`
- introduce more explicit substructures:
  - `beam`
  - `copper`
  - `bitplanes`
- keep MMIO routing through Agnus-owned handlers

4. Revisit `RigelChipset`
- once Paula and Agnus have stronger ownership, reduce leftover shared state
- keep chipset as mediation/orchestration layer, not policy owner
