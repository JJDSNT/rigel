#ifndef RIGEL_HARNESS_FASTRAM_H
#define RIGEL_HARNESS_FASTRAM_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Zorro II Fast RAM — a memory board the guest finds through autoconfig and
 * adds to its free list (the MEMLIST flag in er_Type is what does that).
 *
 * Unlike Chip RAM this is not reachable by the chipset, so Agnus cannot fetch
 * bitplanes or samples from it. That is the real hardware distinction and the
 * reason a program that runs from Fast RAM leaves more bus time for DMA.
 *
 * Valid sizes are the Zorro II codes: 1, 2, 4 or 8 MB.
 */

typedef struct fastram_board fastram_board_t;

fastram_board_t *fastram_create(uint32_t megabytes);
void             fastram_destroy(fastram_board_t *b);

/* Autoconfig participation. The harness routes the config window to whichever
 * board is still pending, one at a time, as the bus protocol requires. */
bool    fastram_pending(const fastram_board_t *b);
uint8_t fastram_ac_read(const fastram_board_t *b, uint32_t off);
void    fastram_ac_write(fastram_board_t *b, uint32_t off, uint8_t value);

bool     fastram_owns(const fastram_board_t *b, uint32_t addr);
uint32_t fastram_read(const fastram_board_t *b, uint32_t addr, unsigned size);
void     fastram_write(fastram_board_t *b, uint32_t addr, uint32_t value, unsigned size);

/* Configure the board directly, for a machine with no Kickstart to run
 * autoconfig — loading a bare executable, for instance. */
void fastram_force_configure(fastram_board_t *b, uint32_t base);

uint32_t fastram_base(const fastram_board_t *b);
uint32_t fastram_size(const fastram_board_t *b);

#endif
