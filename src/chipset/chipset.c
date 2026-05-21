#include "chipset/chipset.h"

#include <stddef.h>
#include <string.h>

#include "agnus/agnus_state.h"
#include "core/riegel_timing.h"
#include "paula/paula_interrupts.h"
#include "paula/paula_state.h"
#include "riegel/riegel_custom.h"

void riegel_chipset_reset(RiegelChipset *chipset)
{
    if (chipset == NULL) {
        return;
    }

    chipset->cycles = 0;
    riegel_agnus_reset(&chipset->agnus);
    riegel_paula_reset(&chipset->paula);
    (void)memset(chipset->custom_regs, 0, sizeof(chipset->custom_regs));
}

void riegel_chipset_step(RiegelChipset *chipset, riegel_u32 cycles)
{
    if (chipset == NULL) {
        return;
    }

    chipset->cycles = riegel_timing_advance(chipset->cycles, cycles);
    riegel_paula_step(&chipset->paula, cycles);

    if (disk_dma_wants_service(&chipset->paula.disk)) {
        disk_dma_service_grant(&chipset->paula.disk);
    }
}

void riegel_chipset_take_snapshot(const RiegelChipset *chipset, riegel_snapshot_t *snapshot)
{
    if (chipset == NULL || snapshot == NULL) {
        return;
    }

    snapshot->cycles = chipset->cycles;
    snapshot->intreq = chipset->paula.interrupts.intreq;
    snapshot->intena = chipset->paula.interrupts.intena;
}

riegel_u16 riegel_chipset_read_reg(const RiegelChipset *chipset, riegel_u32 addr)
{
    riegel_u32 index;

    if (chipset == NULL || !riegel_custom_is_valid_reg(addr)) {
        return 0;
    }

    index = addr >> 1;
    return chipset->custom_regs[index];
}

void riegel_chipset_write_reg(RiegelChipset *chipset, riegel_u32 addr, riegel_u16 value)
{
    riegel_u32 index;

    if (chipset == NULL || !riegel_custom_is_valid_reg(addr)) {
        return;
    }

    index = addr >> 1;
    chipset->custom_regs[index] = value;
}

void riegel_chipset_raise_irq_source(RiegelChipset *chipset, riegel_u16 mask)
{
    if (chipset == NULL) {
        return;
    }

    riegel_paula_raise_irq(&chipset->paula, mask);
    riegel_chipset_write_reg(
        chipset,
        RIEGEL_REG_INTREQ,
        riegel_paula_interrupts_read_intreq(&chipset->paula.interrupts)
    );
}

void riegel_chipset_clear_irq_source(RiegelChipset *chipset, riegel_u16 mask)
{
    if (chipset == NULL) {
        return;
    }

    riegel_paula_clear_irq(&chipset->paula, mask);
    riegel_chipset_write_reg(
        chipset,
        RIEGEL_REG_INTREQ,
        riegel_paula_interrupts_read_intreq(&chipset->paula.interrupts)
    );
}
