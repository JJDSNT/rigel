# Timing Model

## Fundamento

O tempo no Rigel é um contador monotônico de ciclos (`rigel_cycle_t`, `uint64_t`).
Tudo que o chipset produz — IRQ, vídeo, DMA, copper — é um evento nesse eixo de tempo.
O host avança o tempo explicitamente; Rigel nunca avança sozinho.

## DMA slot sequence como coração do timing

No Amiga, Agnus divide cada linha do raster em slots fixos. A cada CCK, ela decide quem
tem o barramento: refresh, disk, audio, bitplanes, sprites, copper/blitter, CPU. Isso não
é só scheduling de DMA — é o que faz o beam avançar, o que dispara VBL/HBL, o que cadencia
os períodos de audio, o que sincroniza o copper.

Uma scanline tem ~227 CCKs (NTSC) ou ~284 CCKs (PAL). O beam avança slot a slot.
Eventos são posições no slot sequence, não durações em ciclos nus.

## Temporal API

```c
rigel_cycle_t       rigel_get_time(const RigelContext *ctx);
rigel_cycle_t       rigel_get_next_deadline(const RigelContext *ctx);
rigel_step_result_t rigel_step(RigelContext *ctx, rigel_cycle_t cycles);
rigel_step_result_t rigel_step_until(RigelContext *ctx, rigel_cycle_t target_time);
```

`rigel_get_next_deadline()` responde: "até quando posso avançar sem perder um evento
obrigatório de sincronização?" Isso permite que o host avance o CPU exatamente até o
próximo ponto relevante, sem passo fixo arbitrário.

`rigel_step_result_t` carrega o tempo atual e um bitmask de eventos:

```c
typedef struct rigel_step_result {
    rigel_cycle_t time;
    rigel_u32     events; /* rigel_event_flags_t */
} rigel_step_result_t;
```

Eventos notificados: `IRQ_CHANGED`, `BLIT_DONE`, `VBLANK`, `HBLANK`, `FRAME_READY`,
`AUDIO_READY`, `COPPER`, `DMA_CHANGED`, `BUS_CHANGED`, `DEADLINE`.

## Abordagens de step (evolução planejada)

### A — Cycle-step (atual, provisório)

O host avança N ciclos. `next_deadline` é aproximado (blitter ou fallback de 1 scanline).
Simples, funciona para hosts sem integração fina de bus.

### B — Slot-step (cycle-exact, futuro)

```c
rigel_step_result_t rigel_step_slot(RigelContext *ctx); /* 1 DMA slot */
```

Requer beam completamente modelado (linha + frame + zonas VBL/HBL) e slot scheduler
no Agnus. `next_deadline` deixa de ser estimativa.

### C — Event-driven sobre slots (alvo)

A API pública não muda. Internamente, `step_until` caminha slot a slot. `next_deadline`
delega ao slot scheduler. Rigel expõe a API da abordagem A; o mecanismo interno é B.

## Frame pacing e drift correction

Rigel é o relógio de referência — não corrige drift. O host mede o tempo real, compara
com o tempo simulado e aplica a correção. Rigel expõe os fatos de que o host precisa:

```c
uint32_t rigel_get_clock_hz(const RigelContext *ctx);    /* 7093790 PAL / 7159090 NTSC */
uint32_t rigel_get_line_cycles(const RigelContext *ctx); /* ciclos por scanline atual */
uint32_t rigel_get_frame_cycles(const RigelContext *ctx); /* ciclos por frame completo */
uint64_t rigel_cycles_to_us(rigel_cycle_t cycles, uint32_t clock_hz);
rigel_cycle_t rigel_us_to_cycles(uint64_t microseconds, uint32_t clock_hz);
```

Loop canônico:

```c
uint64_t frame_start     = host_get_time_us();
rigel_step_until(rigel, rigel_get_time(rigel) + rigel_get_frame_cycles(rigel));
int64_t drift = (int64_t)(host_get_time_us() - frame_start)
              - (int64_t)rigel_cycles_to_us(rigel_get_frame_cycles(rigel),
                                            rigel_get_clock_hz(rigel));
host_apply_frame_correction(drift);
```

A política de correção (sleep, skip, fast-forward) pertence ao host.

## Estado atual do beam

O beam hoje só tem `hpos` e `vpos` sem wrapping de linha/frame. Qualquer deadline
sub-scanline é estimativa. O modelo completo (linha + frame + zonas VBL/HBL)
é pré-requisito para a abordagem C.
