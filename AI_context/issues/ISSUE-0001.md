---
id: ISSUE-0001
title: "Support loading aros.elf as well as aros.rom, selectable in the TUI"
status: open
priority: low
type: research
owner: unassigned
created_at: 2026-08-15
updated_at: 2026-08-16
tags:
  - harness
  - hunk
  - emu68
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

## Standing

**This is research, not a requirement.** The harness exists to move Rigel's
development along, and it already does that: it boots Kickstart, Workbench,
AROS, games and a CD. Loading `aros-emu68-m68k.elf` would be a second way in to
an operating system that already boots here through its ROM, and it buys Rigel
nothing it does not have.

Kept because the harness-side findings are real and not cheap to rediscover. Do
not let it displace the chipset work in [`../next_steps.md`](../next_steps.md).

**The other half of this question left the repository.** What AROS requires from
whatever starts it — the boot contract, how the memory range is delivered, the
interrupt and console assumptions — is a question about the AROS port, not about
Rigel. It is documented in Bellatrix, in `docs/aros_port_contract.md`, and the
work is tracked there as `AI_context/issues/ISSUE-0023`.

That matters for scheduling here: the entry conditions this issue would build
against are the subject of an open refactor. Anything written now against
today's contract is written against a moving target. See **Wait for** below.

## What would be built here

The TUI offers AROS either way: `aros.rom` through the ROM path that boots
today, or the ELF through a path that does not exist. The harness side of that
is three things.

1. **An ELF32 BE m68k `ET_REL` loader.** `harness/hunk.c` does not cover this and
   cannot be extended to — the file has no program headers and `e_entry = 0`, so
   loading it means doing what a linker does.
2. **Dispatch on the file.** `0x000003F3` is hunk, `\x7fELF` is ELF; one
   `--exec` handles both.
3. **TUI**: let a `.elf` be picked in the Kickstart pane and routed to `--exec`
   instead of being passed as a ROM.

Everything else already exists and is reusable: `--exec`, Fast RAM forced
configured without autoconfig, the synthesised reset vector, the framebuffer
plumbing, and the memory accessors a loader would write through.

### The loader does not have to be written from scratch

AROS carries the machinery in-tree, for exactly this relocation set:

- `tools/elf2hunk/elf2hunk.c` — runs on the build host, accepts **only**
  `ET_REL`, applies `R_68K_32` and `R_68K_PC32`, rejects `R_68K_PC16`
- `arch/arm-raspi/boot/elf.c` and `arch/aarch64-raspi/boot/elf.c` — the Pi
  bootstraps' loaders, both accepting `ET_REL` and walking `SHT_RELA`

What none of them does is place the image at an absolute base and report that
base as the entry point, which is what `ET_REL` requires. That is the smaller
half.

## The file

`aros-emu68-m68k.elf`, as built by Bellatrix:

| | |
| --- | --- |
| Class / byte order | ELF32, big endian |
| Machine | MC68000 |
| **Type** | **REL — relocatable, not an executable** |
| Program headers | **none** |
| `e_entry` | **0** |
| `.text` | 0xD31F0 (~865 KB) |
| `.data` / `.bss` | 4 bytes / 0x138 |
| Relocations | **12116 × `R_68K_32`, 46 × `R_68K_PC32`** |

## The open harness bug

`--exec` LoadSegs an AmigaOS hunk executable and runs it in place of a ROM.
Emu68's two bare-metal examples — `Buddha.rom` (a Buddhabrot renderer) and
`sysinfo.rom` (a Dhrystone/BUSTEST benchmark), both misnamed, both actually hunk
binaries rather than ROM images or ELF — load, relocate and execute without a
single exception.

**Neither draws anything.** The framebuffer handed to them stays untouched.

```sh
./build-harness/rigel-harness --exec media/roms/Buddha.rom --cpu 68040 \
    --headless --frames 20000 --chip 2048 --exec-fb 640x256 \
    --exec-fb-out bud.ppm
# ... 2832960001 CPU cycles, 0 non-black pixels
```

Known to work:

- The hunk file parses: 2 hunks for each image, sizes and relocations as
  expected. Buddha's second hunk is a 2.6 MB BSS, which is why the loader places
  images in Fast RAM.
- Execution starts at the right address. Traced for sysinfo:
  `00200008 move.l A0,$2020a8` / `move.l D0,$2020ac` / `pea $3e8` /
  `jsr $200dec` — it is saving the framebuffer pointer and pitch it was handed,
  so the register convention is being received.
- Buddha runs 2.8 billion cycles with no F-line, no illegal instruction and no
  bus error, sitting in a loop that packs RGB565 pixels (`lsl.w #5`, `or.w`,
  `ror.w #8`).
- The Emu68 timer registers are answered (`patches/musashi/0007`), so the earlier
  crash into a null vector at `movec` is gone.

Candidates for why nothing reaches the framebuffer, none tested:

1. **The entry convention may be incomplete.** Whether Emu68 sets up anything
   else first — a stack frame shape, an `A6`, a supervisor/user mode — has not
   been checked against Emu68's own launch path.
2. **The programs may be waiting on something.** Buddha's loop was captured
   mid-computation; whether it ever reaches its write-out phase is unverified. A
   `--watch` on the framebuffer would answer this directly.
3. **The framebuffer address may not be where they write.** The pointer is saved
   to a variable in hunk 0 immediately, but nothing confirms the render path
   reads it back rather than using a compiled-in address.

**These examples are not a stepping stone to the ELF path.** They were picked
because they looked like ELF; they are hunk. They differ from the ELF path in
the file format *and* in the entry contract. What they share with it is
`--exec`, the framebuffer plumbing and the memory accessors — all of which
already work. They are worth finishing on their own account, being 12 KB against
865 KB, and nothing about the ELF path is blocked on them.

## Wait for

Bellatrix `ISSUE-0023` to settle, or to be abandoned. It splits the AROS port
into a `m68k-native` architecture and a machine bootstrap, and the entry
conditions a Rigel-hosted AROS would have to satisfy are precisely what it is
rewriting. Building against today's conditions means building a bootstrap for a
contract that is under revision.

If it does land, the harness becomes a candidate second machine — the value of
which, from Rigel's side, is a CPU-and-memory workload with no chipset
involvement at all, and from the port's side, a deterministic place to be
observed. Its first milestone would be a serial log out of Exec, not a desktop:
with no hardware description there is no storage, so no dosboot.

Neither of those is a reason to start now.
