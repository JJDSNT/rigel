#ifndef RIGEL_AGNUS_BUS_CHIP_BUS_H
#define RIGEL_AGNUS_BUS_CHIP_BUS_H

#include "rigel/rigel_types.h"

/* Chip bus access helpers — thin wrappers over rigel_chip_ram_if_t.
 *
 * All DMA sub-modules (copper, blitter, bitplanes, sprites, disk, audio)
 * use these rather than calling the chip-RAM callbacks directly.
 * Centralises address masking and alignment enforcement. */

rigel_u16 chip_bus_read16(rigel_chip_ram_if_t mem, rigel_u32 addr);
void      chip_bus_write16(rigel_chip_ram_if_t mem, rigel_u32 addr, rigel_u16 val);

/* Address mask for OCS Agnus (512 KB chip RAM window) */
#define CHIP_BUS_OCS_MASK  0x0007FFFFu
/* Address mask for ECS Agnus (1 MB chip RAM window) */
#define CHIP_BUS_ECS_MASK  0x000FFFFFu

#endif
