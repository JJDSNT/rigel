#include "agnus/copper/copper_regs.h"

#include "agnus/copper/copper.h"
#include "core/rigel_context.h"
#include "domains/copper/copper_domain.h"
#include "rigel/rigel_custom.h"
#include "debug/log.h"

#if RIGEL_ENABLE_STDLIB_ENV
#include <stdio.h>
#include <stdlib.h>
#endif

static bool rigel_cop_reg_trace_enabled(void)
{
#if RIGEL_ENABLE_STDLIB_ENV
    static int enabled = -1;
    if (enabled < 0) {
        const char *env = getenv("RIGEL_COP_REG_TRACE");
        enabled = (env != NULL && env[0] != '\0' && env[0] != '0') ? 1 : 0;
    }
    return enabled != 0;
#else
    return false;
#endif
}

static void rigel_cop_reg_trace(RigelContext *ctx, const char *name,
                                rigel_u32 addr, rigel_u16 value,
                                rigel_u32 cop1lc_before, rigel_u32 cop2lc_before,
                                rigel_u32 pc_before)
{
#if RIGEL_ENABLE_STDLIB_ENV
    char msg[200];
    if (!rigel_cop_reg_trace_enabled() || ctx == NULL) return;
    snprintf(msg, sizeof(msg),
             "[RIGEL-COP-REG] %s addr=%03x val=%04x pc_before=%06x cop1_before=%06x cop2_before=%06x "
             "cop1_after=%06x cop2_after=%06x pc_after=%06x h=%u v=%u frame=%llu",
             name, (unsigned)addr, (unsigned)value,
             (unsigned)pc_before, (unsigned)cop1lc_before, (unsigned)cop2lc_before,
             (unsigned)ctx->chipset.agnus.copper.cop1lc,
             (unsigned)ctx->chipset.agnus.copper.cop2lc,
             (unsigned)ctx->chipset.agnus.copper.program_counter,
             (unsigned)ctx->chipset.agnus.beam.hpos,
             (unsigned)ctx->chipset.agnus.beam.vpos,
             (unsigned long long)ctx->chipset.agnus.beam.frame_count);
    rigel_log_info(msg);
#else
    (void)ctx; (void)name; (void)addr; (void)value;
    (void)cop1lc_before; (void)cop2lc_before; (void)pc_before;
#endif
}

bool rigel_copper_regs_owns_reg(rigel_u32 addr)
{
    switch (addr) {
    case RIGEL_REG_COPCON:
    case RIGEL_REG_COP1LCH:
    case RIGEL_REG_COP1LCL:
    case RIGEL_REG_COP2LCH:
    case RIGEL_REG_COP2LCL:
    case RIGEL_REG_COPJMP1:
    case RIGEL_REG_COPJMP2:
        return true;
    default:
        return false;
    }
}

rigel_u16 rigel_copper_regs_read(RigelContext *ctx, rigel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    switch (addr) {
    case RIGEL_REG_COP1LCH:
        return (rigel_u16)(ctx->chipset.agnus.copper.cop1lc >> 16);
    case RIGEL_REG_COP1LCL:
        return (rigel_u16)(ctx->chipset.agnus.copper.cop1lc & 0xffffu);
    case RIGEL_REG_COP2LCH:
        return (rigel_u16)(ctx->chipset.agnus.copper.cop2lc >> 16);
    case RIGEL_REG_COP2LCL:
        return (rigel_u16)(ctx->chipset.agnus.copper.cop2lc & 0xffffu);
    default:
        return rigel_context_read_reg(ctx, addr);
    }
}

void rigel_copper_regs_write(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    rigel_u32 cop1lc_before, cop2lc_before, pc_before;
    const char *name = NULL;

    if (ctx == NULL) {
        return;
    }

    cop1lc_before = ctx->chipset.agnus.copper.cop1lc;
    cop2lc_before = ctx->chipset.agnus.copper.cop2lc;
    pc_before     = ctx->chipset.agnus.copper.program_counter;

    switch (addr) {
    case RIGEL_REG_COPCON:
        ctx->chipset.agnus.copper.copcon = value;
        name = "COPCON";
        break;
    case RIGEL_REG_COP1LCH:
        copper_set_pointer_hi(&ctx->chipset.agnus.copper.cop1lc, value);
        name = "COP1LCH";
        break;
    case RIGEL_REG_COP1LCL:
        copper_set_pointer_lo(&ctx->chipset.agnus.copper.cop1lc, value);
        name = "COP1LCL";
        break;
    case RIGEL_REG_COP2LCH:
        copper_set_pointer_hi(&ctx->chipset.agnus.copper.cop2lc, value);
        name = "COP2LCH";
        break;
    case RIGEL_REG_COP2LCL:
        copper_set_pointer_lo(&ctx->chipset.agnus.copper.cop2lc, value);
        name = "COP2LCL";
        break;
    case RIGEL_REG_COPJMP1:
        rigel_copper_domain_jump1(&ctx->chipset.agnus.copper);
        name = "COPJMP1";
        break;
    case RIGEL_REG_COPJMP2:
        rigel_copper_domain_jump2(&ctx->chipset.agnus.copper);
        name = "COPJMP2";
        break;
    default:
        break;
    }

    if (name != NULL) {
        rigel_cop_reg_trace(ctx, name, addr, value, cop1lc_before, cop2lc_before, pc_before);
    }

    rigel_context_write_reg(ctx, addr, value);
}
