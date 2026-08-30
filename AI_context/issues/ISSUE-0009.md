---
id: ISSUE-0009
title: "The host boundary was designed before a host existed: what a real integration found missing"
status: open
priority: high
type: research
owner: unassigned
created_at: 2026-08-30
updated_at: 2026-08-30
tags:
  - api
  - host-integration
  - time
  - design
related_files:
  - include/rigel/rigel_time.h
  - include/rigel/rigel_audio.h
  - src/core/rigel.c
---

## Why this issue exists

The public API was drawn on the assumption that a host would not need anything
else. A host now exists -- Bellatrix drives this chipset on bare metal, with a
JIT, on hardware that is 37x slower than a desktop -- and the assumption has
been tested. It did not entirely hold.

This issue collects what a real integration found missing, and proposes a shape
for filling it that does not give the assumption away.

## The principle worth keeping

The one thing that should not change is the rule that made the boundary good in
the first place:

> Rigel owns chipset semantics. The host owns the bridge.

Every gap below could be closed by exposing internal state -- DMACON, the CIA
timer registers, the slot table -- and every one of those would move a piece of
Rigel's reasoning into a host that will get it subtly wrong. Bellatrix's own
notes record what that costs: a shadow of intercepted register writes is a
second source of truth whose failure mode is a guest waiting forever for an
interrupt that stopped being generated.

**So the proposal is not "expose internals". It is "answer questions".** A host
should be able to ask Rigel what it needs to know, and Rigel should answer from
the state it already owns.

## 1. The deadline does not mean what it says

`rigel_get_next_observable_deadline()` exists so a host can skip ahead. It
cannot fulfil that: `d.beam_line_end` is assigned unconditionally in
`rigel_get_deadline()`, so `agnus_deadlines_min()` never returns more than a
scanline -- 227 colour clocks -- no matter what is programmed.

Measured on a real host: 226 colour clocks per call, against its own 512
ceiling that is never reached. The deadline is what caps every host, and it
caps them at a line.

This is a contract question rather than a missing accessor. Either the line end
is genuinely observable and the name is wrong, or it is an internal boundary
that `rigel_step()` can cross by itself -- in which case it belongs in
`rigel_get_next_deadline()` with the other internal slot work, which is exactly
the distinction the header already draws:

```c
/* Next host-observable event. Unlike rigel_get_next_deadline(), this excludes
 * internal DMA-slot boundaries that rigel_step() already processes itself. */
```

## 2. There is no way to ask whether anything needs time

Bellatrix arms its chipset clock lazily and wanted to disarm it symmetrically.
It cannot, and the reason is instructive: the API exposes `rigel_get_intena`,
`rigel_get_intreq`, `rigel_get_ipl`, the deadlines and the frame, and nothing
that says whether any domain is running.

The host's only route would be to shadow the register writes it intercepts,
which is the second-source-of-truth problem above.

What it actually needs is one question, answered by the side that knows:

```c
/* Is there anything whose observable state changes if time passes? */
bool rigel_has_pending_work(const RigelContext *ctx);
```

with, ideally, a hint of what -- so a host can log why it is paying, which is
the difference between a performance mystery and a performance finding.

Note this is not the same as the deadline being far away. A chipset with a CIA
timer running has work pending forever, and that is the case that matters:
Bellatrix's measured boot pays 1365 ns per colour clock for a machine whose
only time-dependent thing is a CIA timer, which `rigel_chipset_step()` already
advances in bulk once per call. See ISSUE-0006.

## 3. Paula is only available pre-mixed

Already ISSUE-0007. It belongs in this list because it has the same shape: the
host has its own mixer and wants the four voices' *state* -- location, length,
period, volume, DMA -- not Rigel's rendered output. Asking rather than reading.

## 4. Denise is only available as finished pixels

Already ISSUE-0008, and deliberately low priority. Same shape again, and the
same caution: it is only worth doing if rendering per scanline segment inside
Rigel (ISSUE-0006) does not remove the reason to want it.

## Relationship to the API convergence plan

The plan in `from_bellatrix/rigel_api_convergence_plan.md` is **not a rejected
document**. This directory's README says it "was removed" from Rigel's `docs/`,
and that is misleading: it was relocated. The live copy is Bellatrix's
`docs/Convergence.md`, and it is the standing design frame.

These four items are not a competing proposal. They are measurements filling in
interfaces the plan states only as principle:

| plan | states | this issue adds |
| --- | --- | --- |
| §35 Timing | Rigel is authoritative for chipset time; caches, shadows and fast paths must not create an independent chipset timeline | items 1 and 2 -- the deadline cannot serve skipping, and there is no way to ask whether anything needs time |
| §45 Video | Rigel owns classic video generation and produces host-consumable output | item 4 (ISSUE-0008) |
| §46 Audio | Paula through Rigel to host-independent audio output | item 3 (ISSUE-0007) |

§35 is worth reading before implementing item 2, because it already forbids the
route a host would otherwise take. Bellatrix tried to disarm its chipset clock
by shadowing the DMACON and CIA writes it intercepts, and abandoned it as a
second source of truth -- which is precisely "caches, shadows, and fast paths
must not create an independent chipset timeline", arrived at from the other
direction. **The plan predicted the failure; the measurement confirmed it.**
That is an argument for the plan, and for answering the question inside Rigel
rather than reconstructing it outside.

## Priority

Not urgent in the way ISSUE-0006 is. Items 1 and 2 make a host's
time accounting honest and cheap; ISSUE-0006 is what makes the chipset fast
enough to matter. If only one is done, it should be that one.
