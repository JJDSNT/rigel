/* fseeko/off_t are POSIX, and Rigel builds as strict C11 (-std=c11, no GNU
 * extensions). Ask for them explicitly rather than falling back to fseek(),
 * whose long offset is only 64-bit on some of the platforms we care about. */
#define _POSIX_C_SOURCE 200809L

#include "lide.h"

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ata_ide.h"
#include "atapi_cdrom.h"
#include "shim.h"
#include "zorro.h"

/* -------------------------------------------------------------------------
 * Board layout — the RIPPLE geometry lide.device expects
 * ------------------------------------------------------------------------- */

enum {
    ROM_HDR_END      = 0x0004,   /* raw 'LIV2' header                      */
    ROM_NIBBLE_END   = 0x1000,   /* nibble-encoded boot loader ends here   */
    ROM_DEVICE_BOARD = 0x2000,   /* byte-wide device binary, board offset  */
    ROM_DEVICE_FILE  = 0x1000,   /* the same data's offset inside the ROM  */
    ODFS_BANK_BASE   = 0x10000,  /* second bank: ODFileSystem, byte-wide   */
    ODFS_BANK_END    = 0x20000
};

/* Autoconfig identity. lide.device matches on these. */
enum {
    RIPPLE_MFR_HI = 0x14,        /* manufacturer 0x144A */
    RIPPLE_MFR_LO = 0x4A,
    RIPPLE_PROD   = 0x07
};

/* er_Type bits. The shared autoconfig constants live in zorro.h. */
enum {
    AC_TYPE_Z2        = 0xC0,
    AC_TYPE_DIAGVALID = 0x10,
    AC_SIZE_128KB     = 0x02
};

#define AC_ROM_BYTES   ZORRO_AC_ROM_BYTES
#define AC_DATA_SIZE   ZORRO_AC_DATA_SIZE
#define AC_OFF_BASE_HI ZORRO_AC_OFF_BASE_HI
#define AC_OFF_SHUTUP  ZORRO_AC_OFF_SHUTUP

/* -------------------------------------------------------------------------
 * Media backends
 *
 * Both keep the host file open rather than reading it into memory: an ISO is
 * routinely larger than the machine's RAM, and HDF writes have to persist.
 * ------------------------------------------------------------------------- */

typedef struct media_file {
    FILE    *fp;
    uint32_t sectors;      /* in that medium's sector size */
    bool     present;
} media_file_t;

static bool media_open(media_file_t *m, const char *path, uint32_t sector_size,
                       const char *mode)
{
    long size;

    m->fp = fopen(path, mode);
    if (m->fp == NULL) return false;

    if (fseek(m->fp, 0, SEEK_END) != 0) { fclose(m->fp); m->fp = NULL; return false; }
    size = ftell(m->fp);
    rewind(m->fp);

    if (size <= 0) { fclose(m->fp); m->fp = NULL; return false; }

    m->sectors = (uint32_t)((size_t)size / sector_size);
    m->present = true;
    return true;
}

static int media_read(media_file_t *m, uint32_t sector_size,
                      uint32_t lba, uint32_t count, uint8_t *buf)
{
    size_t want;

    if (!m->present || m->fp == NULL) return -1;
    if (fseeko(m->fp, (off_t)lba * sector_size, SEEK_SET) != 0) return -1;

    want = (size_t)count * sector_size;
    if (fread(buf, 1, want, m->fp) != want) return -1;
    return 0;
}

/* --- ATAPI (CD-ROM, 2048-byte sectors) --- */

static int iso_present(void *ctx)
{
    return ((media_file_t *)ctx)->present ? 1 : 0;
}

static int iso_read_sectors(void *ctx, uint32_t lba, uint32_t count, uint8_t *buf)
{
    return media_read((media_file_t *)ctx, ATAPI_SECTOR_SIZE, lba, count, buf);
}

static uint32_t iso_sector_count(void *ctx)
{
    return ((media_file_t *)ctx)->sectors;
}

