# Temporal API + Bus Observation API

## Decision

Two coordinated APIs were designed to replace the current `void rigel_step()`:

### Temporal API (`rigel_time.h`)

Answers "when does something relevant happen" — for scheduling.

```c
typedef uint64_t rigel_cycle_t;

typedef enum rigel_event_flags {
    RIGEL_EVENT_NONE        = 0,
    RIGEL_EVENT_DEADLINE    = 1 << 0,
    RIGEL_EVENT_IRQ_CHANGED = 1 << 1,
    RIGEL_EVENT_FRAME_READY = 1 << 2,
    RIGEL_EVENT_AUDIO_READY = 1 << 3,
    RIGEL_EVENT_BUS_CHANGED = 1 << 4,
    RIGEL_EVENT_DMA_CHANGED = 1 << 5,
    RIGEL_EVENT_BLIT_DONE   = 1 << 6,
    RIGEL_EVENT_COPPER      = 1 << 7,
    RIGEL_EVENT_VBLANK      = 1 << 8,
    RIGEL_EVENT_HBLANK      = 1 << 9
} rigel_event_flags_t;

typedef struct rigel_step_result {
    rigel_cycle_t time;
    uint32_t events;
} rigel_step_result_t;

rigel_cycle_t       rigel_get_time(const RigelContext *ctx);
rigel_cycle_t       rigel_get_next_deadline(const RigelContext *ctx);
rigel_step_result_t rigel_step(RigelContext *ctx, rigel_cycle_t cycles);
rigel_step_result_t rigel_step_until(RigelContext *ctx, rigel_cycle_t target_time);
```

`rigel_step` continua existindo — a quebra é só no tipo de retorno (`void` → `rigel_step_result_t`). Em C isso é source-compatible para quem ignora o retorno.

### Bus Observation API (`rigel_bus.h`)

Answers "who owns the classic bus at this instant" — for contention, wait states, PiStorm/Emu68-class integration.

```c
typedef enum rigel_bus_owner { ... } rigel_bus_owner_t;
typedef enum rigel_bus_flags { ... } rigel_bus_flags_t;

typedef struct rigel_bus_state {
    rigel_cycle_t    time;
    rigel_bus_owner_t owner;
    uint32_t          active_dma;       // renomeado de flags para clareza
    bool              cpu_can_access_chip_ram;
    bool              cpu_can_access_custom;
    bool              cpu_would_stall;  // advisory, não prescritivo
    rigel_cycle_t     next_change;
} rigel_bus_state_t;

rigel_bus_state_t rigel_get_bus_state(const RigelContext *ctx);
rigel_cycle_t     rigel_get_next_bus_change(const RigelContext *ctx);
bool              rigel_cpu_can_access_chip_ram(const RigelContext *ctx);
bool              rigel_cpu_can_access_custom(const RigelContext *ctx);
rigel_cycle_t     rigel_get_cpu_resume_time(const RigelContext *ctx);
```

## Rationale

- `rigel_get_next_deadline()` é o ganho principal: permite ao host avançar o CPU até exatamente o próximo ponto obrigatório de sincronização, sem passo fixo arbitrário.
- O bitmask de eventos no step result elimina queries desnecessárias após cada step.
- A Bus API desbloqueia integração PiStorm/Emu68 sem que o host reimplemente conhecimento de DMA slots.
- `RIGEL_EVENT_BUS_CHANGED` é a ponte entre as duas APIs: quando sobe, o host chama `rigel_get_bus_state()`.

## Naming decisions

- `cpu_would_stall` (não `cpu_is_stalled`): Rigel não possui a CPU; o campo é advisory.
- `active_dma` (não `flags`): distingue claramente de `owner` (exclusivo) vs. o que está agendado concorrentemente no slot.

## Host loop canônico

```c
while (running) {
    rigel_cycle_t deadline   = rigel_get_next_deadline(rigel);
    rigel_cycle_t bus_change = rigel_get_next_bus_change(rigel);
    rigel_cycle_t until      = deadline < bus_change ? deadline : bus_change;

    cpu_run_until(cpu, until);

    rigel_step_result_t r = rigel_step_until(rigel, cpu_get_time(cpu));

    if (r.events & RIGEL_EVENT_IRQ_CHANGED)
        cpu_set_ipl(cpu, rigel_get_ipl(rigel));

    rigel_bus_state_t bus = rigel_get_bus_state(rigel);
    if (bus.cpu_would_stall)
        cpu_wait_until(cpu, bus.next_change);
}
```

## Implementation risk

`rigel_get_next_deadline()` exige que todo subsistema interno exponha seu próximo wake time. É o ponto mais crítico de validar: se um domínio não reportar corretamente, a deadline fica silenciosamente errada.

## Frame pacing / drift correction

Rigel é o relógio de referência — não corrige drift. O host tem o relógio real
e sabe se está adiantado ou atrasado. Rigel expõe as primitivas para o host
fazer essa conta:

```c
uint32_t rigel_get_clock_hz(const RigelContext *ctx);     /* 7093790 PAL / 7159090 NTSC */
uint32_t rigel_get_frame_cycles(const RigelContext *ctx);  /* ciclos por frame completo */
uint64_t rigel_cycles_to_us(rigel_cycle_t cycles, uint32_t clock_hz);
```

Loop canônico no host:

```c
uint64_t frame_start       = host_get_time_us();
rigel_step_until(rigel, rigel_get_time(rigel) + rigel_get_frame_cycles(rigel));
uint64_t elapsed_real      = host_get_time_us() - frame_start;
uint64_t elapsed_chipset   = rigel_cycles_to_us(rigel_get_frame_cycles(rigel),
                                                rigel_get_clock_hz(rigel));
int64_t drift = (int64_t)elapsed_real - (int64_t)elapsed_chipset;
host_apply_frame_correction(drift); /* sleep, skip, fast-forward — política do host */
```

A correção fica no host porque cada host tem sua política:
SDL com vsync usa SDL_Delay; Pi pode usar busy-wait; harness de teste ignora.

## Header layout

```
include/rigel/
  rigel_time.h    ← rigel_cycle_t, rigel_event_flags_t, rigel_step_result_t
  rigel_bus.h     ← rigel_bus_flags_t, rigel_bus_owner_t, rigel_bus_state_t
  rigel.h         ← inclui ambos
```
