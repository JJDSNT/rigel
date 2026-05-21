#include "paula/paula_regs.h"

#include "core/rigel_context.h"
#include "domains/audio/audio_domain.h"
#include "domains/disk/disk_domain.h"
#include "domains/input/input_domain.h"
#include "domains/interrupt/interrupt_domain.h"
#include "domains/serial/serial_domain.h"

bool paula_owns_reg(rigel_u32 addr)
{
    if (rigel_input_domain_owns_reg(addr)) {
        return true;
    }

    if (rigel_audio_domain_owns_reg(addr)) {
        return true;
    }

    if (rigel_disk_domain_owns_reg(addr)) {
        return true;
    }

    if (rigel_serial_domain_owns_reg(addr)) {
        return true;
    }

    return rigel_interrupt_domain_owns_reg(addr);
}

rigel_u16 paula_read_reg(RigelContext *ctx, rigel_u32 addr)
{
    if (ctx == NULL) {
        return 0;
    }

    if (rigel_input_domain_owns_reg(addr)) {
        return rigel_input_domain_read_reg(&ctx->chipset.paula.input, addr);
    }

    if (rigel_audio_domain_owns_reg(addr)) {
        return rigel_context_read_reg(ctx, addr);
    }

    if (rigel_disk_domain_owns_reg(addr)) {
        return rigel_disk_domain_read_reg(&ctx->chipset.paula.disk, addr);
    }

    if (rigel_serial_domain_owns_reg(addr)) {
        return rigel_serial_domain_read_reg(&ctx->chipset.paula.serial, addr);
    }

    if (rigel_interrupt_domain_owns_reg(addr)) {
        return rigel_interrupt_domain_read_reg(&ctx->chipset.paula.interrupts, addr);
    }

    return rigel_context_read_reg(ctx, addr);
}

void paula_write_reg(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    if (ctx == NULL) {
        return;
    }

    if (rigel_input_domain_owns_reg(addr)) {
        rigel_input_domain_write_reg(&ctx->chipset.paula.input, addr, value);
        rigel_context_write_reg(ctx, addr, value);
        return;
    }

    if (rigel_audio_domain_owns_reg(addr)) {
        rigel_audio_domain_write_reg(&ctx->chipset.paula.audio, addr, value);
        rigel_context_write_reg(ctx, addr, value);
        return;
    }

    if (rigel_disk_domain_owns_reg(addr)) {
        rigel_disk_domain_write_reg(&ctx->chipset.paula.disk, addr, value);

        switch (addr) {
        case RIGEL_REG_DSKLEN:
            rigel_context_write_reg(ctx, addr, ctx->chipset.paula.disk.dsklen);
            break;
        case RIGEL_REG_ADKCON:
            rigel_context_write_reg(ctx, addr, ctx->chipset.paula.disk.adkcon);
            break;
        default:
            rigel_context_write_reg(ctx, addr, value);
            break;
        }
        return;
    }

    if (rigel_serial_domain_owns_reg(addr)) {
        rigel_serial_domain_write_reg(&ctx->chipset.paula.serial, addr, value);

        switch (addr) {
        case RIGEL_REG_SERPER:
            rigel_context_write_reg(ctx, addr, ctx->chipset.paula.serial.serper);
            break;
        default:
            rigel_context_write_reg(ctx, addr, value);
            break;
        }
        return;
    }

    if (rigel_interrupt_domain_owns_reg(addr)) {
        rigel_interrupt_domain_write_reg(&ctx->chipset.paula.interrupts, addr, value);
        rigel_context_write_reg(
            ctx,
            addr,
            rigel_interrupt_domain_read_reg(&ctx->chipset.paula.interrupts, addr)
        );
        return;
    }

    rigel_context_write_reg(ctx, addr, value);
}
