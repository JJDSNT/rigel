# Next Steps

## Immediate — what the harness found

The old "host integration blockers" list here is done: CIA, the serial API, RTC
in `rigel_config_t` and the audio event all exist. What replaces it comes from
running real software through `harness/` — see [`harness.md`](harness.md) for
the full record and the repro commands.

1. **The per-colour-clock loop has no event skipping** ← now measured
   - An idle chipset costs 140 ns/CCK on a modern x86 desktop, against 282 for
     realtime: only 2x headroom with nothing programmed at all. A real workload
     (Demo Reel 3 under KS13 in the harness) costs 162 ns/CCK -- **16% more than
     idle** -- so the fixed per-clock cost is the whole problem and optimising
     the loaded case means optimising the empty one.
   - `gprof` says every clock asks every domain: slot scheduler 14.5%, Denise
     framebuffer sync 13.2%, beam 10.5%, `blitter_is_busy()` 7.9% at 2.25 calls
     per clock for a blitter that never runs, refresh-DMA slot ownership 6.6%,
     copper 5.3%, compositor tick 5.3%.
   - `rigel_get_next_observable_deadline()` bounds the caller's quantum but not
     the loop inside `rigel_chipset_step()`. Hypothesis 1 of
     `from_bellatrix/rigel_performance_research.md` is the direct answer, and
     this is the measurement its gate was waiting for.
   - Scaled to a Raspberry Pi 3 the gap is ~3.8x, which is what stops Bellatrix
     booting a machine with the chipset live. See
     [`issues/ISSUE-0006.md`](issues/ISSUE-0006.md) and the repro in
     [`from_bellatrix/rigel_cck_cost_measurement.md`](from_bellatrix/rigel_cck_cost_measurement.md).

2. **The blitter line drawer is 27% fast** ← now measured
   - In cycle-exact mode Copperline's timing test puts blitter clear at 1.00
     of the reference and fill at 0.99, but a line at 0.73. It is the only
     chipset row still clearly wrong.
   - `tools/tests/timing/run.sh` reproduces in about a minute and gates on a
     baseline, so progress is visible per row.
   - Note the earlier claim here that "the blitter is 2-3x too fast" was
     measured with cycle-exact off, which is not the mode a host runs in.

3. **Paula audio is only available pre-mixed**
   - `rigel_get_audio_sample()` returns one stereo pair; there is no per-voice
     access. Enough to make a machine audible, not enough for per-voice debug,
     per-voice resampling, or handing the four voices to a host mixer that
     wants channels. See [`issues/ISSUE-0007.md`](issues/ISSUE-0007.md).
   - What a host with its own mixer needs is per-voice *state* (location,
     length, period, volume, DMA) rather than rendered samples, so it can play
     the guest's own sample data itself. That also means audio is **not** gated
     on the loop above: the host mixer plays at the host's rate whatever speed
     the chipset runs at.

4. **No per-line display description** (low, revisit after 1)
   - The API's smallest unit of input is a finished pixel, so a host with a
     vector unit and a hardware compositor cannot render a line itself even
     where that would be faster. See [`issues/ISSUE-0008.md`](issues/ISSUE-0008.md).
   - Deliberately behind item 1: rendering per scanline segment inside Rigel
     helps every host with no new API, and may remove the reason to want this.

5. **Vertical banding in a scrolling playfield**
   - Battle Squadron's title and menu are pixel-correct; its scrolling
     gameplay shows vertical stripes that are not in the game.
   - First rendering defect with a short deterministic repro (~1 min headless).
   - Start from `from_bellatrix/rigel_graphics_dma_scroll_investigation.md`;
     BPLCON1 scroll and bitplane modulo are the obvious suspects.

6. **AROS without Fast RAM**
   - Boots clean with Fast RAM. Without it the console handler dies with
     `PC: 0x00000008` regardless of Chip RAM size.
   - May simply be AROS wanting more memory than a stock Amiga has, but the
     failing case is the one where DMA contention on Chip RAM is heaviest, so
     it is worth confirming rather than assuming.

7. **Audio mix has no headroom**
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
