#include "sprites.h"

/* Sprite DMA delivery — Denise side.
 *
 * Agnus fetches the control and pixel words for each sprite during the
 * horizontal blank. This file contains the functions called by the
 * Agnus→Denise delivery path.
 *
 * Agnus owns: SPRxPT pointers, DMA timing, fetch scheduling.
 * Denise owns: latching, interpretation, rendering. */

/* Delivery is wired: slot_scheduler.c dispatch_slot() calls
 * denise_sprite_receive_ctrl / denise_sprite_receive_data from sprites.c
 * for each AGNUS_SLOT_SPRITE_N slot. No additional callback needed. */
