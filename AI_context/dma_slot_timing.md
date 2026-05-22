# DMA Slot Sequence como coração do timing

## O que é o DMA sequence

No Amiga, Agnus divide cada linha do raster em slots fixos. A cada CCK, ela decide quem tem o barramento. A sequência é determinística: refresh, disk, audio (4 canais), bitplanes, sprites, copper/blitter, CPU. Isso não é só DMA — é o relógio de tudo: VBL, HBL, períodos de audio, copper WAITs, blitter grants.

Uma scanline NTSC tem ~227 CCKs; PAL ~284. O beam avança slot a slot. Eventos são posições no slot sequence, não durações em ciclos nus.

---

## Abordagens para step/deadline

### A — Cycle-step (atual, provisional)

```c
rigel_step_result_t rigel_step(RigelContext *ctx, rigel_cycle_t cycles);
rigel_cycle_t rigel_get_next_deadline(const RigelContext *ctx);
```

O host avança N ciclos. Internamente os domínios fazem o que sabem.
`rigel_get_next_deadline()` retorna uma estimativa (blitter ou fallback de 1 scanline).

**Prós**
- API simples e composável
- Funciona sem que os domínios exponham deadline interno
- Host controla a granularidade

**Contras**
- `next_deadline` é aproximado — pode pular eventos intra-scanline (audio slot 2, disk slot 3)
- Domínios não têm visibilidade do contexto de slot — fazem cálculos internos por ciclo
- Bus state derivado de heurísticas, não de slot real

---

### B — Slot-step (cycle-exact)

```c
rigel_step_result_t rigel_step_slot(RigelContext *ctx); // avança 1 DMA slot
```

O chipset mantém um slot scheduler. A cada slot, Agnus decide o dono. Domínios executam no seu slot.

**Prós**
- Cycle-exact por construção
- `next_deadline` é trivial: o próximo slot com evento
- Bus state é o dono do slot atual — sem heurística
- Reflete o hardware real

**Contras**
- Cada slot é 2 CCKs — chamadas muito frequentes para períodos longos
- Exige que o beam esteja completamente modelado (linha, frame, VBL zone)
- Cada domínio precisa registrar seus slots no scheduler
- Implementação mais complexa

---

### C — Event-driven sobre slots (síntese)

```c
rigel_step_result_t rigel_step_until(RigelContext *ctx, rigel_cycle_t target_time);
rigel_cycle_t rigel_get_next_deadline(const RigelContext *ctx);
```

A API pública é a atual (Temporal API). Internamente, `step_until` caminha slot a slot,
processa cada um, e para no deadline ou no target. `next_deadline` pergunta ao slot scheduler
qual o próximo slot com evento, não estima.

**Prós**
- API pública não muda
- Cycle-exact internamente
- Host simples usa `step_until` e ignora slots
- Host avançado usa bus state que agora é preciso

**Contras**
- Requer a implementação do slot scheduler antes de qualquer outra melhoria de fidelidade
- O slot scheduler é pré-requisito para VBL, HBL, audio, disk — não pode ser feito por partes

---

## Decisão a tomar (futura, não agora)

A Abordagem C é o alvo arquitetural. Significa:

1. Beam completamente modelado (linha + frame + zonas VBL/HBL)
2. Slot scheduler central no Agnus que determina o dono de cada slot
3. Cada domínio registra suas posições de slot (estáticas por DMACON, dinâmicas para copper/blitter)
4. `step_until` vira um loop sobre `step_slot` interno
5. `next_deadline` delega ao scheduler

A API pública (Temporal + Bus) não muda. A mudança é interna ao chipset.

---

## Por que o beam atual é insuficiente

O beam hoje só tem `hpos` e `vpos` sem wrapping de linha/frame. O slot sequence depende da posição vertical:

- Linhas de VBL (0–25 NTSC): sem bitplane/sprite DMA, mais slots livres
- Linhas ativas (26–261 NTSC): sequência completa
- Linha de VBL IRQ: posição exata onde VERTB é levantado

Sem isso, qualquer deadline sub-scanline é estimativa.

---

## Harness Musashi

O objetivo do harness (`harness/`) é verificar empiricamente que o timing está correto.

**Ideia central:** rodar Musashi como CPU, Rigel como chipset, e comparar o que o sistema produz com o que o hardware real deveria produzir.

**O que o harness consegue verificar:**
- CPU stall quando BLTPRI ativo: Musashi não deve avançar enquanto `cpu_would_stall`
- VBL IRQ no ciclo correto: `RIGEL_EVENT_VBLANK` + IPL6 no momento certo
- Audio DMA: sample fetch nos slots corretos, período respeitado
- Disk DMA: palavras MFM chegando no tempo esperado
- Copper WAIT: CPU bloqueada no beam position correto

**Estrutura:**
```
harness/
  harness.h            — tipos e API do harness
  harness.c            — main loop, memória, callbacks Musashi
  m68kconf_harness.h   — config do Musashi para o harness
  tests/
    test_vblank.c      — VBL no ciclo certo
    test_blitter_stall.c — CPU stall com BLTPRI
    test_audio_period.c  — período de audio
```

**Main loop canônico do harness:**
```c
while (!done) {
    rigel_cycle_t until = rigel_get_next_deadline(rigel);

    // Musashi executa até o deadline, checando bus a cada acesso
    int cpu_cycles = m68k_execute(until - rigel_get_time(rigel));

    rigel_step_result_t r = rigel_step_until(rigel, rigel_get_time(rigel) + cpu_cycles);

    if (r.events & RIGEL_EVENT_IRQ_CHANGED)
        m68k_set_irq(rigel_get_ipl(rigel));
}
```

O Musashi usa callbacks de memória que consultam `rigel_cpu_would_stall()` antes de
cada acesso a Chip RAM — simulando o wait state real.
