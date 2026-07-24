/*
 * ISSUE-0071 — blitter cycle undercharge.
 *
 * blitter_estimate_cycles historically charged one bus cycle per output word,
 * ignoring the number of active DMA channels. Real Agnus spends one cycle per
 * active channel (USEA/USEB/USEC/USED) per word, so a multi-channel blit ran
 * 2-4x too fast. This test pins:
 *   1. the channel-count kernel (blitter_active_channel_count),
 *   2. the opt-in gate (default OFF = legacy words; ON = words * channels),
 *   3. that line mode is unaffected.
 */

#include "agnus/blitter/blitter.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

static void check_u32(const char *what, uint32_t got, uint32_t want)
{
    if (got != want) {
        printf("FAIL: %s => got %u, want %u\n", what, (unsigned)got, (unsigned)want);
        failures++;
    }
}

/* Build a minimal COPY command with the given channel mask and size. */
static BlitCommand make_copy(bool a, bool b, bool c, bool d,
                             uint16_t w, uint16_t h)
{
    BlitCommand cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.mode = BLITTER_MODE_COPY;
    cmd.use_a = a;
    cmd.use_b = b;
    cmd.use_c = c;
    cmd.use_d = d;
    cmd.width_words = w;
    cmd.height_lines = h;
    return cmd;
}

int main(void)
{
    /* --- 1. channel-count kernel ------------------------------------------ */
    {
        BlitCommand ad   = make_copy(true, false, false, true, 1, 1);   /* A->D */
        BlitCommand abcd = make_copy(true, true, true, true, 1, 1);     /* cookie-cut */
        BlitCommand acd  = make_copy(true, false, true, true, 1, 1);
        BlitCommand none = make_copy(false, false, false, false, 1, 1);

        check_u32("channels A->D", blitter_active_channel_count(&ad), 2);
        check_u32("channels A+B+C+D", blitter_active_channel_count(&abcd), 4);
        check_u32("channels A+C+D", blitter_active_channel_count(&acd), 3);
        check_u32("channels none clamps to 1", blitter_active_channel_count(&none), 1);
    }

    /* --- 1b. per-word cost: +1 idle cycle for D-write without C-read ------- */
    {
        BlitCommand dclr = make_copy(false, false, false, true, 1, 1);  /* D-only clear */
        BlitCommand adf  = make_copy(true, false, false, true, 1, 1);   /* A->D copy/fill */
        BlitCommand abcd = make_copy(true, true, true, true, 1, 1);     /* cookie-cut */
        BlitCommand acd  = make_copy(true, false, true, true, 1, 1);    /* C read present */

        check_u32("word cost D-only = 2 (row 23)", blitter_word_cost(&dclr), 2);
        check_u32("word cost A->D = 3 (row 24)", blitter_word_cost(&adf), 3);
        check_u32("word cost A+B+C+D = 4", blitter_word_cost(&abcd), 4);
        check_u32("word cost A+C+D = 3 (C pairs D)", blitter_word_cost(&acd), 3);
    }

    /* --- 2. gate: default OFF = legacy words ------------------------------- */
    {
        BlitCommand abcd = make_copy(true, true, true, true, 20, 10); /* 200 words */

        blitter_set_channel_cost_enabled(0);
        check_u32("gate OFF => words only", blitter_estimate_cycles(&abcd), 200);

        blitter_set_channel_cost_enabled(1);
        check_u32("gate ON cookie-cut => words * 4",
                  blitter_estimate_cycles(&abcd), 800);

        BlitCommand ad = make_copy(true, false, false, true, 20, 10);
        check_u32("gate ON A->D => words * 3 (2 channels + D idle)",
                  blitter_estimate_cycles(&ad), 600);
    }

    /* --- 3. line mode is unaffected by the gate --------------------------- */
    {
        BlitCommand line;
        memset(&line, 0, sizeof(line));
        line.mode = BLITTER_MODE_LINE;
        line.height_lines = 37;

        blitter_set_channel_cost_enabled(1);
        check_u32("line mode ignores channel cost",
                  blitter_estimate_cycles(&line), 37);
    }

    /* restore default so nothing leaks into other state */
    blitter_set_channel_cost_enabled(-1);

    if (failures == 0) {
        printf("test_blitter_channel_cost: OK\n");
        return 0;
    }
    printf("test_blitter_channel_cost: %d failure(s)\n", failures);
    return 1;
}
