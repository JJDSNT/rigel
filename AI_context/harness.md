# Harness

The Musashi + Rigel harness: a real 68k machine built around the library so
Rigel can be driven the way a host actually drives it. `harness/README.md` is
the user-facing reference; this file is the design record — what was learned
building it, and what is still wrong.

Rebuilt from scratch in Rigel after Bellatrix was rewritten. Bellatrix's own
harness stayed behind because it depends on that project's machine layer
(Zorro registry, RTG, PAL abstraction, expansions). See
[`from_bellatrix/`](from_bellatrix/) for the notes carried over.

## Three invariants

These are the plumbing mistakes that cost the most time. All three were
harness bugs presenting as chipset bugs.

**Two clocks.** Rigel counts colour clocks (CCK, ~3.55 MHz); Musashi counts CPU
cycles (~7.09 MHz). `rigel_get_clock_hz()` reports the CPU-side rate, so the
CCK rate is half of it. The harness halves and carries the odd cycle across
timeslices. The original loop treated them 1:1.

**Flush before every chipset access.** A `VPOS`/`VHPOS`/`INTREQ` read has to see
where the beam is now, not where it was at the top of the timeslice. Every
custom and CIA access advances Rigel by the cycles run so far first.

**IPL is a level, not an edge.** Publishing it only on
`RIGEL_EVENT_IRQ_CHANGED` loses the transition an interrupt handler causes when
it writes `INTREQ` — no step happens in between, so the next VERTB finds
Musashi still holding the old level and never re-triggers. The machine then
wedges waiting for an interrupt that cannot arrive. Kickstart 1.3 boots only
because this is mirrored after every advance *and* every write that can move
it.

## What runs

| Target | State |
| --- | --- |
| Kickstart 1.3 | insert-disk screen, and the Workbench 1.3 desktop from `wb13.adf` |
| Battle Squadron | cracktro → title → menu → gameplay, with sound |
| DiagROM | full serial diagnostic, Chip RAM test passes |
| AROS (1 MB) | boots to its startup screen, with Fast RAM — see below |
| Zorro II autoconfig | Fast RAM at `0x200000` and LIDE at `0xEA0000`, enumerated in chain order |
| LIDE / ATAPI | see below — the board loads and talks, but breaks the boot |

## What does not

**AROS needs Fast RAM.** It boots to its startup screen — a hires Intuition
window with the version banner — but only with Fast RAM present. Without it,
the console handler dies repeatedly with `PC: 0x00000008`, a call through a
null pointer.

| Chip | Fast | Result |
| --- | --- | --- |
| 512 KB | none | stops at InitCode phase `0x01`, never reaches DOS |
| 1 MB | none | phase `0x04`, 23 Software Failures in task CON |
| 2 MB | none | phase `0x04`, 23 Software Failures in task CON |
| 1 MB | 8 MB | **boots clean** |
| 2 MB | 8 MB | **boots clean** |

Chip RAM size is not the variable; the presence of Fast RAM is. Whether that is
a Rigel defect or simply AROS needing more memory than a stock Amiga has is not
settled — the failing case is worth a look because running entirely from Chip
RAM is the configuration where DMA contention is heaviest.

`tools/tests/aros_boot/run.sh` locks the working configuration in as a
regression test. It uses only free media, so it can run anywhere.

Earlier notes here blamed a jump into zeroed memory at `0x387A4`. That was an
artefact of running AROS with no boot media at all, so dosboot waited forever;
the address was noise.

**Attaching the LIDE board breaks an otherwise working boot.** This is the
sharpest open defect, and it covers both HDF and ISO.

With the same budget and memory, Kickstart 2.0 boots Workbench 2.0 from
`wb20.adf` and reaches a 672x256 screen. Attach a CD or an HDF and the machine
ends at 256x256 having never got there:

| Configuration | Result |
| --- | --- |
| KS20 alone | 592x200, insert-disk screen |
| KS20 + `wb20.adf` | **672x256, Workbench boots** |
| KS20 + `wb20.adf` + CU CD | 256x256 |
| KS20 + `wb20.hdf` | 256x256, with or without Fast RAM |

The machine is not hung — it is busier with the board attached (183k register
writes vs 90k) and the PC keeps moving. It simply never gets where it was
going. Fast RAM is not the variable, so the autoconfig chain is not the cause.

Everything up to that point does work, which is what makes the defect narrow:

- autoconfig assigns the board a base
- expansion.library reads the DiagArea at `+0004`
- the boot loader pulls lide.device out of the ROM
- the ODFS bank is read essentially end to end (31156 reads of a 31152-byte
  binary), starting at `+1fff8` where the loader looks for the signature
