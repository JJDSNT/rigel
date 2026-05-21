# Next Steps

1. Continue `paula_disk` migration
- improve `DSKBYTR` / `DSKDATR` / `DSKSYNC` / `ADKCON` fidelity
- decide when to introduce real media/drive abstraction

2. Expand `RiegelPaula`
- choose next subdomain:
  - `serial`
  - `audio`
- keep each subdomain behind narrow surfaces like `paula_interrupts`

3. Deepen `RiegelAgnus`
- introduce more explicit substructures:
  - `beam`
  - `copper`
  - `bitplanes`
- keep MMIO routing through Agnus-owned handlers

4. Revisit `RiegelChipset`
- once Paula and Agnus have stronger ownership, reduce leftover shared state
- keep chipset as mediation/orchestration layer, not policy owner