static const AtapiMediaOps g_iso_ops = {
    .present      = iso_present,
    .read_sectors = iso_read_sectors,
    .sector_count = iso_sector_count,
};

/* --- ATA disk (HDF, 512-byte sectors) --- */

static int hdf_read(void *ctx, uint32_t lba, uint32_t count, uint8_t *buf)
{
    return media_read((media_file_t *)ctx, 512u, lba, count, buf);
}

static int hdf_write(void *ctx, uint32_t lba, uint32_t count, const uint8_t *buf)
{
    media_file_t *m = (media_file_t *)ctx;
    size_t want = (size_t)count * 512u;

    if (!m->present || m->fp == NULL) return -1;
    if (fseeko(m->fp, (off_t)lba * 512, SEEK_SET) != 0) return -1;
    if (fwrite(buf, 1, want, m->fp) != want) return -1;
    fflush(m->fp);
    return 0;
}

/* -------------------------------------------------------------------------
 * Board state
 * ------------------------------------------------------------------------- */

struct lide_board {
    AtaIdeChannel   ide;
    AtapiCdromState atapi;

    media_file_t    iso;
    media_file_t    hdf;

    uint8_t  config[AC_DATA_SIZE];
    bool     configured;
    bool     shutup;
    uint32_t base;

    uint8_t *rom;
    size_t   rom_size;

    uint8_t *odfs;
    size_t   odfs_size;

    bool logged_hdr;
    bool logged_dev;
    bool logged_odfs;
    uint64_t odfs_reads;

    /* Access counters: the difference between "the driver gave up" and "the
     * driver is still polling a status bit that never sets" is invisible
     * otherwise. */
    uint64_t ata_reads;
    uint64_t ata_writes;
    uint64_t rom_reads;
};

