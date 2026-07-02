#include "blitter.h"

#include <string.h>

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

    /*
     * BLTSIZV/BLTSIZH are latches on real hardware and survive a completed
     * blit.  KS2.0 relies on this: it programs BLTSIZV once and re-triggers
     * subsequent same-height blits by writing only BLTSIZH.  Clearing them
     * here made those follow-up blits execute with height 0 (no writes),
     * which dropped the boot-screen text on bitplanes 2/3.
     */
}

uint32_t blitter_estimate_cycles(const BlitCommand *cmd)
{
    /*
     * Very rough initial estimation.
     *
     * This is NOT cycle exact.
     * It only provides observable timing
     * coherent enough for the current early integration stage.
     *
     * Future versions can model:
     *   - DMA slot ownership
     *   - bus contention
     *   - nasty mode
     *   - line mode timing
     *   - fill mode timing
     */

    uint32_t cycles;

    if (cmd->mode == BLITTER_MODE_LINE) {
        /* Line mode advances one pixel per granted blitter DMA slot. */
        cycles = (uint32_t)cmd->height_lines;
    } else {
        cycles = (uint32_t)cmd->width_words * (uint32_t)cmd->height_lines;
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
        blitter_estimate_cycles(&b->cmd);

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
