# Next Steps

## Near-Term Targets

1. Deepen fidelity where the surfaces already exist
- `paula_disk`: improve `DSKBYTR`, `DSKDATR`, `DSKSYNC`, `ADKCON`, and real drive-selection flow into `DF0..DF3`
- `audio`: replace minimal stepping with clearer DMA fetch/service behavior
- `serial`: keep the current MMIO/IRQ path, then decide what host-facing serial bridge should look like
- `beam` / `copper`: move more timing policy out of `agnus_state.c` and into domains that already exist

2. Keep the domain split honest
- every extracted domain must reduce ambiguity, not just add files
- keep classic chipset execution single-thread and deterministic
- use `domains/` for ownership, stepping, and state boundaries, not for premature parallelism
- prefer narrow hooks such as `reset`, `step`, `owns_reg`, `read_reg`, `write_reg`, and service/grant helpers

3. Finish structural cleanup without reopening core API decisions
- continue cleaning residue from the older layout while keeping build and tests green
- keep `RigelChipset` as orchestration and mediation, not as a policy blob
- reduce leftover shared state as Agnus and Paula domains gain stronger ownership

## Medium-Term Targets

1. Strengthen Agnus composition
- make `beam`, `dma`, `copper`, `blitter`, and later `bitplanes` more explicit in ownership and stepping order
- keep MMIO routing through Agnus-facing handlers while moving behavior into domains

2. Strengthen Paula composition
- keep `interrupt`, `disk`, `serial`, `audio`, and `input` behind narrow surfaces
- prefer deepening fidelity over creating new first-cut submodules

3. Keep RTC and convenience peripherals separate from custom MMIO
- RTC remains part of `Rigel`, but not part of the custom-chip register family
- floppy, input, and RTC APIs should stay host-facing and explicit instead of leaking internal state layout

## Architectural Rule Of Thumb

- `Rigel` should remain hardware-facing, deterministic, and single-thread by default
- the library should stay concurrency-aware internally, but multicore execution is not a near-term goal for the classic chipset path
