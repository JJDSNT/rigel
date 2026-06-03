#include "copper_exec.h"
#include "agnus/copper/copper.h"
#include "core/rigel_context.h"
#include "debug/log.h"
#include "mmio/custom_regs.h"

/* copper_exec_move: IR1 = register address (bits 8:1, bit 0 must be 0)
 *                   IR2 = 16-bit data value to write.
 * Respects COPCON CDANG: when clear, writes below 0x80 are blocked; when set,
 * writes below 0x40 are blocked. */
void copper_exec_move(RigelContext *ctx, rigel_u16 ir1, rigel_u16 ir2)
{
    rigel_u32 reg;
    bool cdang;

    if (ctx == NULL) return;

    reg   = (rigel_u32)(ir1 & 0x01FEu);
    cdang = (ctx->chipset.agnus.copper.copcon & 0x0002u) != 0;
    if ((!cdang && reg < 0x80u) || (cdang && reg < 0x40u)) {
        return;
    }

    ctx->chipset.denise.output.pending_flags |= (rigel_u32)RIGEL_FRAME_COPPER_ACTIVE;
    if (reg == 0x096u || reg == 0x100u) {
        static unsigned trace_count = 0u;
        if (trace_count < 512u) {
            rigel_log_event_t event = {
                RIGEL_LOG_EVENT_COPPER_WRITE,
                "copper_write",
                {
                    reg,
                    ir2,
                    ctx->chipset.agnus.copper.program_counter & 0x00ffffffu,
                    ctx->chipset.agnus.beam.hpos,
                    ctx->chipset.agnus.beam.vpos,
                    (rigel_u32)(ctx->chipset.agnus.beam.frame_count & 0xffffffffu),
                    (rigel_u32)(ctx->chipset.agnus.beam.frame_count >> 32)
                },
                7u
            };
            rigel_log_event(&event);
            trace_count++;
        }
    }
    custom_regs_write16(ctx, reg, ir2);
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
