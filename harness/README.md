# Harness

A real 68k machine built around Rigel: Musashi executes the CPU, the harness
owns the memory map and the ROM, and Rigel owns everything on the chipset side
of the bus. It exists so Rigel can be driven the way a host actually drives it —
booting a Kickstart, reading a floppy, putting pixels on screen — rather than
only through unit tests.

## run.sh

`./run.sh` in the repository root is the short path: it fetches and patches
Musashi, builds the harness and the launcher, opens the TUI to pick media and
machine options, then runs what you picked.

```sh
./run.sh                                    # TUI, then an SDL2 window
./run.sh --headless --frames 600 --screenshot boot.ppm
./run.sh build                              # build only
./run.sh test                               # core + harness + launcher suites
./run.sh clean

KICKSTART=kick13.rom ADF=wb13.adf ./run.sh  # skip the TUI
MEDIA_DIR=/path/to/images ./run.sh          # scan somewhere else
NO_TUI=1 ...                                # never open the TUI
```

Images live in `MEDIA_DIR` (default `./media`), either as `roms/` and `disks/`
subdirectories or flat in one directory. `./run.sh` creates the skeleton on
first run. `media/` is gitignored — Kickstart ROMs are not redistributable.

Options after the mode go straight to `rigel-harness` and override the TUI.

## Build by hand

```sh
git submodule update --init external/musashi
scripts/apply_musashi_patches.sh

cmake -S . -B build-harness -DRIGEL_BUILD_HARNESS=ON -DRIGEL_BUILD_TESTS=OFF
cmake --build build-harness
```

CMake refuses to configure if the Musashi patches are missing, because a
pristine tree silently drops the FPU fixes and the F-line diagnostic. SDL2 is
optional: without it `rigel-harness` still builds and runs headless.

## Run

```sh
# Interactive — SDL2 window, F12 grabs the mouse, Escape releases it
./build-harness/rigel-harness kick13.rom --adf wb13.adf

# Headless, for CI and chipset validation
./build-harness/rigel-harness kick13.rom --headless --frames 600 \
    --screenshot boot.ppm
```

| Option | Meaning |
| --- | --- |
| `--adf FILE` | insert into DF0 (also `--df1`, `--df2`, `--df3`) |
| `--cpu TYPE` | `68000` (default), `68010`, `68ec020`, `68020`, `68030`, `68040` |
| `--chip KB` | Chip RAM size, default 512 |
| `--slow KB` | Slow RAM at `0xC00000`, default none |
| `--pal` / `--ntsc` | video standard, default PAL |
| `--ecs` | ECS chipset instead of OCS |
| `--cycle-exact` | Rigel's honest-hybrid cost model |
| `--headless` | no window |
| `--frames N` / `--cycles N` | stop after a budget |
| `--screenshot FILE` | write the last completed frame as a binary PPM |
| `--trace` | let Rigel's structured log events reach stderr |

Every path option also reads an environment fallback (`KICKSTART`, `ADF`,
`HARNESS_CPU`) so the Go launcher can drive it.

## Watching the machine

The harness is meant to be read by people and by agents, so everything it
reports is line-oriented with a fixed `[TAG ]` prefix: `[SERIAL]`, `[REG ]`,
`[DMA ]`, `[IRQ ]`, `[VID ]`, `[STAT]`, `[CPU ]`, `[DIS ]`, `[MEM ]`, `[BRK ]`,
`[SHOT]`. Grep for the tag you want.

| Option | Meaning |
| --- | --- |
| `--serial MODE` | Paula UART: `line` (default), `raw`, `off` |
| `--serial-slow` | pace SERDAT at the programmed baud rate instead of instantly |
| `--log CATS` | `regs,dma,irq,disk,copper,blitter,video,cia,all` |
| `--status N` | one-line machine summary every N frames |
| `--trace-cpu N` | keep the last N instructions and disassemble them at the end |
| `--trace-pc LO:HI` | disassemble live while the PC is inside the range |
| `--disasm ADDR[:N]` | disassemble N instructions at ADDR when the run ends |
| `--break ADDR` | stop when the PC reaches ADDR; exit code 4 |
| `--watch ADDR:LEN` | report changed bytes in a region, once per frame |
| `--dump ADDR:LEN[:FILE]` | dump memory at the end; hex to stdout with no FILE |
| `--screenshot-every N` | write a PPM every N frames |
| `--screenshot-dir DIR` | where those go |

Addresses and lengths are hex.

The serial log is the most useful of these. AROS and DiagROM write their whole
boot sequence there, which is what tells you where a machine that runs but
draws nothing actually got to.

