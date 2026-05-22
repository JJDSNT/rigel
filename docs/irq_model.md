# IRQ Model

## Scope

O modelo de interrupção cobre requisição, máscara e consulta de estado pendente.
A entrega ao processador pertence ao host.

## Registradores

- `INTREQ` (`0x09C`): fontes requisitadas
- `INTENA` (`0x09A`): fontes habilitadas (bit 14 = INTEN master enable)
- IPL: nível de prioridade resultante (0–6)

Estado pendente = `INTREQ & INTENA & INTEN`. O IPL é derivado da prioridade mais
alta entre as fontes pendentes.

## Fontes de interrupção

| Bit  | Nome    | Fonte                          |
|------|---------|--------------------------------|
| 0    | TBE     | Serial transmit buffer empty   |
| 1    | DSKBLK  | Disk DMA block done            |
| 2    | SOFT    | Software interrupt             |
| 3    | PORTS   | CIA-A / external               |
| 4    | COPER   | Copper                         |
| 5    | VERTB   | Vertical blank                 |
| 6    | BLIT    | Blitter done                   |
| 7–10 | AUD0–3  | Audio channel DMA              |
| 11   | RBF     | Serial receive buffer full     |
| 12   | DSKSYN  | Disk sync word match           |
| 13   | EXTER   | CIA-B / external               |

## Integração com o host

Rigel notifica mudanças via `RIGEL_EVENT_IRQ_CHANGED` no `rigel_step_result_t`.
O host consulta `rigel_get_ipl()` e entrega a interrupção ao CPU core:

```c
rigel_step_result_t r = rigel_step_until(rigel, target);

if (r.events & RIGEL_EVENT_IRQ_CHANGED)
    cpu_set_ipl(cpu, rigel_get_ipl(rigel));
```

Rigel não sabe o que é autovector, IACK ou ciclo de interrupção — isso é
responsabilidade do host e do CPU core integrado.

## Queries diretas

```c
rigel_u16 rigel_get_intreq(const RigelContext *ctx);
rigel_u16 rigel_get_intena(const RigelContext *ctx);
rigel_u8  rigel_get_ipl(const RigelContext *ctx);
```
