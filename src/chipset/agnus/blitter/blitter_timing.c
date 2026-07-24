#include "blitter.h"

#include <string.h>

uint32_t blitter_active_channel_count(const BlitCommand *cmd)
{
    uint32_t channels = (cmd->use_a ? 1u : 0u)
                      + (cmd->use_b ? 1u : 0u)
                      + (cmd->use_c ? 1u : 0u)
                      + (cmd->use_d ? 1u : 0u);

    return channels == 0u ? 1u : channels;
}

uint32_t blitter_word_cost(const BlitCommand *cmd)
{
    uint32_t cost = blitter_active_channel_count(cmd);

    /*
     * Extra idle cycle when the blitter writes D but does not read C: the D
     * write needs its own bus slot with no C read to pair with. This is why a
     * D-only clear costs 2 cck/word (not 1) and an A->D copy/fill costs 3 (not
     * 2) on real Agnus — the rates the Copperline timing oracle rows 23/24
     * cross-checked against FS-UAE and vAmiga. Fill mode adds no further
     * per-word cost (it resolves inside the D cycle).
     */
    if (cmd->use_d && !cmd->use_c) {
        cost += 1u;
    }

    return cost;
}

void blitter_begin_command(BlitterState *b)
{
    blitter_build_command(b);
    blitter_start_timing(b);
}

void blitter_publish_result(BlitterState *b)
{
    b->regs.bltapt = b->result.final_apt;
    b->regs.bltbpt = b->result.final_bpt;
    b->regs.bltcpt = b->result.final_cpt;
    b->regs.bltdpt = b->result.final_dpt;

    b->regs.bltadat = b->result.final_adat;
    b->regs.bltbdat = b->result.final_bdat;
    b->regs.bltcdat = b->result.final_cdat;
    b->regs.bltddat = b->result.final_ddat;
    b->regs.bltbhold = b->result.final_bhold;

    /*
     * BLTSIZV/BLTSIZH are latches on real hardware and survive a completed
     * blit.  KS2.0 relies on this: it programs BLTSIZV once and re-triggers
     * subsequent same-height blits by writing only BLTSIZH.  Clearing them
     * here made those follow-up blits execute with height 0 (no writes),
     * which dropped the boot-screen text on bitplanes 2/3.
     */
}

uint32_t blitter_estimate_cycles(const BlitCommand *cmd, bool cycle_exact)
{
    /*
     * cycle_exact selects the hardware-faithful cost model; when false the
     * legacy coarse estimate is used (one slot per word / per pixel). The mode
     * comes from rigel_config.cycle_exact via BlitterState.cycle_exact — it is
     * not a per-module flag. See ISSUE-0071.
     */

    uint32_t cycles;

    if (cmd->mode == BLITTER_MODE_LINE) {
        /*
         * Line mode draws one pixel per iteration, and each pixel is a
         * read-modify-write of the destination word: a C read plus a D write,
         * i.e. two chip-bus cycles. The legacy model charged one slot per
         * pixel, running line blits 2x too fast (Copperline oracle row 25:
         * 0.49x of the FS-UAE reference). See blitter_dma.c for the two-slot
         * cadence.
         */
        cycles = (uint32_t)cmd->height_lines;
        if (cycle_exact) {
            cycles *= 2u;
        }
    } else {
        uint32_t words = (uint32_t)cmd->width_words * (uint32_t)cmd->height_lines;

        if (cycle_exact) {
            /*
             * Real Agnus spends one chip-bus cycle per active DMA channel per
             * word (USEA/USEB/USEC/USED), plus one idle cycle when D writes
             * without a C read. The legacy estimate charged one cycle per word
             * regardless, running the blitter 2-4x too fast (the 2-3x undercharge
             * the Copperline timing oracle measured, ISSUE-0071). See
             * blitter_word_cost() for the per-word rule and its oracle basis.
             */
            cycles = words * blitter_word_cost(cmd);
        } else {
            cycles = words;
        }
    }

    if (cycles == 0) {
        cycles = 1;
    }

    return cycles;
}

void blitter_start_timing(BlitterState *b)
{
    b->busy = true;

    b->exec_state = BLITTER_EXEC_PENDING;

    memset(&b->result, 0, sizeof(b->result));
    memset(&b->line_state, 0, sizeof(b->line_state));
    b->dma_slots_consumed = 0;
    b->cycles_remaining =
        blitter_estimate_cycles(&b->cmd, b->cycle_exact);

    /*
     * Hardware starts with BLTZERO set.
     * It is cleared only if a non-zero
     * word is generated.
     */

    b->result.zero = true;
}

void blitter_force_finish(
    BlitterState *b,
    BlitterIrqSink irq
) {
    b->busy = false;

    b->exec_state = BLITTER_EXEC_IDLE;

    b->cycles_remaining = 0;

    /*
     * Publish BLIT interrupt through Paula.
     */

    if (irq.raise) {
        irq.raise(
            irq.opaque,
            BLITTER_INTREQ_BLIT
        );
    }
}
