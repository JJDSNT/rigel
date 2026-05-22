# Rigel

Rigel is a C library for classic Amiga chipset and peripheral emulation.

The library provides a deterministic, single-threaded hardware-facing core. The host
owns the CPU, memory map, ROM, and presentation layer; Rigel owns the chipset behavior.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/test_blitter       # run a single test
```

Optional: Musashi integration harness for timing verification:

```sh
cmake -S . -B build-harness -DRIGEL_BUILD_HARNESS=ON -DRIGEL_BUILD_TESTS=OFF
cmake --build build-harness
```

## Public API

All headers are included via `<rigel/rigel.h>`.

**Lifecycle:**
```c
RigelContext *rigel_create(const rigel_config_t *config);
void          rigel_destroy(RigelContext *ctx);
void          rigel_reset(RigelContext *ctx);
```

**Temporal API** — scheduling and synchronization:
```c
rigel_cycle_t       rigel_get_time(const RigelContext *ctx);
rigel_cycle_t       rigel_get_next_deadline(const RigelContext *ctx);
rigel_step_result_t rigel_step(RigelContext *ctx, rigel_cycle_t cycles);
rigel_step_result_t rigel_step_until(RigelContext *ctx, rigel_cycle_t target_time);
```

`rigel_step_result_t` carries the current time and a bitmask of what changed
(`RIGEL_EVENT_IRQ_CHANGED`, `RIGEL_EVENT_FRAME_READY`, `RIGEL_EVENT_BLIT_DONE`, etc.).

**Bus observation** — contention and wait states:
```c
rigel_bus_state_t rigel_get_bus_state(const RigelContext *ctx);
rigel_cycle_t     rigel_get_next_bus_change(const RigelContext *ctx);
bool              rigel_cpu_can_access_chip_ram(const RigelContext *ctx);
rigel_cycle_t     rigel_get_cpu_resume_time(const RigelContext *ctx);
```

**Custom register MMIO:**
```c
rigel_u16 rigel_custom_read16(const RigelContext *ctx, rigel_u32 offset);
void      rigel_custom_write16(RigelContext *ctx, rigel_u32 offset, rigel_u16 value);
```

**IRQ state:**
```c
rigel_u16 rigel_get_intreq(const RigelContext *ctx);
rigel_u16 rigel_get_intena(const RigelContext *ctx);
rigel_u8  rigel_get_ipl(const RigelContext *ctx);
```

**Peripherals:** `rigel_floppy_*`, `rigel_input_*`, `rigel_rtc_*`

## Host loop

```c
while (running) {
    rigel_cycle_t until = rigel_get_next_deadline(rigel);

    cpu_run_until(cpu, until);

    rigel_step_result_t r = rigel_step_until(rigel, cpu_get_time(cpu));

    if (r.events & RIGEL_EVENT_IRQ_CHANGED)
        cpu_set_ipl(cpu, rigel_get_ipl(rigel));

    if (r.events & RIGEL_EVENT_FRAME_READY)
        host_present_frame(rigel_get_frame(rigel));
}
```

For fine-grained bus integration (PiStorm/Emu68 style), also consult
`rigel_get_bus_state()` and `rigel_get_next_bus_change()`.

## Minimal example

```c
#include <rigel/rigel.h>

static rigel_u16 chip_ram_read16(void *opaque, rigel_u32 addr)
{
    rigel_u16 *ram = opaque;
    return ram[addr >> 1];
}

static void chip_ram_write16(void *opaque, rigel_u32 addr, rigel_u16 value)
{
    rigel_u16 *ram = opaque;
    ram[addr >> 1] = value;
}

int main(void)
{
    rigel_u16 chip_ram[256 * 1024] = {0};
    rigel_config_t config = {
        .clock_hz      = 7093790,
        .chip_ram_size = sizeof(chip_ram),
        .chip_ram      = { .opaque = chip_ram,
                           .read16 = chip_ram_read16,
                           .write16 = chip_ram_write16 },
    };

    RigelContext *ctx = rigel_create(&config);

    rigel_custom_write16(ctx, 0x096, 0x8200); /* DMACON */
    rigel_custom_write16(ctx, 0x09a, 0x8020); /* INTENA */

    rigel_step_result_t r = rigel_step(ctx, 227); /* one scanline */

    if (r.events & RIGEL_EVENT_IRQ_CHANGED)
        handle_ipl(rigel_get_ipl(ctx));

    rigel_destroy(ctx);
    return 0;
}
```

## Architecture

See `docs/` for detailed documentation:

- `architecture.md` — layers, domains, chipset composition
- `integration.md` — host loop, memory-map forwarding, bus observation
- `timing_model.md` — DMA slot sequence, Temporal API, frame pacing
- `video_output.md` — video pipeline, frame struct, pixel formats, dirty tracking
- `irq_model.md` — interrupt sources, INTREQ/INTENA, host delivery
- `memory_map.md` — custom register offsets

## Status

Under active construction. Core structure and domain split are in place.
Near-term focus: beam model completion (line/frame wrapping), slot scheduler,
and video output pipeline via Denise.
