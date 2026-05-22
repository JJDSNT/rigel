#ifndef RIGEL_DENISE_DUALPF_H
#define RIGEL_DENISE_DUALPF_H

#include <stdint.h>
#include <stdbool.h>

/* Dual playfield mode — BPLCON0 DUALFW=1.
 *
 * The 6 planes are split: odd planes (1, 3, 5) → PF1, even planes (2, 4, 6) → PF2.
 * Each playfield independently looks up a 3-bit color in its own sub-palette:
 *   PF1 → colors  0–7   (+ pf1 offset controlled by BPLCON2 PF1P)
 *   PF2 → colors  8–15  (+ pf2 offset controlled by BPLCON2 PF2P... TODO verify)
 *
 * Priority between the two playfields is set by BPLCON2. */

typedef struct dualpf_result {
    uint8_t pf1_index;   /* color index for playfield 1 (0 = transparent) */
    uint8_t pf2_index;   /* color index for playfield 2 (0 = transparent) */
} dualpf_result_t;

/* Decode a 6-bit plane_bits value into separate PF1 and PF2 color indices. */
dualpf_result_t dualpf_decode(uint8_t plane_bits);

/* Given PF1 and PF2 color indices and BPLCON2, return the winning pixel index.
 * Returns 0 if both playfields are transparent. */
uint8_t dualpf_priority_resolve(const dualpf_result_t *pf, uint16_t bplcon2);

#endif
