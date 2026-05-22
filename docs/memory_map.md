# Memory Map

## Scope

Rigel works with register offsets and access interfaces, not with a full machine
memory map. Global address decoding belongs to the host.

## Custom register space

Rigel handles offsets `0x000`–`0x1FE` within the Amiga custom chip space
(`0xDFF000`–`0xDFF1FE` in the 68000 address space). The host decodes global
addresses and calls `rigel_custom_read16` / `rigel_custom_write16` with the
offset only.

Named constants for all supported registers are defined in `rigel_custom.h`.

### Key registers

| Offset  | Name      | Dir | Description                              |
|---------|-----------|-----|------------------------------------------|
| `0x000` | BLTDDAT   | R   | Blitter destination early read           |
| `0x002` | DMACONR`  | R   | DMA control read-back                    |
| `0x004` | VPOSR`    | R   | Vertical beam position (high)            |
| `0x006` | VHPOSR`   | R   | Vertical/horizontal beam position        |
| `0x008` | DSKDATR`  | R   | Disk data early read                     |
| `0x01C` | INTENAR`  | R   | Interrupt enable read-back               |
| `0x01E` | INTREQR`  | R   | Interrupt request read-back              |
| `0x020` | DSKPTH`   | W   | Disk DMA pointer high                    |
| `0x022` | DSKPTL`   | W   | Disk DMA pointer low                     |
| `0x024` | DSKLEN`   | W   | Disk DMA length and enable               |
| `0x02E` | COPCON`   | W   | Copper control (CDANG danger bit)        |
| `0x036` | SERDAT`   | W   | Serial data transmit                     |
| `0x038` | SERPER`   | W   | Serial period and control                |
| `0x07E` | DSKSYNC`  | W   | Disk sync word                           |
| `0x080` | COP1LCH`  | W   | Copper list 1 pointer high               |
| `0x082` | COP1LCL`  | W   | Copper list 1 pointer low                |
| `0x084` | COP2LCH`  | W   | Copper list 2 pointer high               |
| `0x086` | COP2LCL`  | W   | Copper list 2 pointer low                |
| `0x088` | COPJMP1`  | W   | Restart copper at list 1                 |
| `0x08A` | COPJMP2`  | W   | Restart copper at list 2                 |
| `0x09A` | INTENA`   | W   | Interrupt enable (SETCLR)                |
| `0x09C` | INTREQ`   | W   | Interrupt request (SETCLR)               |
| `0x096` | DMACON`   | W   | DMA control (SETCLR)                     |
| `0x09E` | ADKCON`   | W   | Audio/disk/UART control                  |
| `0x0A0`–`0x0D6` | AUD0–AUD3 | W | Audio channel registers (LCH, LCL, LEN, PER, VOL, DAT) |
| `0x0E0`–`0x0F4` | BPL1PTH–BPL6PTL | W | Bitplane DMA pointers |
| `0x100` | BPLCON0`  | W   | Bitplane control 0 (depth, mode)         |
| `0x102` | BPLCON1`  | W   | Bitplane scroll offsets                  |
| `0x180`–`0x1BE` | COLOR00–COLOR31 | W | Palette registers |

## DMACON bit assignments

`DMACON` uses bit 15 as a SETCLR flag (1 = set bits, 0 = clear bits).

| Bit | Name   | Channel          |
|-----|--------|------------------|
| 0   | AUD0EN | Audio channel 0  |
| 1   | AUD1EN | Audio channel 1  |
| 2   | AUD2EN | Audio channel 2  |
| 3   | AUD3EN | Audio channel 3  |
| 4   | DSKEN  | Disk DMA         |
| 5   | SPREN  | Sprite DMA       |
| 6   | BLTEN  | Blitter DMA      |
| 7   | COPEN  | Copper DMA       |
| 8   | BPLEN  | Bitplane DMA     |
| 9   | DMAEN  | Master enable    |
| 10  | BLTPRI | Blitter priority |

All individual channel enables require `DMAEN` (bit 9) to be active. The slot
scheduler enforces this; a channel without `DMAEN` is not assigned a DMA slot.

## Chip RAM access

Chip RAM is never mapped inside Rigel. The host supplies `read16`/`write16`
callbacks in `rigel_config_t`. Internal domains call those callbacks:

| Domain         | Access type | Purpose                        |
|----------------|-------------|--------------------------------|
| Bitplane fetch | read16      | Fetch plane words for Denise   |
| Audio DMA      | read16      | Fetch audio sample words       |
| Blitter DMA    | read16/write16 | Blit source/destination     |
| Disk DMA       | write16     | Write decoded MFM words        |
