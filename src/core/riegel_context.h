#ifndef RIEGEL_CONTEXT_H
#define RIEGEL_CONTEXT_H

#include "riegel/riegel_config.h"
#include "riegel/riegel_types.h"

enum {
    RIEGEL_CUSTOM_SPACE_SIZE = 0x200,
    RIEGEL_CUSTOM_REG_COUNT = RIEGEL_CUSTOM_SPACE_SIZE / 2
};

struct RiegelContext {
    riegel_config_t config;
    riegel_u64 cycles;
    riegel_u16 dmacon;
    riegel_u16 intreq;
    riegel_u16 intena;
    riegel_u16 custom_regs[RIEGEL_CUSTOM_REG_COUNT];
};

riegel_u16 riegel_context_read_reg(const RiegelContext *ctx, riegel_u32 addr);
void riegel_context_write_reg(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value);

#endif
