# Agnus

Agnus is the timing and DMA-facing core of the classic chipset inside Rigel.

## Ownership

- **Agnus** owns beam/raster timing, DMA cadence, arbitration, copper stepping,
  blitter progression, and fetch-side concerns.
- **Denise** consumes already-timed state and handles visual composition.
- **Paula** handles IRQ policy, disk-controller logic, audio finalisation, and
  serial/input behaviour.

## Implemented baseline

- Beam wraps correctly across scanlines and frames with visible-area information.
- DMA ownership is explicit through the DMA domain (`dma_state_t`, `DMACON`).
- **Slot scheduler** (`agnus_slot_scheduler_t`): the timing heartbeat. Divides each
  raster line into fixed CCK slots and dispatches the correct domain at each one:
  - Disk DMA at `AGNUS_HPOS_DISK_0` (0x07) and `AGNUS_HPOS_DISK_1` (0x09)
  - Audio DMA at `AGNUS_HPOS_AUDIO_0`–`AGNUS_HPOS_AUDIO_3` (0x0B–0x11)
  - Bitplane fetch in the DDFSTRT–DDFSTOP window
  - Copper at FREE slots when `DMAEN|COPEN`
  - Blitter at FREE (or CPU with BLTPRI) slots when the blitter is busy
- **Copper**: pointer registers, jump registers, beam-bound WAIT, MOVE with COPCON
  danger-register protection (CDANG bit enforced — copper cannot write registers
  below `0x040` unless `COPCON` bit 1 is set).
- **Bitplane fetch**: plane words fetched per slot and staged for Denise composition.
- **Blitter**: full minterm/shift pipeline with DMA.
- **ECS minimum surface**: `rigel_config_t.chipset_model` can select ECS.
  Agnus exposes ECS IDs through `VPOSR`, accepts `BEAMCON0` PAL/NTSC selection,
  and Denise exposes `DENISEID` plus `DIWHIGH`.

## Structural rules

- There is no visual `sprites/` subsystem inside Agnus. Sprite *fetch* belongs to
  Agnus DMA/timing; sprite *interpretation and composition* belong to Denise.
- `src/domains/` remains the canonical location for shared hardware state machines
  (beam, DMA, copper, blitter). `src/chipset/agnus/` stays focused on Agnus-facing
  composition, MMIO entrypoints, and state ownership.
- Agnus should not grow a parallel tree that duplicates what already lives in `domains/`.

## Slot scheduler — DMACON requirements

The slot table is rebuilt whenever DMACON changes or a new raster line starts.
Individual channel slots are only assigned when both the master enable (`DMAEN`,
bit 9) and the channel enable bit are set:

| Channel    | Required DMACON bits |
|------------|----------------------|
| Disk       | `DMAEN \| DSKEN`     |
| Audio 0–3  | `DMAEN \| AUD0EN`… `AUD3EN` |
| Sprites    | `DMAEN \| SPREN`     |
| Bitplanes  | `DMAEN \| BPLEN`     |
| Copper     | `DMAEN \| COPEN`     |
| Blitter    | `DMAEN \| BLTEN`     |

## Next steps

- ECS programmable beam timing beyond `BEAMCON0` bit 5
- ECS SuperHires/Productivity display modes
- ECS Chip RAM address policy
- `BPLCON3` display semantics
