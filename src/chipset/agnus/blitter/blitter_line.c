#include "blitter_line.h"
#include "blitter.h"

static uint16_t line_chip_read16(const BlitterMemory *mem, uint32_t addr)
{
    addr &= BLITTER_CHIP_ADDR_MASK;
    return (mem->read16 != NULL) ? mem->read16(mem->opaque, addr) : 0u;
}

static void line_chip_write16(const BlitterMemory *mem, uint32_t addr, uint16_t value)
{
    addr &= BLITTER_CHIP_ADDR_MASK;
    if (mem->write16 != NULL)
        mem->write16(mem->opaque, addr, value);
}

static uint16_t line_blitter_logic(uint8_t minterm, uint16_t A, uint16_t B, uint16_t C)
{
    uint16_t na = (uint16_t)~A;
    uint16_t nb = (uint16_t)~B;
    uint16_t nc = (uint16_t)~C;
    uint16_t D = 0;
    if (minterm & 0x80u) D |=  (A &  B &  C);
    if (minterm & 0x40u) D |=  (A &  B & nc);
    if (minterm & 0x20u) D |=  (A & nb &  C);
    if (minterm & 0x10u) D |=  (A & nb & nc);
    if (minterm & 0x08u) D |= (na &  B &  C);
    if (minterm & 0x04u) D |= (na &  B & nc);
    if (minterm & 0x02u) D |= (na & nb &  C);
    if (minterm & 0x01u) D |= (na & nb & nc);
    return D;
}

static uint16_t line_rotate_pattern(uint16_t pattern, uint8_t shift)
{
    if (shift == 0) return pattern;
    return (uint16_t)((pattern >> shift) | (pattern << (16u - shift)));
}

static void line_state_init(BlitterState *b)
{
    BlitterLineState *state = &b->line_state;
    const BlitCommand *cmd = &b->cmd;

    if (state->initialized) return;

    state->error       = (int16_t)(cmd->apt & 0xFFFFu);
    state->pattern     = line_rotate_pattern(cmd->bdat, cmd->bshift);
    state->plane_addr  = cmd->cpt & BLITTER_CHIP_ADDR_MASK;
    state->last_addr   = state->plane_addr;
    state->plane_delta = (int)cmd->cmod;
    state->offset_delta = 0;
    state->step_index  = 0;
    state->last_cdat   = cmd->cdat;
    state->last_ddat   = 0;
    state->zero        = true;
    state->initialized = true;
}

static void line_publish_partial_result(BlitterState *b)
{
    const BlitCommand *cmd = &b->cmd;
    const BlitterLineState *state = &b->line_state;

    b->result.final_apt  = (cmd->apt & 0xFFFF0000u) | (uint16_t)state->error;
    b->result.final_bpt  = cmd->bpt;
    b->result.final_cpt  = state->last_addr;
    b->result.final_dpt  = state->last_addr;
    b->result.final_adat = cmd->adat;
    b->result.final_bdat = cmd->bdat;
    b->result.final_cdat = state->last_cdat;
    b->result.final_ddat = state->last_ddat;
    b->result.zero       = state->zero;
}

void blitter_line_step(BlitterState *b, BlitterMemory mem, BlitterIrqSink irq)
{
    const BlitCommand *cmd;
    BlitterLineState *state;
    int start_pixel;
    int offset;
    uint32_t addr;
    uint16_t bitmask;
    uint16_t pixel;
    uint16_t dval;

    (void)irq;

    if (b == NULL || b->cmd.mode != BLITTER_MODE_LINE) return;

    cmd   = &b->cmd;
    state = &b->line_state;
    line_state_init(b);

    if (state->step_index >= cmd->height_lines) return;

    start_pixel = (int)cmd->line_start_bit;
    addr = state->plane_addr;

    switch (cmd->line_octant) {
    case 0:
        offset = state->offset_delta + start_pixel;
        addr = state->plane_addr +
               (uint32_t)(offset >> 3) +
               (uint32_t)(state->step_index * (unsigned)state->plane_delta);
        bitmask = (uint16_t)(0x8000u >> (offset & 15));
        break;
    case 1:
        offset = state->offset_delta + start_pixel;
        addr = state->plane_addr +
               (uint32_t)(offset >> 3) -
               (uint32_t)(state->step_index * (unsigned)state->plane_delta);
        bitmask = (uint16_t)(0x8000u >> (offset & 15));
        break;
    case 2:
        offset = state->offset_delta + (15 - start_pixel);
        addr = (state->plane_addr + 1u) -
               (uint32_t)(offset >> 3) +
               (uint32_t)(state->step_index * (unsigned)state->plane_delta);
        bitmask = (uint16_t)(0x0001u << (offset & 15));
        break;
    case 3:
        offset = state->offset_delta + start_pixel;
        addr = state->plane_addr +
               (uint32_t)(offset >> 3) -
               (uint32_t)(state->step_index * (unsigned)state->plane_delta);
        bitmask = (uint16_t)(0x8000u >> (offset & 15));
        break;
    case 4:
        offset = (int)state->step_index + start_pixel;
        addr = state->plane_addr +
               (uint32_t)(offset >> 3) +
               (uint32_t)(state->offset_delta * state->plane_delta);
        bitmask = (uint16_t)(0x8000u >> (offset & 15));
        break;
    case 5:
        offset = (int)state->step_index + (15 - start_pixel);
        addr = (state->plane_addr + 1u) -
               (uint32_t)(offset >> 3) +
               (uint32_t)(state->offset_delta * state->plane_delta);
        bitmask = (uint16_t)(0x0001u << (offset & 15));
        break;
    case 6:
        offset = (int)state->step_index + start_pixel;
        addr = state->plane_addr +
               (uint32_t)(offset >> 3) -
               (uint32_t)(state->offset_delta * state->plane_delta);
        bitmask = (uint16_t)(0x8000u >> (offset & 15));
        break;
    case 7:
    default:
        offset = (int)state->step_index + (15 - start_pixel);
        addr = (state->plane_addr + 1u) -
               (uint32_t)(offset >> 3) -
               (uint32_t)(state->offset_delta * state->plane_delta);
        bitmask = (uint16_t)(0x0001u << (offset & 15));
        break;
    }

    pixel = line_chip_read16(&mem, addr);
    state->last_cdat = pixel;
    dval = line_blitter_logic(cmd->minterm, bitmask, state->pattern, pixel);
    line_chip_write16(&mem, addr, dval);

    if (dval != 0) state->zero = false;
    state->last_ddat = dval;
    state->last_addr = addr;
    state->step_index++;

    if (state->error > 0) {
        state->error = (int16_t)(state->error + cmd->amod);
        if (cmd->line_octant == 3)
            state->offset_delta -= 1;
        else
            state->offset_delta += 1;
    } else {
        state->error = (int16_t)(state->error + cmd->bmod);
    }

    line_publish_partial_result(b);
}

bool blitter_line_done(const BlitterState *b)
{
    if (b == NULL || b->cmd.mode != BLITTER_MODE_LINE) return true;
    return b->line_state.initialized &&
           b->line_state.step_index >= b->cmd.height_lines;
}
