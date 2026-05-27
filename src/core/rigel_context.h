#ifndef RIGEL_CONTEXT_H
#define RIGEL_CONTEXT_H

#include "chipset/chipset.h"
#include "rigel/rigel_config.h"
#include "rigel/rigel_types.h"

struct RigelContext {
    rigel_config_t config;
    RigelChipset chipset;
};

rigel_chip_ram_if_t rigel_context_chip_ram(RigelContext *ctx);
rigel_u16 rigel_context_read_reg(const RigelContext *ctx, rigel_u32 addr);
void rigel_context_write_reg(RigelContext *ctx, rigel_u32 addr, rigel_u16 value);

#endif
