# Harness media

Images the harness boots. `./run.sh` scans this directory (override with
`MEDIA_DIR`) and creates `roms/` and `disks/` on first run.

```
media/
  roms/    Kickstart and other boot ROMs — 256K, 512K or 1 MB
  disks/   ADF floppy images (ISO and HDF are listed but have no backend yet)
```

## What is committed here

`.gitignore` denies this directory by default and re-includes only images that
are free to redistribute. An accidentally committed Kickstart is not something
a later commit undoes, so the allowlist is deliberately narrow and anchored —
a loose `*aros*` pattern would also match `WB13_aros.adf`, which is a Workbench
disk and is not free.

Tracked:

| Image | Licence |
| --- | --- |
| `roms/aros*.rom`, `roms/new_aros*.rom` | AROS — APL |
| `roms/diagrom*.rom` | DiagROM (John Hertell) — freely redistributable |
| `roms/lide.rom` | lide.device (LIV2) — open source |
| `disks/aros*.adf`, `disks/bootdisk-amiga-m68k.adf` | AROS — APL |
| `disks/AmigaTestKit.adf` | Amiga Test Kit (Keir Fraser) — open source |

Everything else — Kickstart 1.x/2.x/3.x, Workbench, commercial software —
stays ignored. Put your own copies here; git will not pick them up.

Adding another free image means adding a `!` line to `.gitignore` next to the
others, with the licence named.

## ROM layout

| Size | Where it lands |
| --- | --- |
| 256K | `0xF80000` window, mirrored — `0xF80000` and `0xFC0000` both reach offset 0 |
| 512K | `0xF80000` window |
| 1 MB | split: first half is the extended ROM at `0xE00000`, second half the standard ROM at `0xF80000` |

## Status

Kickstart 1.3 boots to the insert-disk screen, and to the Workbench 1.3
desktop with `disks/wb13.adf` in DF0.

AROS (1 MB) loads and executes — the F-line diagnostic shows it probing for an
FPU — but produces no picture yet. See
`AI_context/from_bellatrix/rigel_aros_adf_investigation.md`.
