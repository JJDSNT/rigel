#include "blitter_line.h"
#include "blitter.h"

static uint16_t line_chip_read16(const BlitterMemory *mem, uint32_t addr)
{
    addr &= BLITTER_CHIP_ADDR_MASK;
    return (mem->read16 != NULL) ? mem->read16(mem->opaque, addr) : 0u;
}

void rigel_blt_w_trace(uint32_t addr, uint16_t value);

static void line_chip_write16(const BlitterMemory *mem, uint32_t addr, uint16_t value)
{
    addr &= BLITTER_CHIP_ADDR_MASK;
    if (mem->write16 != NULL) {
        rigel_blt_w_trace(addr, value);
        mem->write16(mem->opaque, addr, value);
    }
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

static void line_incx(BlitterLineState *state)
{
    state->x_shift++;
    if (state->x_shift == 16u) {
        state->x_shift = 0;
        state->cpt = (state->cpt + 2u) & BLITTER_CHIP_ADDR_MASK;
    }
}

static void line_decx(BlitterLineState *state)
{
    if (state->x_shift == 0u) {
        state->x_shift = 15u;
        state->cpt = (state->cpt - 2u) & BLITTER_CHIP_ADDR_MASK;
    } else {
        state->x_shift--;
    }
}

static void line_incy(BlitterLineState *state, int16_t mod)
{
    state->cpt = (uint32_t)((int32_t)state->cpt + (int32_t)mod) &
                 BLITTER_CHIP_ADDR_MASK;
    state->one_dot_count = 0;
}

static void line_decy(BlitterLineState *state, int16_t mod)
{
    state->cpt = (uint32_t)((int32_t)state->cpt - (int32_t)mod) &
                 BLITTER_CHIP_ADDR_MASK;
    state->one_dot_count = 0;
}

static void line_advance(BlitterLineState *state, const BlitCommand *cmd)
{
    if (cmd->use_a) {
        if (state->sign)
            state->error = (int16_t)(state->error + cmd->bmod);
        else
            state->error = (int16_t)(state->error + cmd->amod);
    }

    if (!state->sign) {
        if (cmd->line_octant & 0x04u) {
            if (cmd->line_octant & 0x02u)
                line_decy(state, cmd->cmod);
            else
                line_incy(state, cmd->cmod);
        } else {
            if (cmd->line_octant & 0x02u)
                line_decx(state);
            else
                line_incx(state);
        }
    }

    if (cmd->line_octant & 0x04u) {
        if (cmd->line_octant & 0x01u)
            line_decx(state);
        else
            line_incx(state);
    } else {
        if (cmd->line_octant & 0x01u)
            line_decy(state, cmd->cmod);
        else
            line_incy(state, cmd->cmod);
    }

    state->sign = state->error < 0;
}

static void line_state_init(BlitterState *b)
{
    BlitterLineState *state = &b->line_state;
    const BlitCommand *cmd = &b->cmd;

    if (state->initialized) return;

    state->error       = (int16_t)(cmd->apt & 0xFFFFu);
    state->pattern     = line_rotate_pattern(cmd->bdat, cmd->bshift);
    state->cpt         = cmd->cpt & BLITTER_CHIP_ADDR_MASK;
    state->step_index  = 0;
    state->x_shift     = cmd->line_start_bit & 0x0Fu;
    state->one_dot_count = 0;
    state->last_cdat   = cmd->cdat;
    state->last_ddat   = 0;
    state->zero        = true;
    state->sign        = cmd->line_initial_sign;
    state->initialized = true;
}

static void line_publish_partial_result(BlitterState *b)
{
    const BlitCommand *cmd = &b->cmd;
    const BlitterLineState *state = &b->line_state;

    b->result.final_apt  = (cmd->apt & 0xFFFF0000u) | (uint16_t)state->error;
    b->result.final_bpt  = cmd->bpt;
    b->result.final_cpt  = state->cpt;
    b->result.final_dpt  = state->cpt;
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
    uint16_t ahold;
    uint16_t bhold;
    uint16_t cval;
    uint16_t dval;
    bool write_pixel;

    (void)irq;

    if (b == NULL || b->cmd.mode != BLITTER_MODE_LINE) return;

    cmd   = &b->cmd;
    state = &b->line_state;
    line_state_init(b);

    if (state->step_index >= cmd->height_lines) return;

    cval = cmd->use_c ? line_chip_read16(&mem, state->cpt) : cmd->cdat;
    state->last_cdat = cval;

    ahold = (uint16_t)((cmd->adat & cmd->afwm) >> state->x_shift);
    bhold = (state->pattern & 1u) ? 0xFFFFu : 0x0000u;
    dval = line_blitter_logic(cmd->minterm, ahold, bhold, cval);
    write_pixel = !cmd->line_single_dot || state->one_dot_count == 0u;

    if (cmd->use_c && write_pixel)
        line_chip_write16(&mem, state->cpt, dval);

    if (dval != 0) state->zero = false;
    state->last_ddat = dval;
    state->step_index++;
    state->one_dot_count++;

    line_advance(state, cmd);
    state->pattern = (uint16_t)((state->pattern << 1) |
                                (state->pattern >> 15));

    line_publish_partial_result(b);
}

bool blitter_line_done(const BlitterState *b)
{
    if (b == NULL || b->cmd.mode != BLITTER_MODE_LINE) return true;
    return b->line_state.initialized &&
           b->line_state.step_index >= b->cmd.height_lines;
}
