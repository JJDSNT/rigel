---
id: ISSUE-0005
title: "DF1 media prevents DF0 boot and external DD identification loses its last bit"
status: resolved
priority: high
type: bug
owner: unassigned
created_at: 2026-08-29
updated_at: 2026-08-29
tags:
  - harness
  - floppy
  - cia
  - kickstart
  - multi-drive
related_files:
  - src/core/rigel.c
  - src/core/rigel_cia_api.c
  - src/floppy/floppy_drive.c
  - src/floppy/floppy_drive.h
  - tests/test_floppy.c
---

## Symptom

Kickstart 1.3 could boot NewTek Demo Reel 3 disk 1 from DF0 only while DF1 was
empty. Inserting any ADF in DF1 left the machine in the insert-disk animation;
the ROM wrote `DSKLEN=0x4000` but never programmed a transfer. After the first
part was fixed, Workbench booted but requested volume `demoreeldata` even
though disk 2 was present in DF1.

## Causes

Three floppy/CIA errors combined:

1. `/DSKCHG` was composed open-drain from every connected drive. A pending
   change in unselected DF1 therefore appeared while Kickstart was probing
   DF0. The shared status bus must reflect the one selected drive.
2. The drive-ID value was driven onto active-low `/DSKRDY` with inverted
   polarity. A `DRT_AMIGA` zero bit must pull the sampled line low.
3. `id_count` advanced on every CIA update while a drive was deselected, not
   only on a selected-to-deselected edge. The motor/select preamble also
   consumed one edge before the 32 sampled bits. The result was an exhausted
   or 31-bit identification sequence, so a normal external DD drive was not
   enumerated.

The drive now remembers its selection state, advances the ID counter only on
the deselection edge, arms it at `-1` for the preamble, and exposes ID mode only
for samples 0 through 31.

## Regression coverage

`test_floppy` now checks that:

- a pending change in DF1 does not assert `/DSKCHG` while DF0 is selected;
- repeated DF0 control writes do not consume DF1's ID stream;
- the exact motor/select preamble followed by 32 reads returns
  `DRT_AMIGA` (`0x00000000`) for an external DD drive.

## End-to-end validation

The pinned Musashi harness ran Kickstart 1.3, 512 KiB Chip plus 512 KiB Slow,
PAL OCS and `--cycle-exact` with Demo Reel 3 disk 1 in DF0 and disk 2 in DF1.

- DF0 boot DMA progressed through `DSKPT=0x2064`, `DSKLEN=0x9cbe`.
- Workbench mounted both `DemoReel3` and `DemoReelData` without a requester.
- A disposable disk-1 copy invoked `DemoReelData:Slish` directly because the
  headless harness only scripts button state, not pointer movement/double
  click semantics.
- At frame 3500 Rigel presented a 384x256 demo frame: the red countdown digit
  `7` over the static effect. The run reached 307,219 register writes.

The original ADFs were not modified. The disposable autostart image and
screenshots lived under Bellatrix's ignored `out/` tree.

## Validation

- `test_floppy`: passed.
- CTest: 29/30 passed. The only failure is the pre-existing
  `harness_test_blitter_timing` hardware-reference gap; it is unrelated to
  floppy selection or identification.
- Bellatrix `./scripts/setup.sh --verify`: all patch series applied.
