#ifndef RIGEL_AGNUS_BLITTER_LINE_H
#define RIGEL_AGNUS_BLITTER_LINE_H

#include "blitter.h"

/* Blitter line-draw mode (Bresenham).
 *
 * In line mode the blitter draws one pixel per DMA cycle using:
 *   APT  — texture/pattern pointer
 *   BPT  — error accumulator (signed, 16-bit)
 *   DPT  — destination pointer (the line pixel)
 *   BLTCON1 — octant, start-pixel bit, single-bit mode
 *
 * One call to blitter_line_step() advances one pixel. */

void blitter_line_step(BlitterState *b, BlitterMemory mem, BlitterIrqSink irq);
bool blitter_line_done(const BlitterState *b);

#endif
