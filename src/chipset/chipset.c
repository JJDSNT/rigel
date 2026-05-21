#include "chipset/chipset.h"

#include <stddef.h>
#include <string.h>

#include "agnus/agnus_state.h"
#include "core/rigel_context.h"
#include "core/rigel_timing.h"
#include "paula/paula_interrupts.h"
#include "paula/paula_state.h"
#include "rigel/rigel_custom.h"

void rigel_chipset_reset(RigelChipset *chipset)
{
    rigel_u32 i;

    if (chipset == NULL) {
        return;
    }

    chipset->cycles = 0;
    for (i = 0; i < RIGEL_FLOPPY_DRIVE_COUNT; ++i) {
        floppy_reset(&chipset->floppy[i]);
    }
    rigel_agnus_reset(&chipset->agnus);
    rigel_paula_reset(&chipset->paula);
    rigel_paula_set_disk_drive(&chipset->paula, &chipset->floppy[RIGEL_FLOPPY_DRIVE_DF0]);
    (void)memset(chipset->custom_regs, 0, sizeof(chipset->custom_regs));
}

void rigel_chipset_step(RigelContext *ctx, rigel_u32 cycles)
{
    RigelChipset *chipset;

    if (ctx == NULL) {
        return;
    }

    chipset = &ctx->chipset;
    chipset->cycles = rigel_timing_advance(chipset->cycles, cycles);
    rigel_agnus_step(ctx, cycles);
    rigel_paula_step(&chipset->paula, cycles);

    if (disk_dma_wants_service(&chipset->paula.disk)) {
        disk_dma_service_grant(&chipset->paula.disk);
    }
}

void rigel_chipset_take_snapshot(const RigelChipset *chipset, rigel_snapshot_t *snapshot)
{
    if (chipset == NULL || snapshot == NULL) {
        return;
    }

    snapshot->cycles = chipset->cycles;
    snapshot->intreq = chipset->paula.interrupts.intreq;
    snapshot->intena = chipset->paula.interrupts.intena;
}

rigel_u16 rigel_chipset_read_reg(const RigelChipset *chipset, rigel_u32 addr)
{
    rigel_u32 index;

    if (chipset == NULL || !rigel_custom_is_valid_reg(addr)) {
        return 0;
    }

    index = addr >> 1;
    return chipset->custom_regs[index];
}

void rigel_chipset_write_reg(RigelChipset *chipset, rigel_u32 addr, rigel_u16 value)
{
    rigel_u32 index;

    if (chipset == NULL || !rigel_custom_is_valid_reg(addr)) {
        return;
    }

    index = addr >> 1;
    chipset->custom_regs[index] = value;
}

void rigel_chipset_raise_irq_source(RigelChipset *chipset, rigel_u16 mask)
{
    if (chipset == NULL) {
        return;
    }

    rigel_paula_raise_irq(&chipset->paula, mask);
    rigel_chipset_write_reg(
        chipset,
        RIGEL_REG_INTREQ,
        rigel_paula_interrupts_read_intreq(&chipset->paula.interrupts)
    );
}

void rigel_chipset_clear_irq_source(RigelChipset *chipset, rigel_u16 mask)
{
    if (chipset == NULL) {
        return;
    }

    rigel_paula_clear_irq(&chipset->paula, mask);
    rigel_chipset_write_reg(
        chipset,
        RIGEL_REG_INTREQ,
        rigel_paula_interrupts_read_intreq(&chipset->paula.interrupts)
    );
}

FloppyDrive *rigel_chipset_floppy_drive(RigelChipset *chipset, rigel_floppy_drive_id_t drive)
{
    if (chipset == NULL) {
        return NULL;
    }

    switch (drive) {
    case RIGEL_FLOPPY_DRIVE_DF0:
    case RIGEL_FLOPPY_DRIVE_DF1:
    case RIGEL_FLOPPY_DRIVE_DF2:
    case RIGEL_FLOPPY_DRIVE_DF3:
        return &chipset->floppy[drive];
    default:
        return NULL;
    }
}

const FloppyDrive *rigel_chipset_floppy_drive_const(const RigelChipset *chipset, rigel_floppy_drive_id_t drive)
{
    if (chipset == NULL) {
        return NULL;
    }

    switch (drive) {
    case RIGEL_FLOPPY_DRIVE_DF0:
    case RIGEL_FLOPPY_DRIVE_DF1:
    case RIGEL_FLOPPY_DRIVE_DF2:
    case RIGEL_FLOPPY_DRIVE_DF3:
        return &chipset->floppy[drive];
    default:
        return NULL;
    }
}
