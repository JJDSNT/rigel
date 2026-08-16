# Next Steps

## Immediate — what the harness found

The old "host integration blockers" list here is done: CIA, the serial API, RTC
in `rigel_config_t` and the audio event all exist. What replaces it comes from
running real software through `harness/` — see [`harness.md`](harness.md) for
the full record and the repro commands.

1. **The blitter line drawer is 27% fast** ← now measured
   - In cycle-exact mode Copperline's timing test puts blitter clear at 1.00
     of the reference and fill at 0.99, but a line at 0.73. It is the only
     chipset row still clearly wrong.
   - `tools/tests/timing/run.sh` reproduces in about a minute and gates on a
     baseline, so progress is visible per row.
   - Note the earlier claim here that "the blitter is 2-3x too fast" was
     measured with cycle-exact off, which is not the mode a host runs in.

2. **Vertical banding in a scrolling playfield**
   - Battle Squadron's title and menu are pixel-correct; its scrolling
     gameplay shows vertical stripes that are not in the game.
   - First rendering defect with a short deterministic repro (~1 min headless).
   - Start from `from_bellatrix/rigel_graphics_dma_scroll_investigation.md`;
     BPLCON1 scroll and bitplane modulo are the obvious suspects.

3. **AROS without Fast RAM**
   - Boots clean with Fast RAM. Without it the console handler dies with
     `PC: 0x00000008` regardless of Chip RAM size.
   - May simply be AROS wanting more memory than a stock Amiga has, but the
     failing case is the one where DMA contention on Chip RAM is heaviest, so
     it is worth confirming rather than assuming.

4. **Audio mix has no headroom**
   - Every capture peaks at exactly 32768, the absolute value of the int16
     minimum. Plausible once, suspicious every time.
   - `--audio-out FILE.wav` reports peak and RMS.

## Open issues

[`issues/`](issues/) — one file each, Bellatrix's convention.

| | |
| --- | --- |
| [ISSUE-0002](issues/ISSUE-0002.md) | `pixel_format` and `framebuffer.format` can disagree; a host reading `frame.format` is misled. Needs a design call, not just a patch. |
| [ISSUE-0003](issues/ISSUE-0003.md) | `enable_trace` is read by nothing. Wire it or remove it. |
| [ISSUE-0004](issues/ISSUE-0004.md) | Nothing exercises the config surface. Three defects hid there, including one whose test enshrined the bug. |

## Not on the critical path

[`issues/ISSUE-0001.md`](issues/ISSUE-0001.md) — loading `aros.elf` as an
alternative to `aros.rom`. Research: AROS already boots here through its ROM,
so this adds a second door to a room we are already in. The harness-side
findings are written up because they are not cheap to rediscover, but the work
should not displace the items above, and it is parked besides: the AROS side of
the question moved to Bellatrix (`docs/aros_port_contract.md`,
`AI_context/issues/ISSUE-0023`), which is rewriting the entry conditions this
would have to be built against.

## Near-Term Targets (fidelidade e completude)

- `paula_disk`: melhorar `DSKBYTR`, `DSKDATR`, `DSKSYNC`, `ADKCON`, drive-selection real DF0–DF3
- `audio`: stepping mais fiel ao DMA fetch/service por canal; `RIGEL_EVENT_AUDIO_READY` ainda não dispara
- slot scheduler: disk `_step_slot()`, audio `_step_slot()`, sprite DMA `_step_slot()`
- `denise`: BPLCON1 scroll offsets para PF2; dirty-lines bitmask; frame flags (interlace, copper-active)
- Sprite DMA: fetch em Agnus, interpretação/composição em Denise (wired, mas sem testes de integração completos)

## Medium-Term

1. Strengthen Agnus composition
   - `beam`, `dma`, `copper`, `blitter`, `bitplanes` com ownership e stepping mais explícitos
   - MMIO routing via Agnus-facing handlers, comportamento nos domains

2. Strengthen Paula composition
   - `interrupt`, `disk`, `serial`, `audio`, `input` atrás de superfícies estreitas
   - Aprofundar fidelidade antes de criar sub-módulos novos

3. Keep RTC e periféricos fora do custom MMIO
   - RTC permanece parte do Rigel mas não do custom-chip register family
   - Floppy, input, RTC APIs devem ficar host-facing e explícitas

## Architectural Rule Of Thumb

- `Rigel` deve permanecer hardware-facing, determinístico e single-thread por defeito
- A biblioteca deve ser concurrency-aware internamente, mas multicore não é meta próxima
- Domains expressam ownership e fronteiras temporais, não paralelismo prematuro

## Reference

Documento de status completo: `docs/api_status.md`
