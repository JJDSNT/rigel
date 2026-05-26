#include "blitter_line.h"
#include "blitter.h"

/* Bresenham line-draw: one pixel per DMA slot.
 * Reference: HRM Appendix C "Blitter Line Draw" algorithm.
 *
 * State across steps:
 *   b->cmd.apt  low 16 bits — signed error term (updated each step)
 *   b->cmd.cpt / dpt        — base plane address (unchanged; address computed per step)
 *   b->cmd.height_lines     — steps remaining (decremented each step)
 *   b->line_d               — horizontal/vertical displacement counter */

bool blitter_line_done(const BlitterState *b)
{
    if (!b->command_valid || !b->busy) return true;
    if (b->cmd.mode != BLITTER_MODE_LINE) return true;
    return b->cmd.height_lines == 0;
}

void blitter_line_step(BlitterState *b, BlitterMemory mem, BlitterIrqSink irq)
{
    const BlitCommand *cmd;
    int16_t error;
    int step_i;
    int d;
    int offset;
    uint32_t addr;
    uint16_t bitmask;
    uint16_t pattern;
    uint16_t pixel;
    uint16_t dval;
    uint32_t plane_addr;

    if (blitter_line_done(b)) return;

    cmd       = &b->cmd;
    error     = (int16_t)(cmd->apt & 0xFFFFu);
    d         = (int)b->line_d;
    step_i    = (int)(b->regs.bltsizv - cmd->height_lines);  /* 0-based step index */
    plane_addr = cmd->cpt & BLITTER_CHIP_ADDR_MASK;

    /* Rotate pattern by bshift */
    pattern = cmd->bdat;
    if (cmd->bshift != 0) {
        pattern = (uint16_t)((pattern >> cmd->bshift) | (pattern << (16u - cmd->bshift)));
    }

    switch (cmd->line_octant) {
    case 0:
        offset = d + (int)cmd->line_start_bit;
        addr   = plane_addr + (uint32_t)(offset >> 3) + (uint32_t)(step_i * (int)cmd->cmod);
        bitmask = (uint16_t)(0x8000u >> (offset & 15));
        break;
    case 1:
        offset = d + (int)cmd->line_start_bit;
        addr   = plane_addr + (uint32_t)(offset >> 3) - (uint32_t)(step_i * (int)cmd->cmod);
        bitmask = (uint16_t)(0x8000u >> (offset & 15));
        break;
    case 2:
        offset = d + (15 - (int)cmd->line_start_bit);
        addr   = (plane_addr + 1u) - (uint32_t)(offset >> 3) + (uint32_t)(step_i * (int)cmd->cmod);
        bitmask = (uint16_t)(0x0001u << (offset & 15));
        break;
    case 3:
        offset = d + (int)cmd->line_start_bit;
        addr   = plane_addr + (uint32_t)(offset >> 3) - (uint32_t)(step_i * (int)cmd->cmod);
        bitmask = (uint16_t)(0x8000u >> (offset & 15));
        break;
    case 4:
        offset = step_i + (int)cmd->line_start_bit;
        addr   = plane_addr + (uint32_t)(offset >> 3) + (uint32_t)(d * (int)cmd->cmod);
        bitmask = (uint16_t)(0x8000u >> (offset & 15));
        break;
    case 5:
        offset = step_i + (15 - (int)cmd->line_start_bit);
        addr   = (plane_addr + 1u) - (uint32_t)(offset >> 3) + (uint32_t)(d * (int)cmd->cmod);
        bitmask = (uint16_t)(0x0001u << (offset & 15));
        break;
    case 6:
        offset = step_i + (int)cmd->line_start_bit;
        addr   = plane_addr + (uint32_t)(offset >> 3) - (uint32_t)(d * (int)cmd->cmod);
        bitmask = (uint16_t)(0x8000u >> (offset & 15));
        break;
    case 7:
    default:
        offset = step_i + (15 - (int)cmd->line_start_bit);
        addr   = (plane_addr + 1u) - (uint32_t)(offset >> 3) - (uint32_t)(d * (int)cmd->cmod);
        bitmask = (uint16_t)(0x0001u << (offset & 15));
        break;
    }

    addr &= BLITTER_CHIP_ADDR_MASK;

    pixel = (mem.read16 != NULL) ? mem.read16(mem.opaque, addr) : 0u;

    /* minterm: A=bitmask, B=pattern, C=pixel */
    {
        uint8_t  mt  = cmd->minterm;
        uint16_t A   = bitmask;
        uint16_t B   = pattern;
        uint16_t C   = pixel;
        uint16_t nA  = (uint16_t)~A;
        uint16_t nB  = (uint16_t)~B;
        uint16_t nC  = (uint16_t)~C;
        dval = 0;
        if (mt & 0x80u) dval |=  (A &  B &  C);
        if (mt & 0x40u) dval |=  (A &  B & nC);
        if (mt & 0x20u) dval |=  (A & nB &  C);
        if (mt & 0x10u) dval |=  (A & nB & nC);
        if (mt & 0x08u) dval |= (nA &  B &  C);
        if (mt & 0x04u) dval |= (nA &  B & nC);
        if (mt & 0x02u) dval |= (nA & nB &  C);
        if (mt & 0x01u) dval |= (nA & nB & nC);
    }

    if (mem.write16 != NULL) mem.write16(mem.opaque, addr, dval);
    if (dval != 0) b->result.zero = false;
    b->result.final_ddat = dval;
    b->result.final_cdat = pixel;
    b->result.final_cpt  = addr;
    b->result.final_dpt  = addr;

    /* Update Bresenham error term and d counter */
    if (error > 0) {
        error = (int16_t)(error + cmd->amod);
        d += (cmd->line_octant == 3) ? -1 : 1;
    } else {
        error = (int16_t)(error + cmd->bmod);
    }

    b->cmd.apt = (b->cmd.apt & 0xFFFF0000u) | (uint16_t)error;
    b->line_d  = (int16_t)d;
    b->cmd.height_lines--;

    if (b->cmd.height_lines == 0) {
        b->result.final_apt = b->cmd.apt;
        b->result.final_bpt = b->cmd.bpt;
        blitter_publish_result(b);
        blitter_force_finish(b, irq);
    }
}
