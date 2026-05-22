#include "agnus/copper/copper.h"
#include "agnus/beam.h"
#include "agnus/dma.h"
#include "domains/copper/copper_domain.h"

int main(void)
{
    copper_state_t copper = { 123 };
    beam_state_t beam = { 0 };
    dma_state_t dma = { 0 };

    copper_reset(&copper);
    if (copper.program_counter != 0) {
        return 1;
    }

    copper_set_pointer_hi(&copper.cop1lc, 0x0001);
    copper_set_pointer_lo(&copper.cop1lc, 0x2340);
    rigel_copper_domain_jump1(&copper);
    if (copper.program_counter != 0x00012340u) {
        return 1;
    }

    beam_reset(&beam);
    dma_set_dmacon(&dma, 0x0280u);
    /* WAIT at vpos=0, hpos=10, full masks */
    rigel_copper_domain_set_wait(&copper, 0, 10, 0xFF, 0xFE);
    rigel_copper_domain_step(&copper, &beam, &dma);
    if (copper.triggered) {
        return 1;
    }

    beam_step(&beam, 10);
    rigel_copper_domain_step(&copper, &beam, &dma);
    if (!copper.triggered || copper.waiting || copper.program_counter != 0x00012344u) {
        return 1;
    }

    /* WAIT with partial VP mask: vpmask=0x7F means only lower 7 bits of vpos compared.
     * Wait target vpos=0x80 with mask=0x7F → effective target = 0x80 & 0x7F = 0x00.
     * beam vpos=1 → (1 & 0x7F)=1 >= 0 → satisfied immediately. */
    copper_reset(&copper);
    dma_set_dmacon(&dma, 0x0280u);
    rigel_copper_domain_set_wait(&copper, 0x80, 0, 0x7F, 0xFE);
    beam_reset(&beam);
    beam_step(&beam, 227);   /* advance to vpos=1 */
    rigel_copper_domain_step(&copper, &beam, &dma);
    if (!copper.triggered || copper.waiting) {
        return 1;
    }

    /* copper_beam_cmp: SKIP-style test — beam past target returns true */
    if (!copper_beam_cmp(10, 0, 5, 0, 0xFF, 0xFE)) {
        return 1;
    }
    /* beam before target returns false */
    if (copper_beam_cmp(4, 0, 5, 0, 0xFF, 0xFE)) {
        return 1;
    }

    return 0;
}
