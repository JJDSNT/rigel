#ifndef RIEGEL_CHIPSET_H
#define RIEGEL_CHIPSET_H

#include "agnus/agnus_state.h"
#include "riegel/riegel_snapshot.h"
#include "riegel/riegel_types.h"

enum {
    RIEGEL_CUSTOM_SPACE_SIZE = 0x200,
    RIEGEL_CUSTOM_REG_COUNT = RIEGEL_CUSTOM_SPACE_SIZE / 2
};

struct RiegelChipset {
    riegel_u64 cycles;
    RiegelAgnus agnus;
    riegel_u16 intreq;
    riegel_u16 intena;
    riegel_u16 custom_regs[RIEGEL_CUSTOM_REG_COUNT];
};

void riegel_chipset_reset(RiegelChipset *chipset);
void riegel_chipset_step(RiegelChipset *chipset, riegel_u32 cycles);
void riegel_chipset_take_snapshot(const RiegelChipset *chipset, riegel_snapshot_t *snapshot);

riegel_u16 riegel_chipset_read_reg(const RiegelChipset *chipset, riegel_u32 addr);
void riegel_chipset_write_reg(RiegelChipset *chipset, riegel_u32 addr, riegel_u16 value);
void riegel_chipset_raise_intreq(RiegelChipset *chipset, riegel_u16 value);

#endif
