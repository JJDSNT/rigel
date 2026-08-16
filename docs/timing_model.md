# Timing Model

## Foundation

Time in Rigel is a monotonic cycle counter (`rigel_cycle_t`, `uint64_t`).
Everything the chipset produces — IRQs, video, DMA, copper — is an event on
this time axis. The host advances time explicitly; Rigel never advances on its own.

### The unit is the colour clock

**`rigel_step` counts CCK — colour clocks, ~3.55 MHz — not CPU cycles.** A
scanline is 227 of them in both video standards, which is what
`rigel_get_line_cycles` returns; the standards differ in line count, not line
length.

`rigel_get_clock_hz` reports the *CPU-side* rate, twice the step rate. The two
are easy to mix, and mixing them is silently wrong by a factor of two:

```c
uint32_t cck_hz = rigel_get_clock_hz(rigel) / 2u;   /* the step unit */
```

Both are configurable — `rigel_config_t.clock_hz` and `.video_std` — and the
geometry follows the standard correctly: PAL gives 312 lines and 70824 CCK per
frame, NTSC 262 and 59474, each within a microsecond of the real frame time
once converted at `cck_hz`. Converted at `clock_hz` instead, both come out at
half, and a drift correction built on that never converges.

One thing `clock_hz` does not do is track `video_std`: it returns the PAL
constant unless the host sets it, so an NTSC host should configure
`clock_hz = 7159090` explicitly rather than rely on the default.

A 68000 host has the same halving to do in the other direction — CPU cycles
into CCK, carrying the odd cycle so time is not lost across timeslices. See
`harness/harness.c` for a host that does it, and `AI_context/harness.md` for
what it looks like when it does not.

## DMA slot sequence as the timing heartbeat

On the Amiga, Agnus divides each raster line into fixed slots. At every CCK it
decides who owns the bus: refresh, disk, audio, bitplanes, sprites, copper/blitter,
CPU. This is not just DMA scheduling — it is what advances the beam, fires VBL/HBL,
cadences audio periods, and synchronises the copper.

A scanline has ~227 CCKs (NTSC) or ~284 CCKs (PAL). The beam advances slot by slot.
Events are positions in the slot sequence, not raw cycle durations.

## Temporal API

```c
rigel_cycle_t       rigel_get_time(const RigelContext *ctx);
rigel_cycle_t       rigel_get_next_deadline(const RigelContext *ctx);
rigel_cycle_t       rigel_get_next_observable_deadline(const RigelContext *ctx);
rigel_step_result_t rigel_step(RigelContext *ctx, rigel_cycle_t cycles);
rigel_step_result_t rigel_step_until(RigelContext *ctx, rigel_cycle_t target_time);
```

`rigel_get_next_deadline()` answers: "how far can I advance without missing a
mandatory synchronisation point?" This lets the host run the CPU exactly up to
the next relevant event without an arbitrary fixed step.

`rigel_get_next_observable_deadline()` excludes internal DMA-slot boundaries.
Use it when the host only needs externally visible events and calls
`rigel_step()` for the interval; the internal slot scheduler still processes
every CCK. Hosts that arbitrate the chip bus slot-by-slot must continue using
`rigel_get_next_deadline()` and `rigel_get_next_bus_change()`.

`rigel_step_result_t` carries the current time and an event bitmask:

```c
typedef struct rigel_step_result {
    rigel_cycle_t time;
    rigel_u32     events; /* rigel_event_flags_t */
} rigel_step_result_t;
```

Notified events: `IRQ_CHANGED`, `BLIT_DONE`, `VBLANK`, `HBLANK`, `FRAME_READY`,
`AUDIO_READY`, `COPPER`, `DMA_CHANGED`, `BUS_CHANGED`, `DEADLINE`.

### Read-only beam projection

Multicore hosts can copy the current scan geometry and project VPOS/HPOS at a
later chipset time without stepping or reading the context:

