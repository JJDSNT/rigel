#ifndef RIGEL_CHIPSET_H
#define RIGEL_CHIPSET_H

#include "agnus/agnus_state.h"
#include "paula/paula_state.h"
#include "rigel/rigel_snapshot.h"
#include "rigel/rigel_types.h"

enum {
    RIGEL_CUSTOM_SPACE_SIZE = 0x200,
    RIGEL_CUSTOM_REG_COUNT = RIGEL_CUSTOM_SPACE_SIZE / 2
};

struct RigelChipset {
    rigel_u64 cycles;
    RigelAgnus agnus;
    RigelPaula paula;
    rigel_u16 custom_regs[RIGEL_CUSTOM_REG_COUNT];
};

void rigel_chipset_reset(RigelChipset *chipset);
void rigel_chipset_step(RigelContext *ctx, rigel_u32 cycles);
void rigel_chipset_take_snapshot(const RigelChipset *chipset, rigel_snapshot_t *snapshot);

rigel_u16 rigel_chipset_read_reg(const RigelChipset *chipset, rigel_u32 addr);
void rigel_chipset_write_reg(RigelChipset *chipset, rigel_u32 addr, rigel_u16 value);
void rigel_chipset_raise_irq_source(RigelChipset *chipset, rigel_u16 mask);
void rigel_chipset_clear_irq_source(RigelChipset *chipset, rigel_u16 mask);

#endif
