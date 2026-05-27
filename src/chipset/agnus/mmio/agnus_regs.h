#ifndef RIGEL_AGNUS_REGS_H
#define RIGEL_AGNUS_REGS_H

#include "rigel/rigel_custom.h"

/* Custom chip register offsets owned by Agnus (byte addresses in custom space).
 * Read-only registers have an R suffix; write-only have a W suffix.
 * Registers with no suffix are readable and writable via the same address. */

/* --- DMA control --------------------------------------------------------- */
#define AGNUS_DMACONR   0x002   /* R  DMA control (read)        */
#define AGNUS_DMACON    0x096   /* W  DMA control (write)       */

/* --- Beam position ------------------------------------------------------- */
#define AGNUS_VPOSR     0x004   /* R  vertical position high    */
#define AGNUS_VHPOSR    0x006   /* R  vertical + horizontal pos */
#define AGNUS_VPOSW     0x02A   /* W  vertical position high    */
#define AGNUS_VHPOSW    0x02C   /* W  vertical + horizontal pos */

/* --- Display window / data fetch ---------------------------------------- */
#define AGNUS_DIWSTRT   0x08E   /* W  display window start      */
#define AGNUS_DIWSTOP   0x090   /* W  display window stop       */
#define AGNUS_DDFSTRT   0x092   /* W  data fetch start          */
#define AGNUS_DDFSTOP   0x094   /* W  data fetch stop           */

/* --- Disk DMA ------------------------------------------------------------ */
#define AGNUS_DSKPTH    0x020   /* W  disk pointer high         */
#define AGNUS_DSKPTL    0x022   /* W  disk pointer low          */
#define AGNUS_DSKLEN    0x024   /* W  disk length               */
#define AGNUS_DSKDATR   0x008   /* R  disk data (early read)    */
#define AGNUS_DSKDAT    0x026   /* W  disk data (write)         */

/* --- Audio DMA ----------------------------------------------------------- */
#define AGNUS_AUD0LCH   0x0A0
#define AGNUS_AUD0LCL   0x0A2
#define AGNUS_AUD0LEN   0x0A4
#define AGNUS_AUD1LCH   0x0A6
#define AGNUS_AUD1LCL   0x0A8
#define AGNUS_AUD1LEN   0x0AA
#define AGNUS_AUD2LCH   0x0AC
#define AGNUS_AUD2LCL   0x0AE
#define AGNUS_AUD2LEN   0x0B0
#define AGNUS_AUD3LCH   0x0B2
#define AGNUS_AUD3LCL   0x0B4
#define AGNUS_AUD3LEN   0x0B6

/* --- Sprite DMA ---------------------------------------------------------- */
#define AGNUS_SPR0PTH   0x120
#define AGNUS_SPR0PTL   0x122
#define AGNUS_SPR1PTH   0x124
#define AGNUS_SPR1PTL   0x126
#define AGNUS_SPR2PTH   0x128
#define AGNUS_SPR2PTL   0x12A
#define AGNUS_SPR3PTH   0x12C
#define AGNUS_SPR3PTL   0x12E
#define AGNUS_SPR4PTH   0x130
#define AGNUS_SPR4PTL   0x132
#define AGNUS_SPR5PTH   0x134
#define AGNUS_SPR5PTL   0x136
#define AGNUS_SPR6PTH   0x138
#define AGNUS_SPR6PTL   0x13A
#define AGNUS_SPR7PTH   0x13C
#define AGNUS_SPR7PTL   0x13E

/* --- Bitplane pointers --------------------------------------------------- */
#define AGNUS_BPL1PTH   0x0E0
#define AGNUS_BPL1PTL   0x0E2
#define AGNUS_BPL2PTH   0x0E4
#define AGNUS_BPL2PTL   0x0E6
#define AGNUS_BPL3PTH   0x0E8
#define AGNUS_BPL3PTL   0x0EA
#define AGNUS_BPL4PTH   0x0EC
#define AGNUS_BPL4PTL   0x0EE
#define AGNUS_BPL5PTH   0x0F0
#define AGNUS_BPL5PTL   0x0F2
#define AGNUS_BPL6PTH   0x0F4
#define AGNUS_BPL6PTL   0x0F6

/* --- Bitplane control (Agnus-side: depth, fetch mode) ------------------- */
#define AGNUS_BPLCON0   0x100
#define AGNUS_BPLMOD1   0x108
#define AGNUS_BPLMOD2   0x10A

/* --- ECS: beam control --------------------------------------------------- */
#define AGNUS_BEAMCON0  RIGEL_REG_BEAMCON0   /* W  beam timing control (ECS) */

/* --- Copper -------------------------------------------------------------- */
#define AGNUS_COP1LCH   0x080
#define AGNUS_COP1LCL   0x082
#define AGNUS_COP2LCH   0x084
#define AGNUS_COP2LCL   0x086
#define AGNUS_COPJMP1   0x088   /* W  restart copper list 1     */
#define AGNUS_COPJMP2   0x08A   /* W  restart copper list 2     */
#define AGNUS_COPINS    0x08C   /* W  copper instruction        */
#define AGNUS_COPCON    0x02E   /* W  copper danger bit (CDANG) */

/* Blitter offsets are defined in blitter/blitter.h */

#endif
