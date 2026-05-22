# Integration

## Scope

Rigel integra com um host externo sem depender de CPU, frontend ou plataforma.
O host é responsável por: memória global, decodificação de endereços, CPU, ROM,
Fast RAM, e toda a camada de apresentação.

## Host loop canônico

```c
while (running) {
    rigel_cycle_t deadline    = rigel_get_next_deadline(rigel);
    rigel_cycle_t bus_change  = rigel_get_next_bus_change(rigel);
    rigel_cycle_t until       = deadline < bus_change ? deadline : bus_change;

    cpu_run_until(cpu, until);

    rigel_step_result_t r = rigel_step_until(rigel, cpu_get_time(cpu));

    if (r.events & RIGEL_EVENT_IRQ_CHANGED)
        cpu_set_ipl(cpu, rigel_get_ipl(rigel));

    if (r.events & RIGEL_EVENT_FRAME_READY)
        host_present_frame(rigel_get_frame(rigel));

    rigel_bus_state_t bus = rigel_get_bus_state(rigel);
    if (bus.cpu_would_stall)
        cpu_wait_until(cpu, bus.next_change);
}
```

Hosts simples podem ignorar `bus_change`, `cpu_would_stall`, e o frame delta —
a API foi desenhada para que integração fina seja opcional, não obrigatória.

## Dois perfis de host

**Host simples (SDL, headless):**
- Usa `rigel_step_until` com deadline fixo (1 frame)
- Checa `RIGEL_EVENT_IRQ_CHANGED` e `RIGEL_EVENT_FRAME_READY`
- Ignora bus state e scanlines individuais

**Host avançado (PiStorm, Emu68):**
- Usa `rigel_get_next_deadline` + `rigel_get_next_bus_change` para passo fino
- Respeita `cpu_would_stall` para contenção de Chip RAM com BLTPRI
- Consulta `rigel_get_bus_state` para arbitragem de bus slot a slot

## Memory-mapped I/O

O host decodifica o mapa de memória global e encaminha apenas o que pertence a Rigel.

```
CPU escreve 0x00DFF096
  → host identifica custom space (0xDFF000–0xDFF1FF)
  → host chama rigel_custom_write16(ctx, 0x096, value)
```

Rigel não conhece endereços globais. Ele recebe apenas offsets dentro do espaço
custom (0x000–0x1FE).

## Chip RAM

O host fornece callbacks de leitura e escrita para Chip RAM via `rigel_config_t`.
Rigel usa esses callbacks quando domínios internos precisam acessar memória
(blitter DMA, disk DMA, bitplane fetch).

```c
rigel_config_t config = {
    .clock_hz      = 7093790,
    .chip_ram_size = 512 * 1024,
    .chip_ram = {
        .opaque  = my_ram,
        .read16  = my_read16,
        .write16 = my_write16,
    },
};
```

## Bus observation (integração fina)

Para hosts que precisam de contenção real de bus:

```c
rigel_bus_state_t bus = rigel_get_bus_state(rigel);
/* bus.owner            — quem tem o barramento neste ciclo */
/* bus.cpu_would_stall  — advisory: condição de stall ativa (BLTPRI + blitter busy) */
/* bus.cpu_can_access_chip_ram */
/* bus.next_change      — quando o estado muda */
```

`cpu_would_stall` é advisory — Rigel não possui a CPU. O campo diz que a condição
clássica de stall está ativa; o host decide como aplicá-la ao seu CPU core.

## IRQ delivery

Rigel expõe `INTREQ`, `INTENA` e IPL. A entrega ao processador pertence ao host:

```c
if (r.events & RIGEL_EVENT_IRQ_CHANGED)
    cpu_set_ipl(cpu, rigel_get_ipl(rigel));
```

## Frame pacing

Ver `timing_model.md`. Em resumo: Rigel expõe `clock_hz`, `line_cycles` e `frame_cycles`;
o host mede o tempo real e aplica sua própria política de correção (sleep/skip).

## Harness de teste (Musashi)

`harness/` contém uma integração de referência com o Musashi (68000) para verificar
empiricamente que timing, bus contention e IRQ delivery estão corretos.

```sh
cmake -S . -B build-harness -DRIGEL_BUILD_HARNESS=ON -DRIGEL_BUILD_TESTS=OFF
cmake --build build-harness
```
