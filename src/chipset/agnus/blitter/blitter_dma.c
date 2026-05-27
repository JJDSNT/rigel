#include "blitter.h"
#include "blitter_line.h"

void blitter_step_dma(
    BlitterState *b,
    BlitterMemory mem,
    BlitterIrqSink irq,
    uint32_t dma_slots
) {
    uint32_t i;

    if (!b->busy) {
        return;
    }

    /* LINE mode: one pixel per DMA slot using the incremental Bresenham path. */
    if (b->cmd.mode == BLITTER_MODE_LINE) {
        for (i = 0; i < dma_slots; i++) {
            if (blitter_line_done(b)) {
                break;
            }

            blitter_line_step(b, mem, irq);
            b->dma_slots_consumed++;
            if (b->cycles_remaining > 0u) {
                b->cycles_remaining--;
            }

            if (blitter_line_done(b)) {
                b->cycles_remaining = 0;
                blitter_publish_result(b);
                blitter_force_finish(b, irq);
                break;
            }
        }
        return;
    }

    /* COPY mode: execute the semantic backend on the first grant, then count
     * down DMA time until the result becomes externally visible. */

    if (b->exec_state == BLITTER_EXEC_PENDING) {
        bool ok = blitter_execute_reference(b, mem);

        if (!ok) {
            b->busy             = false;
            b->exec_state       = BLITTER_EXEC_IDLE;
            b->cycles_remaining = 0;
            return;
        }

        b->exec_state = BLITTER_EXEC_RUNNING;
    }

    b->dma_slots_consumed += dma_slots;

    if (b->cycles_remaining > dma_slots) {
        b->cycles_remaining -= dma_slots;
        return;
    }

    b->cycles_remaining = 0;
    blitter_publish_result(b);
    blitter_force_finish(b, irq);
}
