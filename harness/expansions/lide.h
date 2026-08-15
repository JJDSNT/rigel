#ifndef RIGEL_HARNESS_LIDE_H
#define RIGEL_HARNESS_LIDE_H

#include <stdbool.h>
#include <stdint.h>

/*
 * LIDE — a Zorro II board carrying LIV2's lide.device, which is how the
 * harness offers a hard disk (HDF) and a CD-ROM (ISO) to the guest.
 *
 * The board answers in two places:
 *
 *   0xE80000  the Zorro II autoconfig window, until the guest assigns a base
 *   <base>    a 128 KB window holding, by offset:
 *               0x0000  'LIV2' header
 *               0x0004  nibble-encoded boot loader
 *               0x1000  ATA/IDE registers (stride 0x200)
 *               0x2000  the lide.device binary, byte-wide
 *              0x10000  the ODFileSystem binary, byte-wide (ISO support)
 *
 * The guest walks autoconfig, latches a base, loads the driver out of the ROM
 * window, and then talks ATA. Everything below that is the ATA and ATAPI
 * layers carried over from Bellatrix unchanged.
 */

typedef struct lide_board lide_board_t;

/* Zorro II autoconfig window. Fixed by the hardware. */
#define LIDE_AUTOCONFIG_BASE 0xE80000u
#define LIDE_AUTOCONFIG_SIZE 0x010000u
#define LIDE_WINDOW_SIZE     0x020000u   /* 128 KB */

/* Create the board with the ROM image at `rom_path`. Returns NULL if the ROM
 * cannot be read; the harness then leaves the board out entirely rather than
 * presenting one the guest cannot drive. */
lide_board_t *lide_create(const char *rom_path);
void          lide_destroy(lide_board_t *b);

/* Attach media. Both take a host path and keep the file open for the run:
 * HDF writes are meant to persist, and an ISO can be larger than RAM. */
bool lide_attach_hdf(lide_board_t *b, const char *path);
bool lide_attach_iso(lide_board_t *b, const char *path);

/* Load the ODFileSystem binary that the guest needs to mount an ISO. Without
 * it a CD is visible but has no filesystem. */
bool lide_load_odfs(lide_board_t *b, const char *path);

/* True once the guest has assigned a base address to this board. */
bool     lide_configured(const lide_board_t *b);
uint32_t lide_base(const lide_board_t *b);

/* Autoconfig participation; the harness arbitrates which board answers. */
bool    lide_pending(const lide_board_t *b);
uint8_t lide_ac_read(const lide_board_t *b, uint32_t off);
void    lide_ac_write(lide_board_t *b, uint32_t off, uint8_t value);

/* True when `addr` belongs to the board — either the autoconfig window while
 * unconfigured, or the assigned window afterwards. */
bool lide_owns(const lide_board_t *b, uint32_t addr);

/* Bus access. `size` is 1, 2 or 4 bytes. Reads of unmapped offsets give the
 * 0xFF..  an unpulled bus would. */
uint32_t lide_read(lide_board_t *b, uint32_t addr, unsigned size);
void     lide_write(lide_board_t *b, uint32_t addr, uint32_t value, unsigned size);

void lide_reset(lide_board_t *b);

#endif
