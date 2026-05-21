#include "mmio/custom_regs.h"

#include "agnus/agnus_regs.h"
#include "core/riegel_context.h"
#include "denise/denise_regs.h"
#include "paula/paula_regs.h"

bool riegel_custom_is_valid_reg(riegel_u32 addr)
{
    return (addr & 1u) == 0 && addr <= RIEGEL_CUSTOM_END;
}

riegel_custom_domain_t riegel_custom_domain_for_reg(riegel_u32 addr)
{
    if (!riegel_custom_is_valid_reg(addr)) {
        return RIEGEL_DOMAIN_UNKNOWN;
    }

    switch (addr) {
    case RIEGEL_REG_DMACON:
        return RIEGEL_DOMAIN_AGNUS;
    case RIEGEL_REG_INTENA:
    case RIEGEL_REG_INTREQ:
        return RIEGEL_DOMAIN_PAULA;
    case RIEGEL_REG_COLOR00:
        return RIEGEL_DOMAIN_DENISE;
    default:
        return agnus_owns_reg(addr) ? RIEGEL_DOMAIN_AGNUS : RIEGEL_DOMAIN_UNKNOWN;
    }
}

riegel_u16 custom_regs_read16(RiegelContext *ctx, riegel_u32 addr)
{
    if (ctx == NULL || !riegel_custom_is_valid_reg(addr)) {
        return 0;
    }

    switch (riegel_custom_domain_for_reg(addr)) {
    case RIEGEL_DOMAIN_AGNUS:
        return agnus_read_reg(ctx, addr);
    case RIEGEL_DOMAIN_PAULA:
        return paula_read_reg(ctx, addr);
    case RIEGEL_DOMAIN_DENISE:
        return denise_read_reg(ctx, addr);
    default:
        return riegel_context_read_reg(ctx, addr);
    }
}

void custom_regs_write16(RiegelContext *ctx, riegel_u32 addr, riegel_u16 value)
{
    if (ctx == NULL || !riegel_custom_is_valid_reg(addr)) {
        return;
    }

    switch (riegel_custom_domain_for_reg(addr)) {
    case RIEGEL_DOMAIN_AGNUS:
        agnus_write_reg(ctx, addr, value);
        break;
    case RIEGEL_DOMAIN_PAULA:
        paula_write_reg(ctx, addr, value);
        break;
    case RIEGEL_DOMAIN_DENISE:
        denise_write_reg(ctx, addr, value);
        break;
    case RIEGEL_DOMAIN_UNKNOWN:
    default:
        riegel_context_write_reg(ctx, addr, value);
        break;
    }
}
