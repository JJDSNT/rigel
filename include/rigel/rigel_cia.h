#ifndef RIGEL_CIA_H
#define RIGEL_CIA_H

#include "rigel_types.h"

typedef struct RigelContext RigelContext;

/*
 * CIA MMIO access — the host decodes CIA addresses and calls these.
 *
 * cia_id: 0 = CIA-A (0xBFExxx, odd bytes — keyboard, joystick fire, parallel)
 *         1 = CIA-B (0xBFDxxx, even bytes — floppy motor/select, serial /RxD)
 * reg:    CIA register number (bits 11-8 of the Amiga CIA address, 0x0–0xF).
 *
 * Address decoding example:
 *   0xBFE001 → CIA-A, reg = (0xBFE001 >> 8) & 0xF = 0xE → CIA_REG_CRA
 *   0xBFD000 → CIA-B, reg = (0xBFD000 >> 8) & 0xF = 0xD → CIA_REG_ICR
 */
rigel_u8 rigel_cia_read(RigelContext *ctx, rigel_u32 cia_id, rigel_u8 reg);
void     rigel_cia_write(RigelContext *ctx, rigel_u32 cia_id, rigel_u8 reg, rigel_u8 value);

#endif