`--trace-cpu` and `--break` depend on `patches/musashi/0001`, which turns the
Musashi instruction hook on. Disassembly reads through a side-effect-free path,
so it never advances the clock or touches a read-sensitive register.

Note that a low-address dump reads ROM, not Chip RAM, while OVL is still
asserted — real hardware behaves the same way, and `--dump` says so when it
applies.

### Exit codes

| Code | Meaning |
| --- | --- |
| 0 | ran to the frame or cycle budget |
| 1 | setup failure (bad ROM, unreadable file) |
| 2 | bad arguments |
| 3 | the CPU stopped making progress |
| 4 | a breakpoint was hit |

## Cross toolchain

`scripts/m68k` runs an m68k tool against files in the repository, picking a
backend automatically:

```sh
scripts/m68k --check          # which backend is available
scripts/m68k --tools          # what it provides
scripts/m68k vasmm68k_mot -Fbin -quiet -o test.bin test.S
```

It prefers the `amigadev/crosstools:m68k-amigaos` Docker image (vasm, vlink,
vbcc, m68k-amigaos GCC) and falls back to `m68k-linux-gnu-*` from apt, which
can assemble and link but has no AmigaOS headers or hunk output. `$HOME` and
`/tmp` are mounted at the same paths inside the container, so paths mean the
same thing on both sides.

`tools/tests/chipset/` is a worked example of the whole loop: assemble a
program, find its final spin loop in the disassembly, run it to a breakpoint,
and verify what it wrote to Chip RAM.

```sh
tools/tests/chipset/run.sh
```

## Launcher

`tools/launcher` is a bubbletea TUI that scans a media directory and builds the
command line. `./run.sh` builds and drives it; to use it standalone:

```sh
cd tools/launcher && go build -o ../../build-harness/rigel-launcher .

./build-harness/rigel-launcher media -exec ./build-harness/rigel-harness
./build-harness/rigel-launcher media -o sel.env   # shell assignments
```

It scans ROM, ADF, ISO and HDF. ISO and HDF are listed and reported, but the
harness has no CD-ROM or hard-disk backend yet, so selecting one changes
nothing about the run.

## Memory map

| Range | Contents |
| --- | --- |
| `0x000000`–chip size | Chip RAM. Reads come from ROM while OVL is asserted; writes always land in RAM. |
| `0xA00000`–`0xBFFFFF` | CIA-A when A12 is low, on the odd byte lane; CIA-B when A13 is low, on the even lane. |
| `0xC00000`–`0xC7FFFF` | Slow RAM, when configured. |
| `0xDF0000`–`0xDFFFFF` | Custom registers, mirrored; register = `addr & 0x1FE`. |
| `0xE00000`–`0xE7FFFF` | Extended ROM — the first half of a 1 MB image (the AROS layout). |
| `0xF80000`–`0xFFFFFF` | Kickstart. A 256K image mirrors through the window, so `0xF80000` and `0xFC0000` both reach offset 0; a 1 MB image puts its second half here. |

The overlay follows CIA-A PRA bit 0. At reset DDRA has that bit as an input, so
the line floats high and the overlay is on — which is what puts the Kickstart
reset vector at address 0.

## Two clocks

Rigel counts colour clocks (CCK, ~3.55 MHz); Musashi counts CPU cycles
(~7.09 MHz). The harness halves CPU cycles into CCK and carries the odd cycle
across timeslices, so the two never drift.

Every chipset-visible access flushes the cycles run so far into Rigel *before*
the access, so a `VPOS`/`VHPOS`/`INTREQ` read reflects where the beam actually
is rather than where it was at the top of the timeslice.

IPL is mirrored into Musashi as a **level**, not an edge — after every advance
and after every CIA or custom write that can move it. Publishing it only on
`RIGEL_EVENT_IRQ_CHANGED` loses the transition an interrupt handler causes when
it writes `INTREQ`, and the machine then wedges waiting for an interrupt that
never re-triggers.

## Tests

`harness/tests/` holds timing tests that run the CPU against the chipset. They
use `harness_create()`, the minimal rig — no ROM image, synthesised reset
vector, 512K Chip RAM.

```sh
ctest --test-dir build-harness --output-on-failure
```

`harness_test_blitter_timing` currently fails: the blitter cost model estimates
`W×H` without the slot/CCK factor or the channel count, so it comes out ~96 CCKs
faster than hardware. This is a Rigel model gap, not a harness one — it fails
identically against an unpatched Musashi.