```c
rigel_beam_geometry_t beam = rigel_get_beam_geometry(rigel);
rigel_u16 vpos, hpos;
bool exact = rigel_beam_position_at(&beam, cpu_time, &vpos, &hpos);
```

The projection is exact for uniform progressive scan. It returns `false` for
interlace, long-line toggling, transient LOF/LOL state, invalid snapshots, or a
time before the snapshot. Hosts must then synchronize with the Rigel owner and
read VPOSR/VHPOSR normally. The copied snapshot contains no context pointer and
is safe to publish between host threads.

## Step approaches

### A — Cycle-step (current)

The host advances N cycles. `next_deadline` is approximate (blitter
`cycles_remaining` or 1-scanline fallback). Simple; works for hosts without
fine bus integration.

### Cost fidelity: `cycle_exact`

Separate from how the host steps, `rigel_config_t.cycle_exact` selects the cost
model — the honest hybrid, with hardware-calibrated per-word blitter channel
costs and the line's two-cycle cadence, against a coarser estimate. It can also
be set at runtime with `rigel_set_cycle_exact`.

**It defaults off, and a host that wants hardware-faithful timing must turn it
on.** The difference is not marginal. Measured against a cross-emulator timing
reference (`tools/tests/timing/`):

| Operation | coarse | `cycle_exact` |
| --- | --- | --- |
| blitter clear | 0.50 | **1.00** |
| blitter fill | 0.33 | **0.99** |
| fill + 3 bitplanes | 0.33 | **0.99** |
| blitter line | 0.47 | 0.73 |

No other measured row moves. Bellatrix ran with it on and treated turning it
off as a development-only A/B, which is the shape to copy: on for anything
whose timing is being judged, off only to compare against.

### B — Slot-step (planned)

```c
rigel_step_result_t rigel_step_slot(RigelContext *ctx); /* 1 DMA slot */
```

Requires a fully modelled beam (line + frame + VBL/HBL zones) and the Agnus
slot scheduler. `next_deadline` becomes exact rather than estimated.

### C — Event-driven over slots (target)

The public API does not change. Internally, `step_until` walks slot by slot.
`next_deadline` delegates to the slot scheduler. The public interface is
approach A; the internal mechanism is B.

The slot scheduler (`agnus_slot_scheduler_t`) is already implemented and drives
disk, audio, bitplane, copper, and blitter dispatch. The remaining work for full
Approach C is wiring `next_deadline` to the scheduler instead of the blitter
cycles estimate.

## Frame pacing and drift correction

Rigel is the reference clock — it does not correct drift. The host measures real
time, compares it with simulated time, and applies its own correction policy.
Rigel exposes the facts the host needs:

```c
uint32_t rigel_get_clock_hz(const RigelContext *ctx);    /* CPU rate: 7093790 PAL / 7159090 NTSC — halve for the step unit */
uint32_t rigel_get_line_cycles(const RigelContext *ctx); /* cycles per current scanline */
uint32_t rigel_get_frame_cycles(const RigelContext *ctx); /* cycles per complete frame */
uint64_t rigel_cycles_to_us(rigel_cycle_t cycles, uint32_t clock_hz);
rigel_cycle_t rigel_us_to_cycles(uint64_t microseconds, uint32_t clock_hz);
```

Canonical loop:

```c
uint32_t cck_hz = rigel_get_clock_hz(rigel) / 2u;   /* step unit, not clock_hz */

uint64_t frame_start = host_get_time_us();
rigel_step_until(rigel, rigel_get_time(rigel) + rigel_get_frame_cycles(rigel));
int64_t drift = (int64_t)(host_get_time_us() - frame_start)
              - (int64_t)rigel_cycles_to_us(rigel_get_frame_cycles(rigel), cck_hz);
host_apply_frame_correction(drift);
```

Correction policy (sleep, skip, fast-forward) belongs to the host.
