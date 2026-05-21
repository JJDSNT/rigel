#ifndef RIEGEL_CONTEXT_H
#define RIEGEL_CONTEXT_H

#include "chipset/chipset.h"
#include "riegel/riegel_config.h"
#include "riegel/riegel_types.h"

struct RiegelContext {
    riegel_config_t config;
    RiegelChipset chipset;
};

riegel_u16 riegel_context_read_reg(const RiegelContext *ctx, riegel_u32 addr);
void riegel_context_write_reg(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value);

#endif
