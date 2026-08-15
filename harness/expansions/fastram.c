#include "fastram.h"

#include <stdlib.h>
#include <string.h>

#include "shim.h"
#include "zorro.h"

/* er_Type: a Zorro II board whose memory joins the system free list. */
enum {
    AC_TYPE_Z2      = 0xC0,
    AC_TYPE_MEMLIST = 0x20
};

/* Size codes as they appear in the low bits of er_Type. */
static uint8_t size_code(uint32_t bytes)
{
    if (bytes >= 8u * 1024u * 1024u) return 0x00u;   /* 8 MB is code 0 */
    if (bytes >= 4u * 1024u * 1024u) return 0x07u;
    if (bytes >= 2u * 1024u * 1024u) return 0x06u;
    return 0x05u;                                     /* 1 MB */
}

struct fastram_board {
    uint8_t  config[ZORRO_AC_DATA_SIZE];
    bool     configured;
    bool     shutup;
    uint32_t base;

    uint8_t *ram;
    uint32_t size;
};

fastram_board_t *fastram_create(uint32_t megabytes)
{
    uint8_t raw[ZORRO_AC_ROM_BYTES];
    fastram_board_t *b;

    if (megabytes != 1u && megabytes != 2u &&
        megabytes != 4u && megabytes != 8u)
        return NULL;

    b = (fastram_board_t *)calloc(1, sizeof(*b));
    if (b == NULL) return NULL;

    b->size = megabytes * 1024u * 1024u;
    b->ram  = (uint8_t *)calloc(1, b->size);
    if (b->ram == NULL) {
        free(b);
        return NULL;
    }

    memset(raw, 0, sizeof(raw));
    raw[0] = (uint8_t)(AC_TYPE_Z2 | AC_TYPE_MEMLIST | size_code(b->size));
    raw[1] = 0x01u;      /* er_Product */
    raw[4] = 0x07u;      /* manufacturer 0x07DB */
    raw[5] = 0xDBu;
    raw[9] = 0x01u;      /* serial */
    zorro_autoconfig_build(b->config, raw);

    return b;
}

void fastram_destroy(fastram_board_t *b)
{
    if (b == NULL) return;
    free(b->ram);
    free(b);
}

bool fastram_pending(const fastram_board_t *b)
{
    return b != NULL && !b->configured && !b->shutup;
}

uint8_t fastram_ac_read(const fastram_board_t *b, uint32_t off)
{
    if (b == NULL || off >= ZORRO_AC_DATA_SIZE) return 0xFFu;
    return b->config[off];
}

void fastram_ac_write(fastram_board_t *b, uint32_t off, uint8_t value)
{
    if (b == NULL) return;

    if (off == ZORRO_AC_OFF_BASE_HI) {
        b->base = (uint32_t)((value & 0xFFu) << 16);
        b->configured = true;
        kprintf("[FASTRAM] %u MB at %06x\n",
                (unsigned)(b->size / (1024u * 1024u)), (unsigned)b->base);
    } else if (off == ZORRO_AC_OFF_SHUTUP) {
        b->shutup = true;
        kprintf("[FASTRAM] shut up by the guest\n");
    }
}

void fastram_force_configure(fastram_board_t *b, uint32_t base)
{
    if (b == NULL || b->configured) return;
    b->base = base;
    b->configured = true;
    kprintf("[FASTRAM] %u MB forced to %06x (no autoconfig without a ROM)\n",
            (unsigned)(b->size / (1024u * 1024u)), (unsigned)base);
}

bool fastram_owns(const fastram_board_t *b, uint32_t addr)
{
    return b != NULL && b->configured &&
           addr >= b->base && addr < b->base + b->size;
}

uint32_t fastram_read(const fastram_board_t *b, uint32_t addr, unsigned size)
{
    uint32_t off = addr - b->base;

    /* Fast RAM is plain memory: no side effects, so a straight big-endian
     * fetch is all this needs. */
    if (size == 1) return b->ram[off];
    if (off + 1u >= b->size) return 0xFFFFFFFFu;
    if (size == 2)
        return ((uint32_t)b->ram[off] << 8) | b->ram[off + 1u];
    if (off + 3u >= b->size) return 0xFFFFFFFFu;
    return ((uint32_t)b->ram[off]      << 24) |
           ((uint32_t)b->ram[off + 1u] << 16) |
           ((uint32_t)b->ram[off + 2u] <<  8) |
            (uint32_t)b->ram[off + 3u];
}

void fastram_write(fastram_board_t *b, uint32_t addr, uint32_t value, unsigned size)
{
    uint32_t off = addr - b->base;

    if (size == 1) { b->ram[off] = (uint8_t)value; return; }
    if (off + 1u >= b->size) return;
    if (size == 2) {
        b->ram[off]      = (uint8_t)(value >> 8);
        b->ram[off + 1u] = (uint8_t)value;
        return;
    }
    if (off + 3u >= b->size) return;
    b->ram[off]      = (uint8_t)(value >> 24);
    b->ram[off + 1u] = (uint8_t)(value >> 16);
    b->ram[off + 2u] = (uint8_t)(value >>  8);
    b->ram[off + 3u] = (uint8_t)value;
}

uint32_t fastram_base(const fastram_board_t *b) { return b ? b->base : 0u; }
uint32_t fastram_size(const fastram_board_t *b) { return b ? b->size : 0u; }
