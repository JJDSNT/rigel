#include "dualpf.h"

dualpf_result_t dualpf_decode(uint8_t plane_bits)
{
    dualpf_result_t r;
    /* Odd planes: bits 0,2,4 → PF1 (3-bit index in colors 0-7) */
    r.pf1_index = ((plane_bits >> 0) & 1u) |
                  ((plane_bits >> 1) & 2u) |
                  ((plane_bits >> 2) & 4u);
    /* Even planes: bits 1,3,5 → PF2 (3-bit index in colors 8-15) */
    r.pf2_index = ((plane_bits >> 1) & 1u) |
                  ((plane_bits >> 2) & 2u) |
                  ((plane_bits >> 3) & 4u);
    /* Remap to palette slots */
    if (r.pf1_index) r.pf1_index += 0;   /* PF1 uses colors 0-7  */
    if (r.pf2_index) r.pf2_index += 8;   /* PF2 uses colors 8-15 */
    return r;
}

uint8_t dualpf_priority_resolve(const dualpf_result_t *pf, uint16_t bplcon2)
{
    /* TODO(render): implement proper PF1/PF2 priority via BPLCON2 PF1P/PF2P */
    (void)bplcon2;
    if (pf->pf2_index) return pf->pf2_index;
    if (pf->pf1_index) return pf->pf1_index;
    return 0;
}