- ATAPI runs a correct exchange: TEST UNIT READY → UNIT_ATTENTION, REQUEST
  SENSE, TEST UNIT READY → OK, READ CAPACITY returning the right last-LBA
  (`0x0004F1A6`) and block size

Then the driver polls TEST UNIT READY forever and never issues READ TOC or
READ(10). No interrupt is wired from the board, which is the first thing to
check: a driver waiting on command completion would look exactly like this.

The board was reported as working earlier on the strength of log lines showing
the driver loading. That was never a boot — no screenshot was ever taken. It is
recorded here so the same mistake is not repeated: an emulator milestone is a
picture or an assertion, not a trace line.

The CU CD is a CDTV disc (`CD001`, System ID `CDTV`), so it is already marked
bootable and the `HARNESS_CD_BOOTABLE` PVD patch does not apply to it. Kickstart
2.0 booting this ISO is a known-good result from Bellatrix.

**Vertical banding in Battle Squadron gameplay.** The title and menu screens
are pixel-correct; the scrolling playfield shows vertical stripes that are not
in the game. Reproduces in about a minute:

```sh
./build-harness/rigel-harness media/roms/KS13.rom --adf media/disks/battle.adf \
    --headless --frames 3600 --lmb 800:120,2700:30 --screenshot game.ppm
```

This is the first rendering defect with a short, deterministic repro, and it
lands squarely on the material in
[`from_bellatrix/rigel_graphics_dma_scroll_investigation.md`](from_bellatrix/rigel_graphics_dma_scroll_investigation.md).

## Open suspicions

Not diagnosed, recorded so they are not rediscovered from zero.

**Audio clips at full scale.** Every capture peaks at exactly 32768 — the
absolute value of the int16 minimum. A loud game hitting full scale is
plausible; every capture doing it suggests the four-voice mix has no headroom.

**Battle Squadron's cracktro is silent** (`rms 0.0`) while the game itself is
not (`rms ~9000`). That particular crack may genuinely have no music, but it is
worth confirming against another machine before assuming so.

## Things that made debugging hard

Kept because each one wasted real time and would again.

- **A frozen PC does not mean a wedge.** `dbra Dn,*` is an ordinary delay loop
  and looks identical from outside. The halt detector used to end runs at
  Battle Squadron's title screen. It now reports and continues;
  `--stop-on-halt` restores the old behaviour for a caller that wants it.
- **The overlay applies to debug reads too.** A `--dump` of low memory returns
  ROM while OVL is asserted — correct, and confusing. `--dump` now says so.
- **A trace compiled out is worse than no trace.** The ATAPI and ATA sources
  carried over from Bellatrix gate their tracing on `BELLATRIX_HARNESS`, a
  define that does not exist here, so the trace silently returned nothing and
  sent the ISO investigation down a false path. Now `RIGEL_HARNESS_HAS_ENV`.
- **Software waits for input that a headless run cannot give.** Battle
  Squadron's loader wants a button before it will do anything. `--key` and
  `--lmb` exist for that.

## Instrumentation

The harness is read by people and by agents, so output is line-oriented with a
fixed `[TAG ]` prefix — `[SERIAL]`, `[REG ]`, `[DMA ]`, `[IRQ ]`, `[VID ]`,
`[STAT]`, `[CPU ]`, `[DIS ]`, `[MEM ]`, `[BRK ]`, `[SHOT]`, `[INPUT]`,
`[AUDIO]`. Exit codes distinguish budget reached (0), setup failure (1), bad
arguments (2), halt (3) and breakpoint (4).

The serial log matters most: AROS and DiagROM write their whole boot sequence
there, and it is what tells you where a machine that runs but draws nothing
actually got to. CPU tracing and breakpoints depend on
`patches/musashi/0001`, which turns the instruction hook on.

Audio has no speaker in CI, so `--audio-out FILE.wav` reports peak and RMS —
that is how a test asserts a game is making noise.

## Toolchain

`scripts/m68k` runs an m68k tool against the repository, preferring the
`amigadev/crosstools:m68k-amigaos` Docker image and falling back to
`m68k-linux-gnu-*`. `tools/tests/chipset/` is the worked example of the whole
loop: assemble, locate the program's final spin loop in the disassembly, run to
a breakpoint, verify Chip RAM. `tools/tests/musashi_fpu/` regression-tests the
FPU opmodes that `patches/musashi/0005` adds — 21 of them.

The disassembler is Musashi's own rather than Capstone: it needs no dependency
and it decodes exactly what the emulated CPU will execute, including the opcodes
our patches add. Agreeing with the emulator is the point.
