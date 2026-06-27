#include "blitter.h"

#include <string.h>

static inline uint16_t chip_read16(
    const BlitterMemory *mem,
    uint32_t addr
) {
    addr &= BLITTER_CHIP_ADDR_MASK;

    if (mem->read16 == NULL) {
        return 0;
    }

    return mem->read16(mem->opaque, addr);
}

static inline void chip_write16(
    const BlitterMemory *mem,
    uint32_t addr,
    uint16_t value
) {
    addr &= BLITTER_CHIP_ADDR_MASK;

    if (mem->write16 == NULL) {
        return;
    }

    mem->write16(mem->opaque, addr, value);
}

static uint16_t blitter_logic(
    uint8_t minterm,
    uint16_t A,
    uint16_t B,
    uint16_t C
) {
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

static uint16_t shift_ascending(
    uint16_t previous,
    uint16_t current,
    uint8_t shift
) {
    if (shift == 0) {
        return current;
    }

    return (uint16_t)(
        (previous << (16 - shift)) |
        (current >> shift)
    );
}

static uint16_t shift_descending(
    uint16_t previous,
    uint16_t current,
    uint8_t shift
) {
    if (shift == 0) {
        return current;
    }

    return (uint16_t)(
        (previous >> (16 - shift)) |
        (current << shift)
    );
}

static uint16_t blitter_fill_word(
    uint16_t d,
    uint8_t *carry,
    bool inclusive,
    bool descending
) {
    uint16_t result = 0;
    uint8_t c = *carry;

    if (!descending) {
        for (int i = 15; i >= 0; --i) {
            uint8_t bit = (uint8_t)((d >> i) & 1u);

            if (inclusive) {
                if (bit)
                    c ^= 1u;
                if (c)
                    result |= (uint16_t)(1u << i);
            } else {
                if ((c ^ bit) != 0)
                    result |= (uint16_t)(1u << i);
                if (bit)
                    c ^= 1u;
            }
        }
    } else {
        for (int i = 0; i <= 15; ++i) {
            uint8_t bit = (uint8_t)((d >> i) & 1u);

            if (inclusive) {
                if (bit)
                    c ^= 1u;
                if (c)
                    result |= (uint16_t)(1u << i);
            } else {
                if ((c ^ bit) != 0)
                    result |= (uint16_t)(1u << i);
                if (bit)
                    c ^= 1u;
            }
        }
    }

    *carry = c;
    return result;
}

static uint16_t blitter_apply_shift(
    bool descending,
    uint16_t previous,
    uint16_t current,
    uint8_t shift
) {
    if (descending) {
        return shift_descending(previous, current, shift);
    }

    return shift_ascending(previous, current, shift);
}

static uint16_t blitter_apply_a_masks(
    const BlitCommand *cmd,
    uint16_t value,
    uint32_t word_index
) {
    if (word_index == 0) {
        value &= cmd->afwm;
    }

    if (word_index + 1u == (uint32_t)cmd->width_words) {
        value &= cmd->alwm;
    }

    return value;
}

static uint16_t blitter_apply_fill(
    const BlitCommand *cmd,
    uint16_t value,
    uint8_t *fill_carry
) {
    switch (cmd->fill_mode) {
        case BLITTER_FILL_INCLUSIVE:
            return blitter_fill_word(value, fill_carry, true, cmd->descending);

        case BLITTER_FILL_EXCLUSIVE:
            return blitter_fill_word(value, fill_carry, false, cmd->descending);

        case BLITTER_FILL_NONE:
        default:
            return value;
    }
}

static uint16_t blitter_fetch_word(
    const BlitterMemory *mem,
    bool enabled,
    uint32_t *ptr,
    int increment,
    uint16_t latched,
    uint16_t *last_observed
) {
    uint16_t value = latched;

    if (!enabled) {
        return value;
    }

    value = chip_read16(mem, *ptr);
    *ptr += increment;
    *last_observed = value;

    return value;
}

typedef struct BlitterCopyState {
    uint32_t apt;
    uint32_t bpt;
    uint32_t cpt;
    uint32_t dpt;
    uint16_t previous_a;
    uint16_t previous_b;
    uint16_t last_adat;
    uint16_t last_bdat;
    uint16_t last_cdat;
    uint16_t last_ddat;
    uint8_t fill_carry;
    bool zero;
} BlitterCopyState;

static void blitter_copy_state_init(
    BlitterCopyState *state,
    const BlitCommand *cmd
) {
    memset(state, 0, sizeof(*state));

    state->apt = cmd->apt;
    state->bpt = cmd->bpt;
    state->cpt = cmd->cpt;
    state->dpt = cmd->dpt;
    state->last_adat = cmd->adat;
    state->last_bdat = cmd->bdat;
    state->last_cdat = cmd->cdat;
    state->fill_carry = cmd->fill_carry_in ? 1u : 0u;
    state->zero = true;
}

static void blitter_copy_advance_line(
    BlitterCopyState *state,
    const BlitCommand *cmd
) {
    if (cmd->descending) {
        state->apt -= cmd->amod;
        state->bpt -= cmd->bmod;
        state->cpt -= cmd->cmod;
        state->dpt -= cmd->dmod;
        return;
    }

    state->apt += cmd->amod;
    state->bpt += cmd->bmod;
    state->cpt += cmd->cmod;
    state->dpt += cmd->dmod;
}

static void blitter_copy_publish_result(
    const BlitterCopyState *state,
    BlitterResult *result
) {
    result->final_apt = state->apt;
    result->final_bpt = state->bpt;
    result->final_cpt = state->cpt;
    result->final_dpt = state->dpt;

    result->final_adat = state->last_adat;
    result->final_bdat = state->last_bdat;
    result->final_cdat = state->last_cdat;
    result->final_ddat = state->last_ddat;
    result->zero = state->zero;
}

static bool execute_line_mode(
    BlitterState *b,
    BlitterMemory mem
) {
    const BlitCommand *cmd = &b->cmd;
    BlitterResult *result = &b->result;
    int16_t error = (int16_t)(cmd->apt & 0xFFFFu);
    uint8_t x_shift = cmd->line_start_bit & 0x0Fu;
    uint8_t one_dot_count = 0;
    int pattern_shift = (int)cmd->bshift;
    uint16_t pattern = cmd->bdat;
    uint32_t cpt = cmd->cpt & BLITTER_CHIP_ADDR_MASK;
    bool zero = true;
    bool sign = cmd->line_initial_sign;
    uint16_t last_cdat = cmd->cdat;

    if (cmd->height_lines == 0) {
        result->final_apt = cmd->apt;
        result->final_bpt = cmd->bpt;
        result->final_cpt = cmd->cpt;
        result->final_dpt = cmd->dpt;
        result->final_adat = cmd->adat;
        result->final_bdat = cmd->bdat;
        result->final_cdat = cmd->cdat;
        result->final_ddat = 0;
        result->zero = true;
        return true;
    }

    if (pattern_shift != 0) {
        pattern = (uint16_t)((pattern >> pattern_shift) |
                             (pattern << (16 - pattern_shift)));
    }

    for (uint16_t i = 0; i < cmd->height_lines; ++i) {
        uint16_t ahold;
        uint16_t bhold;
        uint16_t cval;
        uint16_t dval;
        bool write_pixel;

        cval = cmd->use_c ? chip_read16(&mem, cpt) : cmd->cdat;
        last_cdat = cval;

        ahold = (uint16_t)((cmd->adat & cmd->afwm) >> x_shift);
        bhold = (pattern & 1u) ? 0xFFFFu : 0x0000u;
        dval = blitter_logic(cmd->minterm, ahold, bhold, cval);
        write_pixel = !cmd->line_single_dot || one_dot_count == 0u;

        if (cmd->use_c && write_pixel)
            chip_write16(&mem, cpt, dval);

        if (dval != 0) {
            zero = false;
        }

        result->final_ddat = dval;
        one_dot_count++;

        if (cmd->use_a) {
            if (sign)
                error = (int16_t)(error + cmd->bmod);
            else
                error = (int16_t)(error + cmd->amod);
        }

        if (!sign) {
            if (cmd->line_octant & 0x04u) {
                if (cmd->line_octant & 0x02u)
                    cpt = (uint32_t)((int32_t)cpt - (int32_t)cmd->cmod) &
                          BLITTER_CHIP_ADDR_MASK;
                else
                    cpt = (uint32_t)((int32_t)cpt + (int32_t)cmd->cmod) &
                          BLITTER_CHIP_ADDR_MASK;
                one_dot_count = 0;
            } else if (cmd->line_octant & 0x02u) {
                if (x_shift == 0u) {
                    x_shift = 15u;
                    cpt = (cpt - 2u) & BLITTER_CHIP_ADDR_MASK;
                } else {
                    x_shift--;
                }
            } else {
                x_shift++;
                if (x_shift == 16u) {
                    x_shift = 0;
                    cpt = (cpt + 2u) & BLITTER_CHIP_ADDR_MASK;
                }
            }
        }

        if (cmd->line_octant & 0x04u) {
            if (cmd->line_octant & 0x01u) {
                if (x_shift == 0u) {
                    x_shift = 15u;
                    cpt = (cpt - 2u) & BLITTER_CHIP_ADDR_MASK;
                } else {
                    x_shift--;
                }
            } else {
                x_shift++;
                if (x_shift == 16u) {
                    x_shift = 0;
                    cpt = (cpt + 2u) & BLITTER_CHIP_ADDR_MASK;
                }
            }
        } else if (cmd->line_octant & 0x01u) {
            cpt = (uint32_t)((int32_t)cpt - (int32_t)cmd->cmod) &
                  BLITTER_CHIP_ADDR_MASK;
            one_dot_count = 0;
        } else {
            cpt = (uint32_t)((int32_t)cpt + (int32_t)cmd->cmod) &
                  BLITTER_CHIP_ADDR_MASK;
            one_dot_count = 0;
        }

        sign = error < 0;
        pattern = (uint16_t)((pattern << 1) | (pattern >> 15));
    }

    result->final_apt = (cmd->apt & 0xFFFF0000u) | (uint16_t)error;
    result->final_bpt = cmd->bpt;
    result->final_cpt = cpt;
    result->final_dpt = cpt;

    result->final_adat = cmd->adat;
    result->final_bdat = cmd->bdat;
    result->final_cdat = last_cdat;
    result->zero = zero;

    return true;
}

static bool execute_copy_mode(
    BlitterState *b,
    BlitterMemory mem
) {
    const BlitCommand *cmd = &b->cmd;
    BlitterResult *result = &b->result;
    BlitterCopyState state;
    int increment = cmd->descending ? -2 : 2;
    uint32_t pending_dpt = 0u;
    uint16_t pending_ddat = 0u;
    bool pending_d = false;

    /*
     * TODO: Revisit copy-path semantics as a first-class model:
     * define the canonical boundary between raw fetch, hold/continuity,
     * shift alignment, A masks, latched channel values, fill, and final
     * observable register publication.
     */
    blitter_copy_state_init(&state, cmd);

    for (uint32_t y = 0; y < cmd->height_lines; ++y) {
        state.previous_a = 0;
        state.previous_b = 0;
        state.fill_carry = cmd->fill_carry_in ? 1u : 0u;

        for (uint32_t x = 0; x < cmd->width_words; ++x) {
            uint16_t raw_adat =
                blitter_fetch_word(
                    &mem,
                    cmd->use_a,
                    &state.apt,
                    increment,
                    cmd->adat,
                    &state.last_adat
                );
            uint16_t adat = blitter_apply_a_masks(cmd, raw_adat, x);

            uint16_t bdat =
                blitter_fetch_word(
                    &mem,
                    cmd->use_b,
                    &state.bpt,
                    increment,
                    cmd->bdat,
                    &state.last_bdat
                );

            uint16_t cdat =
                blitter_fetch_word(
                    &mem,
                    cmd->use_c,
                    &state.cpt,
                    increment,
                    cmd->cdat,
                    &state.last_cdat
                );

            uint16_t A =
                blitter_apply_shift(
                    cmd->descending,
                    state.previous_a,
                    adat,
                    cmd->ashift
                );

            uint16_t B =
                blitter_apply_shift(
                    cmd->descending,
                    state.previous_b,
                    bdat,
                    cmd->bshift
                );

            uint16_t C = cdat;
            uint16_t D;

            state.previous_a = adat;
            state.previous_b = bdat;

            if (pending_d) {
                chip_write16(&mem, pending_dpt, pending_ddat);
                pending_d = false;
            }

            D = blitter_logic(cmd->minterm, A, B, C);
            D = blitter_apply_fill(cmd, D, &state.fill_carry);

            if (D != 0) {
                state.zero = false;
            }

            if (cmd->use_d) {
                pending_dpt = state.dpt;
                pending_ddat = D;
                pending_d = true;
                state.dpt += increment;
            }

            state.last_ddat = D;
        }

        blitter_copy_advance_line(&state, cmd);
    }

    if (pending_d) {
        chip_write16(&mem, pending_dpt, pending_ddat);
    }

    blitter_copy_publish_result(&state, result);
    return true;
}

bool blitter_execute_reference(
    BlitterState *b,
    BlitterMemory mem
) {
    if (!b->command_valid) {
        return false;
    }

    memset(&b->result, 0, sizeof(b->result));

    switch (b->cmd.mode) {

        case BLITTER_MODE_COPY:
            return execute_copy_mode(b, mem);

        case BLITTER_MODE_LINE:
            return execute_line_mode(b, mem);

        default:
            return false;
    }
}
