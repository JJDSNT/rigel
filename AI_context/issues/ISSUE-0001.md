---
id: ISSUE-0001
title: "Support loading aros.elf as well as aros.rom, selectable in the TUI"
status: open
priority: high
type: enhancement
owner: unassigned
created_at: 2026-08-15
updated_at: 2026-08-15
tags:
  - harness
  - hunk
  - emu68
  - exec
  - framebuffer
  - elf
  - aros
  - tui
related_files:
  - tools/launcher/tui.go
  - harness/hunk.c
  - harness/hunk.h
  - harness/harness.c
  - harness/main.c
  - patches/musashi/0007-emu68-timer-movec.patch
  - media/roms/Buddha.rom
  - media/roms/sysinfo.rom
---

## Goal

The TUI should offer AROS either way: `aros.rom` through the ROM path, which
boots today, or `aros-emu68-m68k.elf` through an ELF path, which does not
exist yet. Same operating system, two ways in — the ROM path is a plain Amiga
with a Kickstart, the ELF path puts the harness in the role Emu68 plays.

Everything below is what stands between here and that.

## Where it started

`--exec` LoadSegs an AmigaOS hunk executable and runs it in place of a ROM.
Emu68's two bare-metal examples — `Buddha.rom` (a Buddhabrot renderer) and
`sysinfo.rom` (a Dhrystone/BUSTEST benchmark), both misnamed, both actually
hunk binaries — load, relocate and execute without a single exception.

Neither draws anything. The framebuffer handed to them stays untouched.

```sh
./build-harness/rigel-harness --exec media/roms/Buddha.rom --cpu 68040 \
    --headless --frames 20000 --chip 2048 --exec-fb 640x256 \
    --exec-fb-out bud.ppm
# ... 2832960001 CPU cycles, 0 non-black pixels
```

## What is known to work

- The hunk file parses: 2 hunks for each image, sizes and relocations as
  expected. Buddha's second hunk is a 2.6 MB BSS, which is why the loader
  places images in Fast RAM.
- Execution starts at the right address. Traced for sysinfo:
  `00200008 move.l A0,$2020a8` / `move.l D0,$2020ac` / `pea $3e8` /
  `jsr $200dec` — it is saving the framebuffer pointer and pitch it was
  handed, so the register convention is being received.
- Buddha runs 2.8 billion cycles with no F-line, no illegal instruction and no
  bus error, sitting in a loop that packs RGB565 pixels (`lsl.w #5`, `or.w`,
  `ror.w #8`).
- The Emu68 timer registers are answered (`patches/musashi/0007`), so the
  earlier crash into a null vector at `movec` is gone.

## What is not known

Why nothing reaches the framebuffer. Candidates, none tested:

1. **The entry convention may be incomplete.** Emu68 calls `_c_start` with
   four register arguments, which is what `startup.c` in the example declares.
   Whether Emu68 also sets up something else first — a stack frame shape, an
   `A6`, a supervisor/user mode — has not been checked against Emu68's own
   launch path.
2. **The programs may be waiting on something.** Buddha's loop was captured
   mid-computation; whether it ever reaches its write-out phase is unverified.
   A `--watch` on the framebuffer would answer this directly.
3. **The framebuffer address may not be where they write.** The pointer is
   saved to a variable in hunk 0 immediately, but nothing confirms the render
   path reads it back rather than using a compiled-in address.

## Why it matters

Beyond the examples themselves, this is the path towards loading
`aros_68k.elf`, which is the actual goal. That will need an ELF loader
alongside the hunk one, but everything else — `--exec`, the framebuffer, Fast
RAM forced configured without autoconfig, the synthesised reset vector — is
shared.

As a workload these are also the only thing in the tree that exercises CPU and
memory with no chipset involvement at all, which makes them a clean CPU
benchmark once they work.

## Notes

The `.rom` extension on both files is an accident of where they were filed;
they are `HUNK_HEADER` executables, not ROM images and not ELF.


## Update — the AROS ELF is a different problem

Investigated `out/aros/aros-emu68-m68k.elf` from the current Bellatrix, which
is where this path is heading. It is not loadable the way the examples are.

| | |
| --- | --- |
| Class / byte order | ELF32, big endian |
| Machine | MC68000 |
| **Type** | **REL — relocatable, not an executable** |
| Program headers | **none** |
| `e_entry` | **0** |
| Sections | 11 |
| `.text` | 0xD31F0 (~865 KB) |
| `.data` / `.bss` | 4 bytes / 0x138 |
| Relocations | **12116 × R_68K_32, 46 × R_68K_PC32** |

