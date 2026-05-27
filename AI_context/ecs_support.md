# ECS support

## Current baseline

Rigel now has an explicit chipset model in `rigel_config_t`:

```c
cfg.chipset_model = RIGEL_CHIPSET_ECS;
```

Default zeroed config remains OCS-compatible.

Implemented ECS pieces:

- `VPOSR[14:8]` exposes Agnus ID bits:
  - OCS PAL: `$00`
  - OCS NTSC: `$10`
  - ECS PAL: `$20`
  - ECS NTSC: `$30`
- `DENISEID` (`$07c`) returns `$00fc` for ECS Denise and `$0000` for OCS.
- `BEAMCON0` (`$1dc`) is accepted in ECS mode. Minimal policy implemented:
  - bit 5 clear -> NTSC timing
  - bit 5 set -> PAL timing
  - other programmable beam bits are masked to zero until the timing model supports them.
- `DIWHIGH` (`$1e4`) is accepted in ECS mode and extends `DIWSTRT`/`DIWSTOP`
  with high horizontal and vertical bits.
- `BPLCON3` (`$106`) is latched/readable in ECS mode, but its display effects
  are intentionally not implemented yet.

## Still missing for full ECS

- Programmable beam timing beyond `BEAMCON0` bit 5: `HTOTAL`, `VTOTAL`,
  sync/blanking/hcenter registers, LOF/interlace interactions, and external sync bits.
- SuperHires/ECS Denise output. Current renderer supports lores/hires-era paths,
  but not SHRES pixel cadence, display width, or sprite resolution effects.
- Productivity/31 kHz modes.
- ECS Chip RAM address policy: OCS 512 KiB vs ECS 1 MiB/2 MiB visibility should
  be modelled and tested against DMA pointer masking.
- `BPLCON3` semantics: sprite resolution (`SPRES`), bank/compare bits, and any
  Denise-side effects needed by ECS software.
- Broader ECS register map behaviour for currently unimplemented registers:
  decide per-register whether to latch, ignore, or expose side effects.

## Implementation order

1. Keep chipset model as the feature gate for all ECS-only behaviour.
2. Add one ECS register at a time with tests that cover OCS ignore/mask behaviour.
3. Treat SuperHires and productivity as renderer/timing work, not as simple MMIO latches.
4. Implement Chip RAM address limits before relying on ECS memory claims.
