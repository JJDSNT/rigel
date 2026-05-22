#include "blitter_line.h"

/* TODO(blitter): implement Bresenham line-draw step.
 * Reference: HRM Appendix C "Blitter Line Draw" algorithm. */

void blitter_line_step(BlitterState *b, BlitterMemory mem, BlitterIrqSink irq)
{
    (void)b;
    (void)mem;
    (void)irq;
    /* TODO */
}

bool blitter_line_done(const BlitterState *b)
{
    (void)b;
    /* TODO */
    return true;
}
