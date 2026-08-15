#ifndef RIGEL_HARNESS_ZORRO_H
#define RIGEL_HARNESS_ZORRO_H

#include <stdint.h>

/*
 * Zorro II autoconfig, the parts every board shares.
 *
 * The config window at 0xE80000 is nibble-wide: each of the 16 autoconfig
 * bytes occupies four byte addresses, with the nibble in D7-D4.
 *
 * Only one board answers the window at a time. Boards are enumerated in chain
 * order: the first one that has neither been given a base nor been told to shut
 * up owns 0xE80000, and the next appears only once that one is done. The
 * harness arbitrates this; boards just report whether they are still pending.
 */

#define ZORRO_AC_BASE      0xE80000u
#define ZORRO_AC_WINDOW    0x010000u
#define ZORRO_AC_ROM_BYTES 16u
#define ZORRO_AC_DATA_SIZE 64u

/* Config-window write offsets. */
#define ZORRO_AC_OFF_BASE_HI 0x48u   /* ec_BaseAddress: the board's base >> 16 */
#define ZORRO_AC_OFF_SHUTUP  0x4Cu   /* ec_Shutup */

/* Encode 16 raw autoconfig bytes into the 64-byte nibble-wide image.
 *
 * The guest reads each field complemented and inverts it back, except er_Type
 * where the two inversions cancel — so everything but byte 0 is stored
 * complemented here. */
void zorro_autoconfig_build(uint8_t *out, const uint8_t *bytes);

#endif
