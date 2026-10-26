# Convenience API: stepping by host CPU cycles

## Intent

Add an **optional convenience API** to Rigel that accepts a delta of host CPU cycles and advances the existing chipset clock accordingly. Rigel must remain CPU-agnostic: no dependency on Moira, a particular 68k model, JIT, or any host OS.

Illustrative public interface (names subject to implementation):

```c
rigel_step_result_t rigel_step_cpu_cycles(RigelContext *ctx, rigel_u64 cpu_cycles);
```

Provide a **configurable conversion ratio** from host cycles to Rigel's existing CCK units, with a sensible default for conventional Amiga timing. This is a convenience wrapper around the existing temporal API, **not** a second scheduler or a change to the chipset's internal time base. Keep `rigel_step()`, `rigel_step_until()`, and `rigel_get_time()` semantics unchanged.

## Implementation considerations (for the implementing agent)

- Fractional conversions: repeated small increments should not lose time through integer truncation. A per-context remainder is one possible solution.
- Reset and configuration: decide how the conversion state is initialized and reset, and validate the configured ratio.
- API interleaving: define/document behavior when the caller mixes CCK stepping with host-cycle stepping.
- Large increments: the existing `rigel_step()` implementation casts its delta to `rigel_u32` before calling `rigel_chipset_step()`; consider this when implementing the wrapper. Any general correction can be handled separately if appropriate.
- Events: retain the established `rigel_step_result_t` contract; consider whether splitting large steps affects event reporting.
- Tests: cover a default ratio, custom ratio, small repeated increments, reset, and compatibility with existing stepping.
- Documentation: update the relevant temporal API/integration docs once the implementation is settled.

These are **review points, not mandated design decisions**. Prefer the smallest implementation consistent with Rigel's current architecture. The implementing agent should inspect the actual code, choose appropriate names and mechanics, and avoid broad refactors or unrelated timing work.

## Non-goals

No CPU emulation integration, CPU-specific policy, chipset-domain redesign, scheduler replacement, or requirement to implement cycle-exact CPU/chipset synchronization as part of this convenience API.
