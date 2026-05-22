#include "sprites.h"

/* Sprite DMA delivery — Denise side.
 *
 * Agnus fetches the control and pixel words for each sprite during the
 * horizontal blank. This file contains the functions called by the
 * Agnus→Denise delivery path.
 *
 * Agnus owns: SPRxPT pointers, DMA timing, fetch scheduling.
 * Denise owns: latching, interpretation, rendering. */

/* TODO(sprites): implement Agnus→Denise delivery callback.
 * When the slot scheduler runs a sprite DMA slot, Agnus calls back into
 * Denise with the fetched words. Wire this up via an interface in
 * rigel_context or a direct call from agnus_step(). */
