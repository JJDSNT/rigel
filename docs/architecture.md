# Architecture

## Overview

`rigel` e uma biblioteca C para o chipset classico, organizada para manter fidelidade temporal, separacao de ownership e integracao limpa com um host externo.

O objetivo principal do core classico nao e paralelizacao agressiva. O objetivo e:

- comportamento deterministico
- execucao single-thread
- fronteiras internas claras entre subsistemas
- compatibilidade com integracao futura em runtimes maiores

Em resumo:

- `Rigel` nao e `multicore-first`
- `Rigel` deve ser `concurrency-aware`

## Internal Layers

- `domains/`: subsistemas temporais e funcionais do hardware
- `chipset/`: composicao, MMIO, wiring interno e ownership
- `runtime/`: como o core e executado no host
- `bus/`: superficies de acesso e arbitragem externa
- `rtc/`: periferico auxiliar fora do custom MMIO

## Domains

`domains/` nao deve virar "tudo que e interno". Essa camada deve conter apenas maquinas observaveis do hardware.

Direcao inicial recomendada:

- `beam`
- `dma`
- `copper`
- `blitter`

Depois:

- `video`
- `interrupt`
- `cia`
- `audio`
- `disk`

Os domains existem para:

- separar estado e responsabilidade
- explicitar fronteiras temporais
- reduzir acoplamento
- preparar integracao futura sem tornar o core monolitico

Eles nao significam, neste momento, threads separadas.

## Chipset Layer

`chipset/` e a raiz composicional do core. Essa camada deve concentrar:

- `RigelChipset`
- MMIO routing
- wiring entre domains
- ownership dos perifericos
- surfaces internas de mediacao, como IRQ e DMA

Os domains nao devem depender livremente uns dos outros por estado global. Comunicacao entre eles deve passar por wiring, interfaces estreitas ou superfícies explicitas do chipset.

## RTC

O RTC faz sentido dentro da biblioteca por conveniencia, mas nao como parte da familia de custom chips.

Portanto:

- RTC entra no escopo do `rigel`
- RTC fica fora de `rigel_custom_read16/write16`
- RTC deve ter surface propria

## Boundary

Este projeto representa apenas o chipset e seu estado interno. Integracao com CPU, memoria global e plataforma fica fora do nucleo.
