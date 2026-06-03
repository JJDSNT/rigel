#include "domains/disk/disk_domain.h"

#include "core/rigel_context.h"

bool rigel_disk_domain_owns_reg(rigel_u32 addr)
{
    switch (addr) {
    case RIGEL_REG_DSKDATR:
    case RIGEL_REG_ADKCONR:
    case RIGEL_REG_DSKBYTR:
    case RIGEL_REG_DSKPTH:
    case RIGEL_REG_DSKPTL:
    case RIGEL_REG_DSKLEN:
    case RIGEL_REG_DSKSYNC:
    case RIGEL_REG_ADKCON:
        return true;
    default:
        return false;
    }
}

void rigel_disk_domain_reset(disk_state_t *disk)
{
    disk_reset(disk);
}

void rigel_disk_domain_step(disk_state_t *disk, rigel_u32 cycles)
{
    disk_step(disk, cycles);
}

void rigel_disk_domain_step_slot(RigelContext *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (rigel_disk_domain_dma_wants_service(&ctx->chipset.paula.disk)) {
        rigel_disk_domain_dma_service_grant(&ctx->chipset.paula.disk);
    }
}

int rigel_disk_domain_dma_wants_service(const disk_state_t *disk)
{
    return disk_dma_wants_service(disk);
}

void rigel_disk_domain_dma_service_grant(disk_state_t *disk)
{
    disk_dma_service_grant(disk);
}

rigel_u16 rigel_disk_domain_read_reg(const disk_state_t *disk, rigel_u32 addr)
{
    switch (addr) {
    case RIGEL_REG_DSKDATR:
        return disk_read_dskdatr(disk);
    case RIGEL_REG_ADKCONR:
        return disk != NULL ? disk->adkcon : 0;
    case RIGEL_REG_DSKBYTR:
        return disk_read_dskbytr((disk_state_t *)disk);
    case RIGEL_REG_DSKPTH:
        return disk != NULL ? (rigel_u16)((disk->dskptr >> 16) & 0xffffu) : 0;
    case RIGEL_REG_DSKPTL:
        return disk != NULL ? (rigel_u16)(disk->dskptr & 0xffffu) : 0;
    case RIGEL_REG_DSKLEN:
        return disk != NULL ? disk->dsklen : 0;
    case RIGEL_REG_DSKSYNC:
        return disk != NULL ? disk->dsksync : 0;
    default:
        return 0;
    }
}

void rigel_disk_domain_write_reg(disk_state_t *disk, rigel_u32 addr, rigel_u16 value)
{
    switch (addr) {
    case RIGEL_REG_DSKPTH:
        disk_write_dskpth(disk, value);
        return;
    case RIGEL_REG_DSKPTL:
        disk_write_dskptl(disk, value);
        return;
    case RIGEL_REG_DSKLEN:
        disk_write_dsklen(disk, value);
        return;
    case RIGEL_REG_DSKSYNC:
        disk_write_dsksync(disk, value);
        return;
    case RIGEL_REG_ADKCON:
        disk_write_adkcon(disk, value);
        return;
    default:
        return;
    }
}
