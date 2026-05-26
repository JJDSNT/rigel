# Next Steps

## Immediate — Host integration blockers (ordered by cost/benefit)

1. **CIA into chipset** ← maior gap
   - Add `CIA_State cia[2]` to `RigelChipset`
   - CIA-A attached to INTREQ PORTS (bit 3), CIA-B to INTREQ EXTER (bit 13)
   - Step CIA at E-clock rate (CCK / 5) from `rigel_chipset_step`
   - Pulse CIA-A TOD on VBL
   - Add `rigel_cia.h`: `rigel_cia_read8 / rigel_cia_write8`
   - Add `rigel_keyboard.h`: `rigel_keyboard_inject(ctx, keycode, pressed)` via `cia_receive_sdr(cia_b, ...)`

2. **Serial public API**
   - `rigel_serial.h` com `rigel_serial_receive_byte / tx_available / pop_tx_byte`
   - Wrappers directos sobre `serial_receive_byte / serial_pop_tx_byte` existentes

3. **RTC em `rigel_config_t`**
   - Adicionar `rtc_model` e `rtc_time` à config
   - Inicializar em `rigel_create` em vez de exigir chamadas pós-init

4. **`RIGEL_EVENT_AUDIO_READY`**
   - Flag `sample_ready` em `audio_state_t`, set em `audio_mix()` quando sample muda
   - Check em `rigel_step()`, disparar o evento, clear a flag

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
