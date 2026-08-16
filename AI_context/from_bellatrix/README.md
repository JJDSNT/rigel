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

## Still to migrate

Identified but not yet brought over.

**`docs/Rigel_integration.md` from the current Bellatrix** — 60 KB, titled
"Bellatrix / Rigel Integration Specification: Host Interface, MMIO, Timing,
Interrupts, DMA, Memory, and Lifecycle". Note that this is from Bellatrix's
`main`, not the `legacy` branch everything else here came from, so it is the
*current* contract rather than a historical record. It supersedes
`rigel_integration_notes.md` in this directory, which is the old exploratory
version. Worth reading before changing any host-facing API.

**Copperline's timing test** —
`external/copperline/timing-test/` in the legacy tree holds `timing-test.adf`
with its `test.asm` source, an FS-UAE config to compare against, and a
`cputest-runner` crate. A cycle-timing suite on a bootable ADF is directly
runnable by the harness now that it boots floppies, and it is the obvious way
to put numbers on Rigel's timing model rather than arguing about it. Copperline
is at <https://github.com/LinuxJedi/Copperline>.

## What was left behind

Bellatrix's own harness (`tools/harness/`) and its Rigel backend
(`src/machine/machine_rigel*.c`) are not copied here: both depend on the
Bellatrix machine layer — Zorro autoconfig, RTG, lide.device, ODFS, expansions,
the PAL host abstraction. Rigel's `harness/` covers the same ground for a plain
Amiga machine without any of it. The one piece taken from them directly is the
CPU/CCK clock integrator, which is why `harness.c` carries the odd-cycle
remainder across timeslices.
