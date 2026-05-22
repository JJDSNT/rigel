# Agnus

`Agnus` remains the timing and DMA-facing core of the classic chipset inside `Rigel`.

Current direction:

- `Agnus` owns beam/raster timing, DMA cadence, arbitration, copper stepping, blitter progression, and fetch-side concerns
- `Denise` consumes already-timed state and handles visual composition
- `Paula` handles IRQ policy, disk-controller logic, audio finalization, and serial/input behavior

Important rule:

- there is no visual `sprites/` subsystem inside `Agnus`
- sprite fetch belongs to `Agnus` DMA/timing
- sprite interpretation and composition belong to the visual side

The important structural rule is:

- `src/domains/` remains the canonical place for shared hardware state machines such as beam, DMA, copper, and blitter coordination
- `src/chipset/agnus/` stays focused on Agnus-facing composition, MMIO entrypoints, state ownership, and Agnus-specific implementation that does not belong in a generic domain

In practice, this means Agnus should not grow a large parallel tree that duplicates what already lives in `domains/`.
