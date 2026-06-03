#include "chipset/chipset.h"

#include <stddef.h>
#include <string.h>

#include "agnus/agnus_state.h"
#include "agnus/timing/vblank.h"
#include "cia/cia.h"
#include "core/rigel_context.h"
#include "core/rigel_timing.h"
#include "denise/denise_state.h"
#include "domains/disk/disk_domain.h"
#include "paula/paula_interrupts.h"
#include "paula/paula_state.h"
#include "rigel/rigel_custom.h"

/* CIA-A runs at E-clock = CCK/5. One E-clock tick per 5 CCK. */
#define CIA_CCK_PER_ECLOCK 5u

void rigel_chipset_reset(RigelChipset *chipset)
{
    rigel_u32 i;

    if (chipset == NULL) {
        return;
    }

    chipset->cycles = 0;
    chipset->cia_eclock_rem = 0;
    for (i = 0; i < RIGEL_FLOPPY_DRIVE_COUNT; ++i) {
        floppy_reset(&chipset->floppy[i]);
    }
    rtc_reset(&chipset->rtc);
    rigel_agnus_reset(&chipset->agnus);
    rigel_denise_reset(&chipset->denise);
    rigel_paula_reset(&chipset->paula);
    rigel_paula_set_disk_drive(&chipset->paula, &chipset->floppy[RIGEL_FLOPPY_DRIVE_DF0]);

    cia_init(&chipset->cia[0], CIA_PORT_A);
    cia_init(&chipset->cia[1], CIA_PORT_B);
    cia_attach_paula(&chipset->cia[0], &chipset->paula);
    cia_attach_paula(&chipset->cia[1], &chipset->paula);
    /* /FIR0, /FIR1 and parallel port lines default to high (inactive) */
    cia_set_external_pra(&chipset->cia[0], 0xFFu);

    (void)memset(chipset->custom_regs, 0, sizeof(chipset->custom_regs));
}

void rigel_chipset_step(RigelContext *ctx, rigel_u32 cycles)
{
    RigelChipset *chipset;

    if (ctx == NULL) {
        return;
    }

    bool vblank_before;
    rigel_u32 eclock_ticks;
    rigel_u16 vpos_before;
    rigel_u16 frame_lines;
    rigel_u64 frame_before;
    rigel_u64 hsync_pulses;

    chipset = &ctx->chipset;
    chipset->cycles = rigel_timing_advance(chipset->cycles, cycles);

    vblank_before = beam_in_vblank(&chipset->agnus.beam);
    vpos_before = chipset->agnus.beam.vpos;
    frame_before = chipset->agnus.beam.frame_count;
    frame_lines = chipset->agnus.beam.frame_lines;

    rigel_paula_set_dmacon(&chipset->paula, chipset->agnus.dma.dmacon);
    rigel_agnus_step(ctx, cycles);
    rigel_paula_step(&chipset->paula, cycles);

    /* CIA-A TOD increments once per VBL (used by Exec as frame counter) */
    if (!vblank_before && beam_in_vblank(&chipset->agnus.beam)) {
        cia_tod_pulse(&chipset->cia[0], 1u);
    }

    /* CIA-B TOD follows Agnus HSYNC, matching the legacy chipset glue. */
    if (frame_lines != 0u) {
        hsync_pulses =
            ((chipset->agnus.beam.frame_count * (rigel_u64)frame_lines) + chipset->agnus.beam.vpos) -
            ((frame_before * (rigel_u64)frame_lines) + vpos_before);
        if (hsync_pulses > 0u) {
            cia_tod_pulse(&chipset->cia[1], (rigel_u32)hsync_pulses);
        }
    }

    /* CIA timers run at E-clock = CCK/5; accumulate remainder to avoid drift */
    chipset->cia_eclock_rem += cycles;
    eclock_ticks = chipset->cia_eclock_rem / CIA_CCK_PER_ECLOCK;
    chipset->cia_eclock_rem %= CIA_CCK_PER_ECLOCK;
    if (eclock_ticks > 0u) {
        cia_step(&chipset->cia[0], (uint64_t)eclock_ticks);
        cia_step(&chipset->cia[1], (uint64_t)eclock_ticks);
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
