# Next Steps

## Immediate — what the harness found

The old "host integration blockers" list here is done: CIA, the serial API, RTC
in `rigel_config_t` and the audio event all exist. What replaces it comes from
running real software through `harness/` — see [`harness.md`](harness.md) for
the full record and the repro commands.

1. **Vertical banding in a scrolling playfield** ← best lead
   - Battle Squadron's title and menu are pixel-correct; its scrolling
     gameplay shows vertical stripes that are not in the game.
   - First rendering defect with a short deterministic repro (~1 min headless).
   - Start from `from_bellatrix/rigel_graphics_dma_scroll_investigation.md`;
     BPLCON1 scroll and bitplane modulo are the obvious suspects.

2. **AROS without Fast RAM**
   - Boots clean with Fast RAM. Without it the console handler dies with
     `PC: 0x00000008` regardless of Chip RAM size.
   - May simply be AROS wanting more memory than a stock Amiga has, but the
     failing case is the one where DMA contention on Chip RAM is heaviest, so
     it is worth confirming rather than assuming.

3. **Audio mix has no headroom**
   - Every capture peaks at exactly 32768, the absolute value of the int16
     minimum. Plausible once, suspicious every time.
   - `--audio-out FILE.wav` reports peak and RMS.

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
