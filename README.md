# Rigel

Rigel is a C library for classic Amiga-side chipset and peripheral behavior.

The project is built as a reusable hardware-facing core: the host does not talk to high-level chip objects directly. Instead, it drives the library through interfaces that resemble the machine-visible hardware surface, mainly custom-register MMIO, stepping, IRQ state, Chip RAM callbacks, and a few convenience APIs for peripherals such as floppy, input, and RTC.

Rigel is intended to be:
- deterministic on the classic chipset path
- single-thread by default
- internally organized by execution domains
- portable across hosts and runtimes

Rigel is not intended to own the whole machine. The host remains responsible for global memory-map decode, CPU integration, ROM/Fast RAM ownership, platform devices, and frontend/backend concerns.

## Current Direction

The internal architecture is split into a few clear layers:

- `src/domains/`: hardware execution domains such as `beam`, `dma`, `copper`, `blitter`, `interrupt`, `disk`, `serial`, `audio`, and `input`
- `src/chipset/`: composition, MMIO routing, ownership, and internal wiring
- `src/runtime/`: execution policy for the core
- `src/bus/`: access surfaces and bus-facing helpers
- `src/rtc/`: RTC support kept outside custom MMIO

The project is concurrency-aware internally, but it is not multicore-first. For the classic chipset path, correctness, determinism, and clear ownership matter more than early parallelism.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Public API

The main entry point is `RigelContext`.

Core lifecycle:
- `rigel_create`
- `rigel_destroy`
- `rigel_reset`
- `rigel_step`

Custom register MMIO:
- `rigel_custom_read16`
- `rigel_custom_write16`

IRQ state:
- `rigel_get_intreq`
- `rigel_get_intena`
- `rigel_get_ipl`

Convenience peripherals:
- `rigel_floppy_*`
- `rigel_input_*`
- `rigel_rtc_*`

## Host Integration Model

The host decodes the global machine memory map and forwards only the parts that belong to Rigel.

Example flow:

1. The CPU writes `0x00DFF096`.
2. The host decodes that address as custom-chip space.
3. The host forwards the write as `rigel_custom_write16(ctx, 0x096, value)`.

Rigel only understands its own internal chipset/peripheral surfaces. It does not own the machine-wide memory map.

## Minimal Example

```c
#include <rigel/rigel.h>

static rigel_u16 chip_ram_read16(void *opaque, rigel_u32 addr)
{
    rigel_u16 *ram = opaque;
    return ram[(addr >> 1)];
}

static void chip_ram_write16(void *opaque, rigel_u32 addr, rigel_u16 value)
{
    rigel_u16 *ram = opaque;
    ram[(addr >> 1)] = value;
}

int main(void)
{
    rigel_u16 chip_ram_words[256 * 1024] = {0};
    rigel_config_t config = {
        .clock_hz = 7093790,
        .chip_ram_size = sizeof(chip_ram_words),
        .enable_trace = false,
        .chip_ram = {
            .opaque = chip_ram_words,
            .read16 = chip_ram_read16,
            .write16 = chip_ram_write16,
        },
    };

    RigelContext *ctx = rigel_create(&config);
    if (!ctx) {
        return 1;
    }

    rigel_custom_write16(ctx, 0x096, 0x8200); /* DMACON */
    rigel_custom_write16(ctx, 0x09a, 0x8020); /* INTENA */
    rigel_custom_write16(ctx, 0x180, 0x0f00); /* COLOR00 */

    rigel_step(ctx, 64);

    rigel_destroy(ctx);
    return 0;
}
```

## Floppy, Input, and RTC

Rigel already exposes a small host-facing surface for a few peripherals that are convenient to manage from the embedding application:

- floppy media insertion/ejection and drive status via `rigel_floppy_*`
- joystick/pot input injection via `rigel_input_*`
- RTC model, time, and register access via `rigel_rtc_*`

These APIs are separate from custom-register MMIO on purpose. They represent host integration points, not the custom-chip register family.

## Status

Rigel is under active construction. The core structure is in place, the test suite is growing with each migrated area, and the main near-term work is improving fidelity in the domains that already exist rather than expanding the public API aggressively.
