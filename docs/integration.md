# Integration

## Scope

Rigel integrates with an external host without depending on a CPU, frontend, or
platform. The host is responsible for: global memory, address decoding, the CPU,
ROM, Fast RAM, and the entire presentation layer.

## Canonical host loop

```c
while (running) {
    rigel_cycle_t deadline   = rigel_get_next_deadline(rigel);
    rigel_cycle_t bus_change = rigel_get_next_bus_change(rigel);
    rigel_cycle_t until      = deadline < bus_change ? deadline : bus_change;

    cpu_run_until(cpu, until);

    rigel_step_result_t r = rigel_step_until(rigel, cpu_get_time(cpu));

    if (r.events & RIGEL_EVENT_IRQ_CHANGED)
        cpu_set_ipl(cpu, rigel_get_ipl(rigel));

    if (r.events & RIGEL_EVENT_FRAME_READY) {
        rigel_frame_t f;
        rigel_get_frame(rigel, &f);
        host_present_frame(&f);   /* f.pixels valid until next rigel_step */
    }

    rigel_bus_state_t bus = rigel_get_bus_state(rigel);
    if (bus.cpu_would_stall)
        cpu_wait_until(cpu, bus.next_change);
}
```

`RIGEL_EVENT_FRAME_READY` means the beam wrapped to the next frame and Denise
has swapped the completed internal front buffer. If a host framebuffer target is
configured, all visible scanlines for that completed frame have also been
written when this event is reported.

`RIGEL_EVENT_VBLANK` means the hardware vertical blank signal transitioned from
false to true. With the current PAL/NTSC beam model this happens in the vertical
blank zone at the start of the frame, so `FRAME_READY` and `VBLANK` can be
reported by the same `rigel_step`.

For a host-owned RGB565 framebuffer, configure `rigel_config_t.framebuffer`.
Denise will write completed visible scanlines directly into that buffer using
the host pitch; `rigel_get_frame()` remains available for the internal buffer.

Simple hosts can ignore `bus_change`, `cpu_would_stall`, and the per-scanline API —
the interface is designed so that fine integration is optional, not mandatory.

## Two host profiles

**Simple host (SDL, headless):**
- Uses `rigel_step_until` with a fixed deadline (1 frame)
- Checks `RIGEL_EVENT_IRQ_CHANGED` and `RIGEL_EVENT_FRAME_READY`
- Ignores bus state and individual scanlines

**Advanced host (PiStorm, Emu68):**
- Uses `rigel_get_next_deadline` + `rigel_get_next_bus_change` for fine-grained stepping
- Respects `cpu_would_stall` for Chip RAM contention with BLTPRI
- Queries `rigel_get_bus_state` for slot-by-slot bus arbitration

## Memory-mapped I/O

The host decodes the global memory map and forwards only what belongs to Rigel.

```
CPU writes 0x00DFF096
  → host identifies custom space (0xDFF000–0xDFF1FF)
  → host calls rigel_custom_write16(ctx, 0x096, value)
```

Rigel does not know global addresses. It receives only offsets within the
custom register space (0x000–0x1FE).

## Chip RAM

The host supplies read and write callbacks for Chip RAM via `rigel_config_t`.
Rigel uses these callbacks whenever internal domains need memory access:
blitter DMA, disk DMA, audio DMA, and bitplane fetch.

```c
rigel_config_t config = {
    .clock_hz      = 7093790,
    .chip_ram_size = 512 * 1024,
    .chip_ram = {
        .opaque  = my_ram,
        .read16  = my_read16,   /* used by audio DMA, bitplane fetch */
        .write16 = my_write16,  /* used by blitter DMA, disk DMA */
    },
};
```

## Bus observation (fine integration)

For hosts that need real bus contention:

```c
rigel_bus_state_t bus = rigel_get_bus_state(rigel);
/* bus.owner            — who owns the bus this cycle */
/* bus.cpu_would_stall  — advisory: stall condition active (BLTPRI + blitter busy) */
/* bus.cpu_can_access_chip_ram */
/* bus.next_change      — when the state changes */
```

`cpu_would_stall` is advisory — Rigel does not own the CPU. The field signals
that the classic stall condition is active; the host decides how to apply it
to its CPU core.

## IRQ delivery

Rigel exposes `INTREQ`, `INTENA`, and IPL. Delivery to the processor belongs
to the host:

```c
if (r.events & RIGEL_EVENT_IRQ_CHANGED)
    cpu_set_ipl(cpu, rigel_get_ipl(rigel));
```

## Audio output

Rigel mixes Paula's four channels internally. The host reads the current stereo
sample at whatever rate its audio callback requires:

```c
rigel_audio_sample_t s = rigel_get_audio_sample(ctx);
/* s.left / s.right — int16_t, Paula four channels mixed */
```

**Sample-driven (recommended for most hosts):**
```c
/* SDL audio callback — fill num_samples frames at sample_rate Hz */
for (int i = 0; i < num_samples; i++) {
    rigel_cycle_t target = base_cycle + (uint64_t)i * clock_hz / sample_rate;
    rigel_step_until(ctx, target);
    rigel_audio_sample_t s = rigel_get_audio_sample(ctx);
    buf[i * 2]     = s.left;
    buf[i * 2 + 1] = s.right;
}
```

**Frame-driven (simpler, lower quality):**
Call `rigel_get_audio_sample` once per `RIGEL_EVENT_HBLANK` (~15.6 kHz for PAL)
and resample in the host audio backend.

**Event-driven:**
`RIGEL_EVENT_AUDIO_READY` fires when at least one Paula audio channel period
elapses. Read the current mixed sample with `rigel_get_audio_sample(ctx)`.

## Frame pacing

See `timing_model.md`. In short: Rigel exposes `clock_hz`, `line_cycles`, and
`frame_cycles`; the host measures real time and applies its own correction
policy (sleep/skip).

## Test harness (Musashi)

`harness/` contains a reference integration with Musashi (68000) for empirically
verifying timing, bus contention, and IRQ delivery.

```sh
cmake -S . -B build-harness -DRIGEL_BUILD_HARNESS=ON -DRIGEL_BUILD_TESTS=OFF
cmake --build build-harness
```
