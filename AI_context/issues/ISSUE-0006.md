---
id: ISSUE-0006
title: "The per-colour-clock loop has no event skipping, and the idle floor is 140 ns/CCK"
status: open
priority: high
type: performance
owner: unassigned
created_at: 2026-08-29
updated_at: 2026-08-29
tags:
  - performance
  - agnus
  - denise
  - scheduler
  - measurement
related_files:
  - src/chipset/chipset.c
  - src/core/rigel.c
  - src/chipset/agnus/agnus_slot_scheduler.c
  - src/chipset/denise/
---

> **Correction, 2026-08-29, same day: the headline numbers below were measured
> against an unoptimised build and are wrong by about 4x.** `out/rigel-harness`
> on the Bellatrix side had an empty `CMAKE_BUILD_TYPE` and empty
> `CMAKE_C_FLAGS`, so both the idle bench and the Demo Reel 3 timing ran at
> `-O0`. Rebuilt with `-DCMAKE_BUILD_TYPE=Release`:
>
> | | published | correct |
> |---|---|---|
> | idle floor | 140 ns/CCK, 2x realtime | **35 ns/CCK, 8x realtime** |
> | Demo Reel 3, 600 frames | 6.92 s, 86.7 fps, 162 ns/CCK | **2.61 s, 229.9 fps, 61 ns/CCK** |
>
> What survives: the `gprof` ranking below, which came from a separate `-O2 -pg`
> build and is a ranking rather than an absolute; the shape of the finding, that
> a real workload costs only modestly more than an idle one (61 against 35, so
> +74% rather than +16%, still nothing like proportional to what is programmed);
> and that the loop has no event skipping.
>
> What does **not** survive: the claim that an idle chipset has only 2x headroom
> on a desktop, and everything derived from it. In particular the "~3.8x short
> on a Pi 3" conclusion rested on an earlier ~1080 ns/CCK figure for the Pi,
> which against 35 ns/CCK native would make an A53 31x slower than a modern x86
> -- implausible, so that figure is now suspect too and **the Pi must be
> re-measured before any target is set from it**.
>
> The body below is left as written, with its numbers wrong, because the
> mistake is the useful part: a performance gate was opened with a measurement
> whose build flags were never checked.

## Why this issue exists

`AI_context/from_bellatrix/rigel_performance_research.md` sets a gate: no
chipset performance work becomes active without "uma medição que demonstre
gargalo interno no Rigel", separated by wall-time, call count, CCK per call,
and per-domain cost. This is that measurement, taken from the Bellatrix side on
2026-08-29. It opens the gate.

## The floor: 140 ns per colour clock with nothing programmed

A native x86 bench: `rigel_create()` with 2 MB chip RAM, no ROM, no disk, the
chipset in reset, then step one second of NTSC frames. Nothing is programmed --
DMACON is clear, no bitplanes, no copper list, no blitter, no screen.

```text
deadline-bounded    3568440 CCK ->  7.13 M CCK/s  (201% of realtime, 140 ns/CCK)
quantum 512         3568640 CCK ->  7.16 M CCK/s  (202% of realtime, 140 ns/CCK)
quantum 1           3568440 CCK ->  3.02 M CCK/s  ( 85% of realtime, 331 ns/CCK)
one big step        3568440 CCK ->  7.20 M CCK/s  (203% of realtime, 139 ns/CCK)
```

Realtime is 282 ns/CCK. **An idle Rigel has only 2x headroom on a modern x86
desktop.** The repro is in
`AI_context/from_bellatrix/rigel_cck_cost_measurement.md`, which carries the
bench source.

## Under load the floor still dominates

Demo Reel 3 through `harness/`, KS13, 512K slow RAM, headless, 600 frames --
Kickstart booting and the demo running, with Musashi emulating the CPU as well:

```text
rigel-harness: 600 frames, 84988804 CPU cycles     wall 6.92 s
```

84988804 / 600 = 141648 CPU cycles per frame, a full-speed PAL frame, so this is
a genuine run. 600 PAL frames is 42.6 M CCK:

