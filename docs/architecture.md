# Architecture

## Overview

`riegel` e uma biblioteca C organizada por modulos para MMIO, IRQ, timing e subsistemas internos.

## Modules

- `core`: inicializacao, contexto e contagem de ciclos
- `mmio`: acesso a registradores
- `irq`: estado e mascara de interrupcoes
- `agnus`: beam, DMA, copper, blitter e bitplanes
- `denise`: video, sprites e cores
- `paula`: audio, disk e serial
- `cia`: timers, TOD e portas
- `bus`: interfaces de acesso compartilhado
- `runtime`: execucao do loop de simulacao
- `util`: log e trace

## Boundary

Este projeto representa apenas o chipset e seu estado interno. Integracao com CPU, memoria global e plataforma fica fora do nucleo.
