# Notes carried over from Bellatrix

Rigel was developed against Bellatrix as its host, and a lot of thinking about
Rigel ended up living there. This directory collects it.

Two kinds of thing, with different weight:

**Historical notes** — the investigations and gap analyses, from Bellatrix's
`legacy` branch. They record what was learned driving Rigel from a real host,
and they describe Rigel as it was at `external/rigel` commit `000719d`
("feat(timing): wire cycle-exact bus arbitration"), several commits behind this
tree. Check any API claim against `include/rigel/` before acting on it.

**Proposals** — the three planning documents, from Bellatrix's current `main`.
Ideas for where Rigel could go, written from a host's point of view. Not
obligations, and not a reason to abandon how Rigel does things today.

Neither kind is specification. Where either disagrees with `docs/`, `docs/`
wins.

## Proposals for where Rigel could go

These three come from Bellatrix's `main` rather than its `legacy` branch, and
they are **proposals, not descriptions**. Their own status lines say so:
"Proposed API Boundary Refinement Baseline", "Investigation Notes", "Possible
Optimization Directions".

They are ideas for future improvement written from a host's point of view.
None of them is a reason to abandon how Rigel does things today, and nothing
in Rigel is obliged to match them. Read them for the thinking; treat any
concrete instruction in them as a suggestion that has not been weighed against
Rigel's own constraints.

| File | What it proposes |
| --- | --- |
| `rigel_integration_spec.md` | How Bellatrix would like the boundary to work: MMIO registration and dispatch, endianness, autoconfig, the execution-progress and deadline model, IPL ownership and arbitration, lifecycle. 60 KB. Note that Rigel's harness already answers several of these questions differently and works — see `../harness.md` on the two clocks and on IPL being a level. |
| `rigel_api_convergence_plan.md` | A recommended refinement of the public API so the host boundary is explicit, minimal and host-neutral. This one lived in Rigel's own `docs/` until it was removed just before the harness work began, so it has been round the loop once already. 47 KB. |
| `rigel_optimization_candidates.md` | Performance opportunities at the Emu68/Bellatrix/Rigel boundary. Explicitly opportunities, with no measurements behind them — `tools/tests/timing/` is where numbers actually come from. 30 KB. |

## Earlier integration notes

| File | What it holds |
| --- | --- |
| `rigel_integration_notes.md` | The first exploratory pass at the same subject, from the `legacy` branch. Superseded by the spec above, but it records reasoning the spec dropped — why planar→chunky belongs in Denise, why `libamivideo` is a host concern. |

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