- 86.7 fps, 1.73x realtime, **162 ns/CCK**
- against the 140 ns/CCK idle floor, **a real workload costs only 16% more**

That is the most important number here. Rigel's cost is almost entirely fixed
per colour clock and almost independent of what is programmed, so the idle
profile below is a fair map of where a loaded machine spends its time, and
optimising the loaded case means optimising the empty one.

## Where the time goes

`gprof`, same idle workload, per-CCK call counts confirmed against the total
(14273960 calls over 14273760 CCK across the four runs = 1.000 per CCK):

```text
 14.5%  agnus_slot_scheduler_step                1    per CCK
 13.2%  rigel_denise_framebuffer_sync_from_beam  1    per CCK
 10.5%  beam_step                                1    per CCK
  7.9%  blitter_is_busy                          2.25 per CCK
  6.6%  refresh_dma_owns_slot                    1    per CCK
  5.3%  rigel_copper_domain_step                 1    per CCK
  5.3%  rigel_denise_compositor_tick             1    per CCK
  2.6%  rigel_dma_domain_read_dmacon             1    per CCK
  2.6%  beam_in_vblank                           0.96 per CCK
```

Every colour clock asks every domain the same question, and gets the same answer
every time. About a quarter of the total goes to Denise framebuffer sync,
compositor ticks and repeated `blitter_is_busy()` calls, for a screen with
nothing on it and a blitter that never runs.

`rigel_get_next_observable_deadline()` already exists and hosts already use it
-- Bellatrix steps to it -- but it only bounds the *caller's* quantum. Inside
`rigel_chipset_step()` the loop still walks every clock. Hypothesis 1 in
`rigel_performance_research.md` ("scheduler orientado a eventos") is the direct
answer, and this measurement is the evidence it was waiting for.

## The per-call cost is separate, and it is workload-dependent

`rigel_step()` reads eight pieces of state before calling
`rigel_chipset_step()` and compares them after, including two
`rigel_paula_interrupts_current_ipl()` priority resolutions and a
`blitter_is_busy()` poll, to synthesise the event mask. That is roughly 190 ns
of fixed cost per call: `quantum 1` costs 331 ns/CCK against 140.

It does not bite while the chipset is idle -- the deadline-bounded run matches
the one-big-step run, so with nothing programmed the deadlines are far apart.
But a host that steps once per observable deadline pays it exactly when the
deadlines get close together, which is when a copper list is running or a
blitter is busy. The idle measurement cannot see this; a loaded one should be
taken before deciding how much it matters.

## What this means for a host

Scaling the 140 ns/CCK floor by the Raspberry Pi 3 figure recorded in earlier
Bellatrix work (~1080 ns/CCK) gives ~13 fps PAL where 50 is needed: the gap is
**~3.8x**. That is a tuning target with a finish line, not a rewrite. It is also
the reason Bellatrix cannot currently boot a machine with the chipset live --
110 seconds of wall clock got as far as `intuition.library`, where a
chipset-less boot reaches services in under seven.

Caveat: the Pi figure is from earlier work, not a fresh measurement. It should
be re-taken on hardware before 3.8x is treated as a contract.

## Constraint on any fix

Bellatrix's standing rule: **no Rigel performance change may regress
cycle-exactness by default.** Every optimisation arrives as a flag or a mode,
with the exact path still reachable and still the default. `tools/tests/timing/`
and the KS1.3/Battle Squadron compatibility gates are what keep that honest.

## Suggested order

1. **Event skipping inside `rigel_chipset_step()`** -- when no domain has work
   before the next observable deadline, advance the beam and the derived
   counters in closed form instead of iterating. This is where the 3.8x is.
2. **The cheap cuts this profile names** -- `rigel_denise_framebuffer_sync_from_beam`
   and `rigel_denise_compositor_tick` when there is nothing to draw (18.5%
   together), and `blitter_is_busy()` called 2.25 times per clock for a blitter
   that is not running (7.9%).
3. **Re-measure with a loaded profile** before touching the per-call cost of
   `rigel_step()`, since the idle case cannot show it.
