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

**IPL is a level, not an edge.** It is a signal the host mirrors into its CPU
core, not a notification to act on once. A host that publishes it only when
`RIGEL_EVENT_IRQ_CHANGED` appears will hang:

```c
/* Wrong — this hangs. */
rigel_step_result_t r = rigel_step_until(rigel, target);
if (r.events & RIGEL_EVENT_IRQ_CHANGED)
    cpu_set_ipl(cpu, rigel_get_ipl(rigel));
```

The reason is that IPL moves outside `rigel_step`. An interrupt handler
acknowledges by writing `INTREQ`, which drops the level immediately, with no
step in between and so no event to observe. The host is left holding the old
level, the next source of the same priority raises IPL to a value the CPU is
already at, and nothing re-triggers. Kickstart 1.3 does not reach its
insert-disk screen this way.

Mirror the level after every advance *and* after every write that can move it —
which means `INTREQ` and `INTENA`, and any CIA register access, since reading
CIA ICR acknowledges and clears the CIA's pending interrupts:

```c
static void publish_ipl(host_t *h)
{
    rigel_u8 ipl = rigel_get_ipl(h->rigel);
    if (ipl == h->last_ipl) return;   /* only on a real change */
    h->last_ipl = ipl;
    cpu_set_ipl(h->cpu, ipl);
}

/* after every step */
rigel_step_result_t r = rigel_step_until(rigel, target);
publish_ipl(h);

/* and after every write that can move it */
rigel_custom_write16(rigel, reg, value);
publish_ipl(h);
```

`RIGEL_EVENT_IRQ_CHANGED` remains useful for logging or for waking a host that
is otherwise idle. It is not sufficient as the delivery trigger.

Rigel does not know about autovectors, IACK cycles, or interrupt acknowledgement
— those are the responsibility of the host and the integrated CPU core.

A worked implementation is in `harness/harness.c` (`harness_sync_ipl`), and the
history of getting it wrong is in `AI_context/harness.md`.

## Direct queries

```c
rigel_u16 rigel_get_intreq(const RigelContext *ctx);
rigel_u16 rigel_get_intena(const RigelContext *ctx);
rigel_u8  rigel_get_ipl(const RigelContext *ctx);
```
