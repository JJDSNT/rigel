#include "mmio/custom_regs.h"

#include "agnus/agnus_regs.h"
#include "core/rigel_context.h"
#include "denise/denise_regs.h"
#include "paula/paula_regs.h"

bool rigel_custom_is_valid_reg(rigel_u32 addr)
{
    return (addr & 1u) == 0 && addr <= RIGEL_CUSTOM_END;
}

rigel_custom_domain_t rigel_custom_domain_for_reg(rigel_u32 addr)
{
    if (!rigel_custom_is_valid_reg(addr)) {
        return RIGEL_DOMAIN_UNKNOWN;
    }

    switch (addr) {
    case RIGEL_REG_DMACON:
        return RIGEL_DOMAIN_AGNUS;
    case RIGEL_REG_INTENA:
    case RIGEL_REG_DSKDATR:
    case RIGEL_REG_ADKCONR:
    case RIGEL_REG_DSKBYTR:
    case RIGEL_REG_DSKPTH:
    case RIGEL_REG_DSKPTL:
    case RIGEL_REG_DSKLEN:
    case RIGEL_REG_DSKSYNC:
    case RIGEL_REG_ADKCON:
    case RIGEL_REG_INTREQ:
        return RIGEL_DOMAIN_PAULA;
    case RIGEL_REG_COLOR00:
        return RIGEL_DOMAIN_DENISE;
    default:
        if (agnus_owns_reg(addr)) {
            return RIGEL_DOMAIN_AGNUS;
        }

        return paula_owns_reg(addr) ? RIGEL_DOMAIN_PAULA : RIGEL_DOMAIN_UNKNOWN;
    }
}

rigel_u16 custom_regs_read16(RigelContext *ctx, rigel_u32 addr)
{
    if (ctx == NULL || !rigel_custom_is_valid_reg(addr)) {
        return 0;
    }

    switch (rigel_custom_domain_for_reg(addr)) {
    case RIGEL_DOMAIN_AGNUS:
        return agnus_read_reg(ctx, addr);
    case RIGEL_DOMAIN_PAULA:
        return paula_read_reg(ctx, addr);
    case RIGEL_DOMAIN_DENISE:
        return denise_read_reg(ctx, addr);
    default:
        return rigel_context_read_reg(ctx, addr);
    }
}

void custom_regs_write16(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    if (ctx == NULL || !rigel_custom_is_valid_reg(addr)) {
        return;
    }

    switch (rigel_custom_domain_for_reg(addr)) {
    case RIGEL_DOMAIN_AGNUS:
        agnus_write_reg(ctx, addr, value);
        break;
    case RIGEL_DOMAIN_PAULA:
        paula_write_reg(ctx, addr, value);
        break;
    case RIGEL_DOMAIN_DENISE:
        denise_write_reg(ctx, addr, value);
        break;
    case RIGEL_DOMAIN_UNKNOWN:
    default:
        rigel_context_write_reg(ctx, addr, value);
        break;
    }
}
