#include "copper_exec.h"
#include "agnus/copper/copper.h"
#include "core/rigel_context.h"
#include "debug/log.h"
#include "mmio/custom_regs.h"

#include <stdio.h>

/* copper_exec_move: IR1 = register address (bits 8:1, bit 0 must be 0)
 *                   IR2 = 16-bit data value to write.
 * Respects COPCON CDANG: when clear, writes to registers < 0x40 are blocked. */
void copper_exec_move(RigelContext *ctx, rigel_u16 ir1, rigel_u16 ir2)
{
    rigel_u32 reg;
    bool cdang;

    if (ctx == NULL) return;

    reg   = (rigel_u32)(ir1 & 0x01FEu);
    cdang = (ctx->chipset.agnus.copper.copcon & 0x0002u) != 0;
    if (reg >= 0x40u || cdang) {
        if (reg == 0x096u || reg == 0x100u) {
            static unsigned trace_count = 0u;
            if (trace_count < 512u) {
                char msg[192];
                (void)snprintf(msg, sizeof(msg),
                               "[RIGEL-COPPER-W] reg=%03x write=%04x"
                               " pc=%06x beam=%03u,%03u frame=%llu",
                               (unsigned)reg,
                               (unsigned)ir2,
                               (unsigned)(ctx->chipset.agnus.copper.program_counter & 0x00ffffffu),
                               (unsigned)ctx->chipset.agnus.beam.hpos,
                               (unsigned)ctx->chipset.agnus.beam.vpos,
                               (unsigned long long)ctx->chipset.agnus.beam.frame_count);
                rigel_log_info(msg);
                trace_count++;
            }
        }
        custom_regs_write16(ctx, reg, ir2);
    }
}

/* copper_exec_skip_test: returns true if the beam is already at or past the
 * target position encoded in (ir1, ir2), meaning the next instruction should
 * be skipped.
 *
 * SKIP format: IR1 bits[15:8] = VP, bits[7:1] = HP, bit 0 = 1
 *              IR2 bits[15:8] = VPM, bits[7:1] = HPM, bit 0 = 1 */
bool copper_exec_skip_test(rigel_u16 ir1, rigel_u16 ir2,
                           rigel_u16 hpos, rigel_u16 vpos)
{
    rigel_u16 tgt_vpos = (ir1 >> 8) & 0xFFu;
    rigel_u16 tgt_hpos =  ir1 & 0xFEu;
    rigel_u16 vpmask   = (ir2 >> 8) & 0xFFu;
    rigel_u16 hpmask   =  ir2 & 0xFEu;
    return copper_beam_cmp(vpos, hpos, tgt_vpos, tgt_hpos, vpmask, hpmask);
}
