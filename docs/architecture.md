# Architecture

## Overview

Rigel é uma biblioteca C para o chipset clássico do Amiga, organizada para manter
fidelidade temporal, separação de ownership e integração limpa com um host externo.

Objetivos do core clássico:
- comportamento determinístico
- execução single-thread por padrão
- fronteiras internas claras entre subsistemas
- portabilidade entre hosts e runtimes

Rigel é concurrency-aware internamente, mas não é multicore-first. Para o caminho
clássico, correção e determinismo importam mais do que paralelismo antecipado.

## Camadas internas

```
include/rigel/       ← superfície pública (host fala só aqui)
src/chipset/         ← composição, MMIO routing, wiring interno
src/domains/         ← máquinas de estado do hardware
src/bus/             ← interfaces de acesso e callbacks de Chip RAM
src/runtime/         ← política de execução no host
src/rtc/             ← periférico auxiliar fora do custom MMIO
```

## Superfície pública

Organizada em headers temáticos, todos reexportados por `rigel.h`:

| Header              | Conteúdo                                      |
|---------------------|-----------------------------------------------|
| `rigel_time.h`      | Temporal API: step, deadline, step_result     |
| `rigel_bus.h`       | Bus observation: estado do barramento clássico |
| `rigel_events.h`    | Bitmask de eventos (`rigel_event_flags_t`)    |
| `rigel_irq.h`       | INTREQ, INTENA, IPL                           |
| `rigel_mmio.h`      | custom_read16 / custom_write16                |
| `rigel_floppy.h`    | Insert, eject, status por drive               |
| `rigel_input.h`     | Joystick / pot injection                      |
| `rigel_rtc.h`       | RTC model e registradores                     |
| `rigel_config.h`    | Configuração de criação do contexto           |

## Temporal API

Dois contratos principais:

**"quando algo relevante acontece" → para scheduling:**
```c
rigel_cycle_t       rigel_get_time(const RigelContext *ctx);
rigel_cycle_t       rigel_get_next_deadline(const RigelContext *ctx);
rigel_step_result_t rigel_step(RigelContext *ctx, rigel_cycle_t cycles);
rigel_step_result_t rigel_step_until(RigelContext *ctx, rigel_cycle_t target_time);
```

**"quem tem o barramento neste instante" → para contenção e wait states:**
```c
rigel_bus_state_t rigel_get_bus_state(const RigelContext *ctx);
rigel_cycle_t     rigel_get_next_bus_change(const RigelContext *ctx);
bool              rigel_cpu_would_stall(const RigelContext *ctx);
rigel_cycle_t     rigel_get_cpu_resume_time(const RigelContext *ctx);
```

## Domains

`src/domains/` contém as máquinas de estado do hardware clássico:

```
beam       → posição do feixe (hpos/vpos)
dma        → DMACON, arbitragem de slots
copper     → programa copper e WAITs
blitter    → operações e DMA do blitter
interrupt  → INTREQ, INTENA, IPL
disk       → DMA e MMIO de disco
serial     → SERDAT, SERPER, TX/RX
audio      → 4 canais, DMA, período
input      → joystick, POT
```

Domains existem para separar estado, explicitar fronteiras temporais e reduzir
acoplamento. Não são threads — são superfícies de ownership.

## Chipset layer

`src/chipset/` é a raiz composicional. Concentra:
- `RigelChipset`: contexto raiz com Agnus, Paula, Denise, RTC, FloppyDrives
- MMIO routing: `rigel_custom_read16/write16` → domínio correto
- entrypoints de MMIO por chip, sem uma camada extra separada de `*regs`
- Wiring interno: IRQ sink, DMA grant, Chip RAM callbacks
- Mediação: domínios não dependem uns dos outros diretamente

```
RigelChipset
  ├── RigelAgnus
  │     ├── beam_state_t
  │     ├── dma_state_t
  │     ├── copper_state_t
  │     ├── bitplanes_state_t
  │     └── BlitterState
  ├── RigelDenise
  │     ├── registers
  │     ├── palette
  │     ├── render
  │     ├── sprites
  │     ├── video
  │     └── output/debug
  ├── RigelPaula
  │     ├── RigelInterruptDomain
  │     ├── audio domain
  │     ├── disk domain
  │     ├── serial domain
  │     └── input domain
  ├── RigelRTC
  └── FloppyDrive[4]
```

## Video output

Denise produz pixels a partir de bitplanes, sprites, HAM/EHB e palette.
Essa lógica pertence ao chipset porque planar→chunky não é conversão de formato —
é a execução real de Denise durante o DMA slot sequence.

O host recebe um `rigel_frame_t` com pixels prontos, metadata (`flags`) e
dirty tracking (`delta`). Ver `docs/video_output.md`.

## Boundary

Rigel não possui:
- mapa global de memória
- CPU
- ROM / Fast RAM
- dispositivos de plataforma
- lógica de apresentação (scale, vsync real, SDL, OpenGL)

O host integra tudo isso. Ver `docs/integration.md`.
