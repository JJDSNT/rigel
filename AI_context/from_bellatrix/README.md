# Notes carried over from Bellatrix

Rigel was developed against Bellatrix as its host. When Bellatrix was rewritten,
this material stayed behind on its `legacy` branch, so it is copied here
verbatim — it is the record of what was learned driving Rigel from a real host.

**These are historical notes, not current specification.** They describe Rigel
as it was at bellatrix `external/rigel` commit `000719d` ("feat(timing): wire
cycle-exact bus arbitration"), which is four commits behind this tree. Anything
they claim about the API should be checked against `include/rigel/` before being
acted on. Where they disagree with `docs/`, `docs/` wins.

## Integration

| File | What it holds |
| --- | --- |
| `rigel_integration_notes.md` | How the host/chipset boundary was meant to work: temporal API over MMIO, bus observation, where planar→chunky belongs, why `libamivideo` is a host concern. |
| `rigel_suggestions_for_rigel_team.md` | API gaps found from the host side — what a host wants that Rigel did not yet expose. |

## Gap analysis

| File | What it holds |
| --- | --- |
| `legacy_vs_rigel_gap.md` | Bellatrix's own chipset vs Rigel, feature by feature. |
| `rigel_gap_analysis.md` | What was missing to boot and run real software. |

## Investigations

| File | What it holds |
| --- | --- |
| `rigel_ks20_video_investigation.md` | Why Kickstart 2.0 did not display correctly. |
| `rigel_graphics_dma_scroll_investigation.md` | Bitplane DMA and scroll behaviour under real workloads. |
| `rigel_aros_adf_investigation.md` | Booting AROS from ADF; the longest of these, mostly floppy and disk DMA. |
| `rigel_performance_research.md` | Where the time went, and which measurements held up. |

## What was left behind

Bellatrix's own harness (`tools/harness/`) and its Rigel backend
(`src/machine/machine_rigel*.c`) are not copied here: both depend on the
Bellatrix machine layer — Zorro autoconfig, RTG, lide.device, ODFS, expansions,
the PAL host abstraction. Rigel's `harness/` covers the same ground for a plain
Amiga machine without any of it. The one piece taken from them directly is the
CPU/CCK clock integrator, which is why `harness.c` carries the odd-cycle
remainder across timeslices.
