# IRQ Model

## Scope

The interrupt model covers request, masking, and pending-state queries.
Delivery to the processor belongs to the host.

## Registers

- `INTREQ` (`0x09C`): requested sources
- `INTENA` (`0x09A`): enabled sources (bit 14 = INTEN master enable)
- IPL: resulting priority level (0–6)

Pending state = `INTREQ & INTENA & INTEN`. IPL is derived from the highest
priority pending source.

## Interrupt sources

| Bit  | Name    | Source                         |
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

## IRQ sources wired in Rigel

All sources listed above fire through the same interrupt domain. The following
are actively driven by the chipset:

- **DSKBLK** — fired by disk DMA at the end of a DMA block
- **DSKSYN** — fired when the disk sync word matches DSKSYNC
- **BLIT** — fired by the blitter when it finishes an operation
- **COPER** — fired by copper when a WAIT/SKIP triggers the IRQ path
- **VERTB** — fired at the start of vertical blank
- **AUD0–3** — fired by each audio channel when its DMA buffer is exhausted

## Host integration

Rigel notifies changes via `RIGEL_EVENT_IRQ_CHANGED` in `rigel_step_result_t`.
The host queries `rigel_get_ipl()` and delivers the interrupt to the CPU core:

```c
rigel_step_result_t r = rigel_step_until(rigel, target);

if (r.events & RIGEL_EVENT_IRQ_CHANGED)
    cpu_set_ipl(cpu, rigel_get_ipl(rigel));
```

Rigel does not know about autovectors, IACK cycles, or interrupt acknowledgement
— those are the responsibility of the host and the integrated CPU core.

## Direct queries

```c
rigel_u16 rigel_get_intreq(const RigelContext *ctx);
rigel_u16 rigel_get_intena(const RigelContext *ctx);
rigel_u8  rigel_get_ipl(const RigelContext *ctx);
```