With no program headers there are no LOAD segments to copy, and with
`e_entry = 0` the entry is not in the header. This is an object file: loading
it means doing what a linker does — walk the section headers, place every
`SHF_ALLOC` section, and resolve twelve thousand relocations against the
symbol table.

Emu68 does exactly that, in `src/ElfLoader.c` (542 lines):

- `checkHeader` accepts `ET_REL` as well as `ET_EXEC`, big-endian, m68k or PPC
- `GetElfSize` totals the allocatable sections, split into RO and RW so the
  two can be placed in separate regions
- `loadHunk` places each section and assigns it an `sh_addr`
- `relocate` walks `SHT_RELA` and applies `R_68K_32`, `R_68K_PC32` and the
  smaller widths, resolving `SHN_UNDEF` and `SHN_COMMON` symbols

For `ET_REL` it keeps the load base as the result, so the entry is simply the
base — `.text` is the first allocatable section and starts there.
`src/aarch64/start.c` calls it with `top_of_ram` as the base, having first
sniffed whether the initrd is HUNK or ELF.

### What this means here

`harness/hunk.c` does not cover this and cannot be extended to. A separate ELF
loader is needed, of comparable size to Emu68's. What is already built is
reusable: `--exec` and its file dispatch, Fast RAM forced configured without
autoconfig, the synthesised reset vector, the framebuffer plumbing, and the
memory accessors the loader writes through.

Sniffing the two formats apart is trivial — `0x000003F3` versus `\x7fELF` in
the first four bytes.

### Order of work

The hunk examples are the smaller problem and the better first target: they
already load and run, so whatever stops them reaching the framebuffer is a
bounded question. The ELF loader is a known quantity — a day's transcription
of `ElfLoader.c` against our memory accessors — but it lands on top of an
`--exec` path that is not yet proven to produce output at all.


## Update 2 — what the ELF path actually requires

Read the AROS side, in `aros/arch/m68k-emu68/boot/boot.c` of the current
Bellatrix. The contract is larger than the examples', and one part of it is
not optional.

`.text` begins with:

```
move.l D2,-(A7)      ; height
move.l D1,-(A7)      ; width
move.l D0,-(A7)      ; pitch
move.l A0,-(A7)      ; framebuffer
move.l A6,-(A7)      ; fdt
jsr    <relocated>
lea    20(A7),A7
stop   #$2700
```

which lands on:

```c
void emu68_bootstrap(const void *fdt, void *framebuffer, uint32_t pitch,
                     uint32_t width, uint32_t height)
```

So the entry registers are **A6 = flattened device tree, A0 = framebuffer,
D0 = pitch, D1 = width, D2 = height**. The framebuffer four are what the
Emu68 examples already take; A6 is new and it is the hard part.

**The device tree is mandatory.** `emu68_bootstrap` calls `parse_fdt`, which
walks the tree for a `/memory` node and fills `memory_base`/`memory_size`,
setting `EMU68_BOOT_MEMORY_VALID`. `start_aros` then opens with:

```c
if (!(ctx->flags & EMU68_BOOT_MEMORY_VALID))
    return;
```

Pass no FDT — or one without a memory node — and AROS returns without booting.
`parse_fdt` bails out cleanly on a bad header rather than crashing, so the
failure is silent: exactly the shape of "loads, runs, produces nothing" that
this issue opened with.

`/chosen` bootargs are read too, but nothing depends on them.

### The work

1. **ELF32 BE m68k `ET_REL` loader.** Sections rather than segments; place
   every `SHF_ALLOC` one, split RO from RW; resolve 12162 relocations
   (`R_68K_32`, `R_68K_PC32`) against the symbol table, handling `SHN_UNDEF`
   and `SHN_COMMON`. Entry is the load base, since `.text` is placed first.
   Emu68's `src/ElfLoader.c` is 542 lines and is the reference.
2. **Synthesise a minimal FDT** — a header, one `/memory` node with the
   harness's RAM range, optionally `/chosen` for bootargs. A few hundred bytes
   of well-documented binary format, generated in C.
3. **Pass A6** alongside the four registers `--exec` already sets.
4. **Dispatch on the file** — `0x000003F3` is hunk, `\x7fELF` is ELF. One
   `--exec` handles both.
5. **TUI**: let a `.elf` be picked in the Kickstart pane and routed to
   `--exec` instead of being passed as a ROM.

### On the examples

They remain the cheaper thing to get working first — 12 KB rather than 865 KB,
and no device tree involved. But they are no longer the point, and if they
turn out to need something obscure they should not hold the ELF path up: the
two share the loader plumbing, not the environment.
