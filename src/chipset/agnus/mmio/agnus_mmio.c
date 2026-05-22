#include "agnus/mmio/agnus_mmio.h"

#include "agnus/copper/copper_regs.h"
#include "domains/copper/copper_domain.h"
#include "domains/blitter/blitter_domain.h"
#include "rigel/rigel_custom.h"

rigel_u16 rigel_agnus_mmio_read_impl(RigelContext *ctx, rigel_u32 addr);
void rigel_agnus_mmio_write_impl(RigelContext *ctx, rigel_u32 addr, rigel_u16 value);

bool rigel_agnus_mmio_has_reg(rigel_u32 addr)
{
    if (addr == RIGEL_REG_DMACON ||
        addr == RIGEL_REG_DDFSTRT ||
        addr == RIGEL_REG_DDFSTOP) {
        return true;
    }

    if (rigel_copper_regs_owns_reg(addr)) {
        return true;
    }

    return rigel_blitter_domain_owns_reg(addr);
}

bool rigel_agnus_mmio_owns_reg(rigel_u32 addr)
{
    return rigel_agnus_mmio_has_reg(addr);
}

rigel_u16 rigel_agnus_mmio_read(RigelContext *ctx, rigel_u32 addr)
{
    return rigel_agnus_mmio_read_impl(ctx, addr);
}

void rigel_agnus_mmio_write(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    rigel_agnus_mmio_write_impl(ctx, addr, value);
}
