#ifndef RIGEL_CHIPSET_H
#define RIGEL_CHIPSET_H

#include "agnus/agnus_state.h"
#include "cia/cia.h"
#include "denise/denise_state.h"
#include "floppy/floppy_drive.h"
#include "paula/paula_state.h"
#include "rtc/rtc.h"
#include "rigel/rigel_floppy.h"
#include "rigel/rigel_snapshot.h"
#include "rigel/rigel_types.h"

enum {
    RIGEL_CUSTOM_SPACE_SIZE = 0x200,
    RIGEL_CUSTOM_REG_COUNT  = RIGEL_CUSTOM_SPACE_SIZE / 2,
    RIGEL_FLOPPY_DRIVE_COUNT = 4,
    RIGEL_CIA_COUNT          = 2
};

struct RigelChipset {
    rigel_u64  cycles;
    RigelAgnus agnus;
    RigelDenise denise;
    RigelPaula  paula;
    CIA_State   cia[RIGEL_CIA_COUNT]; /* cia[0]=CIA-A (PORTS/IPL2), cia[1]=CIA-B (EXTER/IPL6) */
    rigel_u32   cia_eclock_rem;       /* fractional E-clock tick accumulator */
    RigelRTC    rtc;
    FloppyDrive floppy[RIGEL_FLOPPY_DRIVE_COUNT];
    rigel_u16   custom_regs[RIGEL_CUSTOM_REG_COUNT];
};

void rigel_chipset_reset(RigelChipset *chipset);
void rigel_chipset_step(RigelContext *ctx, rigel_u32 cycles);
void rigel_chipset_take_snapshot(const RigelChipset *chipset, rigel_snapshot_t *snapshot);

rigel_u16 rigel_chipset_read_reg(const RigelChipset *chipset, rigel_u32 addr);
void rigel_chipset_write_reg(RigelChipset *chipset, rigel_u32 addr, rigel_u16 value);
void rigel_chipset_raise_irq_source(RigelChipset *chipset, rigel_u16 mask);
void rigel_chipset_clear_irq_source(RigelChipset *chipset, rigel_u16 mask);
FloppyDrive *rigel_chipset_floppy_drive(RigelChipset *chipset, rigel_floppy_drive_id_t drive);
const FloppyDrive *rigel_chipset_floppy_drive_const(const RigelChipset *chipset, rigel_floppy_drive_id_t drive);

#endif