lide_board_t *lide_create(const char *rom_path)
{
    static const uint8_t ac_bytes[AC_ROM_BYTES] = {
        AC_TYPE_Z2 | AC_TYPE_DIAGVALID | AC_SIZE_128KB,  /* er_Type          */
        RIPPLE_PROD,                                     /* er_Product       */
        0x00, 0x00,                                      /* er_Flags, rsvd   */
        RIPPLE_MFR_HI, RIPPLE_MFR_LO,                    /* manufacturer     */
        0x00, 0x00, 0x00, 0x01,                          /* serial           */
        0x00, 0x04,                                      /* InitDiagVec      */
        0x00, 0x00, 0x00, 0x00                           /* reserved         */
    };

    lide_board_t *b;
    FILE *fp;
    long size;

    b = (lide_board_t *)calloc(1, sizeof(*b));
    if (b == NULL) return NULL;

    fp = fopen(rom_path, "rb");
    if (fp == NULL) {
        free(b);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    if (size <= 0) { fclose(fp); free(b); return NULL; }

    b->rom = (uint8_t *)malloc((size_t)size);
    if (b->rom == NULL || fread(b->rom, 1, (size_t)size, fp) != (size_t)size) {
        fclose(fp);
        free(b->rom);
        free(b);
        return NULL;
    }
    fclose(fp);
    b->rom_size = (size_t)size;

    zorro_autoconfig_build(b->config, ac_bytes);

    if (ata_ide_channel_alloc(&b->ide) != 0) {
        lide_destroy(b);
        return NULL;
    }
    ata_ide_channel_init(&b->ide);

    atapi_cdrom_init(&b->atapi, &g_iso_ops, &b->iso);
    b->ide.atapi_ctx  = &b->atapi;
    b->ide.atapi_exec = atapi_cdrom_exec;

    return b;
}

void lide_destroy(lide_board_t *b)
{
    if (b == NULL) return;
    kprintf("[LIDE] traffic: %llu ATA reads, %llu ATA writes, %llu ROM reads\n",
            (unsigned long long)b->ata_reads,
            (unsigned long long)b->ata_writes,
            (unsigned long long)b->rom_reads);
    kprintf("[LIDE] ODFS bank reads: %llu\n",
            (unsigned long long)b->odfs_reads);
    ata_ide_channel_free(&b->ide);
    if (b->iso.fp) fclose(b->iso.fp);
    if (b->hdf.fp) fclose(b->hdf.fp);
    free(b->odfs);
    free(b->rom);
    free(b);
}

bool lide_attach_hdf(lide_board_t *b, const char *path)
{
    if (b == NULL) return false;
    if (!media_open(&b->hdf, path, 512u, "r+b")) return false;

    b->ide.disk_ctx     = &b->hdf;
    b->ide.disk_read    = hdf_read;
    b->ide.disk_write   = hdf_write;
    b->ide.disk_sectors = b->hdf.sectors;
    return true;
}

bool lide_attach_iso(lide_board_t *b, const char *path)
{
    if (b == NULL) return false;
    if (!media_open(&b->iso, path, ATAPI_SECTOR_SIZE, "rb")) return false;
    atapi_cdrom_insert(&b->atapi);
    return true;
}

bool lide_load_odfs(lide_board_t *b, const char *path)
{
    FILE *fp;
    long size;

    if (b == NULL) return false;

    fp = fopen(path, "rb");
    if (fp == NULL) return false;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    if (size <= 0) { fclose(fp); return false; }

    b->odfs = (uint8_t *)malloc((size_t)size);
    if (b->odfs == NULL || fread(b->odfs, 1, (size_t)size, fp) != (size_t)size) {
        fclose(fp);
        free(b->odfs);
        b->odfs = NULL;
        return false;
    }
    fclose(fp);
    b->odfs_size = (size_t)size;
    return true;
}

bool     lide_configured(const lide_board_t *b) { return b != NULL && b->configured; }
uint32_t lide_base(const lide_board_t *b)       { return b != NULL ? b->base : 0u; }

void lide_reset(lide_board_t *b)
{
    if (b == NULL) return;
    b->configured = false;
    b->shutup = false;
    b->base = 0;
    ata_ide_channel_reset(&b->ide);
    atapi_cdrom_reset(&b->atapi);
}

bool lide_owns(const lide_board_t *b, uint32_t addr)
{
    if (b == NULL || !b->configured) return false;
    return addr >= b->base && addr < b->base + LIDE_WINDOW_SIZE;
}

bool lide_pending(const lide_board_t *b)
{
    return b != NULL && !b->configured && !b->shutup;
}

uint8_t lide_ac_read(const lide_board_t *b, uint32_t off)
{
    if (b == NULL || off >= AC_DATA_SIZE) return 0xFFu;
    return b->config[off];
}

void lide_ac_write(lide_board_t *b, uint32_t off, uint8_t value)
{
    if (b == NULL) return;

    if (off == AC_OFF_BASE_HI) {
        b->base = (uint32_t)((value & 0xFFu) << 16);
        b->configured = true;
        kprintf("[LIDE] configured at %06x\n", (unsigned)b->base);
    } else if (off == AC_OFF_SHUTUP) {
        b->shutup = true;
        kprintf("[LIDE] shut up by the guest\n");
    }
}

/* -------------------------------------------------------------------------
 * ROM window
 *
 * The boot loader is nibble-encoded and the driver binary byte-wide, both
 * spread across the board window in the layout lide.device's own loader
 * expects. Offsets that fall in neither read as an unpulled bus.
 * ------------------------------------------------------------------------- */

static uint8_t rom_byte(const lide_board_t *b, uint32_t off)
{
    if (off < ROM_HDR_END)
        return (off < b->rom_size) ? b->rom[off] : 0xFFu;

    if (off < ROM_NIBBLE_END) {
        uint32_t rel   = off - ROM_HDR_END;
        uint32_t phase = rel & 3u;
        uint32_t nidx  = rel >> 2;
        uint32_t fidx;

        if      (phase == 0u) fidx = ROM_HDR_END + nidx * 2u;
        else if (phase == 2u) fidx = ROM_HDR_END + nidx * 2u + 1u;
        else                  return 0xFFu;

        return (fidx < b->rom_size) ? b->rom[fidx] : 0xFFu;
    }

    /* 0x1000..0x1FFF is the ATA register window, handled before we get here. */
    if (off < ROM_DEVICE_BOARD)
        return 0xFFu;

    /* Byte-wide device binary: only even board addresses carry data. */
    if (off & 1u) return 0xFFu;
    {
        uint32_t fidx = ROM_DEVICE_FILE + ((off - ROM_DEVICE_BOARD) >> 1);
        return (fidx < b->rom_size) ? b->rom[fidx] : 0xFFu;
    }
}

static uint8_t odfs_byte(const lide_board_t *b, uint32_t off)
{
    uint32_t rel = off - ODFS_BANK_BASE;
    uint32_t idx;

    if (rel & 1u) return 0xFFu;           /* byte-wide: odd addresses unmapped */
    idx = rel >> 1;
    return (idx < b->odfs_size) ? b->odfs[idx] : 0xFFu;
}

/* -------------------------------------------------------------------------
 * Bus
 * ------------------------------------------------------------------------- */

uint32_t lide_read(lide_board_t *b, uint32_t addr, unsigned size)
{
    uint32_t off;
    int reg;

    if (b == NULL || !b->configured) return 0xFFFFFFFFu;

    off = addr - b->base;

    /* ATA registers win over the ROM: they overlap the 0x1000 window. */
    reg = ata_ide_offset_to_reg(off);
    if (reg >= 0) b->ata_reads++;
    if (reg == 0 && size == 4) {
        /* movem.l / move.l on the data port is two consecutive word reads. */
        uint16_t hi = ata_ide_read16(&b->ide, 0);
        uint16_t lo = ata_ide_read16(&b->ide, 0);
        return ((uint32_t)hi << 16) | lo;
    }
    if (reg == 0 && size == 2)
        return ata_ide_read16(&b->ide, 0);
    if (reg >= 0)
        return ata_ide_read8(&b->ide, (uint8_t)reg);

    if (b->odfs != NULL && off >= ODFS_BANK_BASE && off < ODFS_BANK_END) {
        uint8_t b0 = odfs_byte(b, off);
        b->odfs_reads++;
        if (!b->logged_odfs) {
            b->logged_odfs = true;
            kprintf("[LIDE] guest started reading the ODFS bank at +%05x\n",
                    (unsigned)off);
        }
        if (size == 1) return b0;
        if (size == 2) return ((uint32_t)b0 << 8) | odfs_byte(b, off + 1u);
        return 0xFFFFFFFFu;
    }

    {
        uint8_t b0 = rom_byte(b, off);
        /* Milestones that say how far the guest got: reading the header means
         * expansion.library found us, and reading the device binary means the
         * boot loader is actually pulling lide.device in. */
        if (!b->logged_hdr) {
            b->logged_hdr = true;
            kprintf("[LIDE] guest started reading the board ROM at +%04x\n",
                    (unsigned)off);
        }
        if (!b->logged_dev && off == ROM_DEVICE_BOARD) {
            b->logged_dev = true;
            kprintf("[LIDE] guest is loading lide.device from the ROM\n");
        }
        b->rom_reads++;
        if (size == 1) return b0;
        if (size == 2) return ((uint32_t)b0 << 8) | rom_byte(b, off + 1u);
    }

    return 0xFFFFFFFFu;
}

void lide_write(lide_board_t *b, uint32_t addr, uint32_t value, unsigned size)
{
    uint32_t off;
    int reg;

    if (b == NULL || !b->configured) return;

    off = addr - b->base;

    reg = ata_ide_offset_to_reg(off);
    if (reg >= 0) b->ata_writes++;
    if (reg == 0 && size == 4) {
        ata_ide_write16(&b->ide, 0, (uint16_t)(value >> 16));
        ata_ide_write16(&b->ide, 0, (uint16_t)(value & 0xFFFFu));
        return;
    }
    if (reg == 0 && size == 2) {
        ata_ide_write16(&b->ide, 0, (uint16_t)value);
        return;
    }
    if (reg >= 0)
        ata_ide_write8(&b->ide, (uint8_t)reg, (uint8_t)value);
}
