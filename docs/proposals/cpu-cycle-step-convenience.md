# Convenience API: stepping by host CPU cycles

Status: implemented by `rigel_step_cpu_cycles()`.

## Intent

Rigel provides an **optional convenience API** that accepts a delta of host CPU cycles and advances the existing chipset clock accordingly. Rigel remains CPU-agnostic: there is no dependency on Moira, a particular 68k model, JIT, or any host OS.

Public interface:

```c
rigel_step_result_t rigel_step_cpu_cycles(RigelContext *ctx, rigel_u64 cpu_cycles);
```

`rigel_config_t.cpu_cycle_ratio` expresses `cpu_cycles : color_clocks`. A
zeroed pair selects the conventional Amiga ratio of `2:1`. Supplying only one
zero is invalid and makes `rigel_create()` fail. This is a convenience wrapper
around the existing temporal API, **not** a second scheduler or a change to the
chipset's internal time base. `rigel_step()`, `rigel_step_until()`, and
`rigel_get_time()` retain their existing semantics.

## Semantics

- Fractional conversions retain a per-context remainder, so repeated small
  increments do not lose time through integer truncation.
- `rigel_reset()` clears that remainder.
- Direct CCK stepping does not alter the CPU-cycle remainder. Hosts may mix the
  APIs when applying an independent chipset-time correction without losing the
  CPU/CCK phase.
- Large inputs are split into bounded internal steps. Events are ORed across
  those steps and the returned time is the final Rigel time.

## Non-goals

No CPU emulation integration, CPU-specific policy, chipset-domain redesign, scheduler replacement, or requirement to implement cycle-exact CPU/chipset synchronization as part of this convenience API.
