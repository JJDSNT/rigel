#include "hunk.h"

#include <stdio.h>
#include <string.h>

/* Hunk block types, from the AmigaOS binary format. */
enum {
    HUNK_HEADER  = 0x3F3,
    HUNK_CODE    = 0x3E9,
    HUNK_DATA    = 0x3EA,
    HUNK_BSS     = 0x3EB,
    HUNK_RELOC32 = 0x3EC,
    HUNK_SYMBOL  = 0x3F0,
    HUNK_DEBUG   = 0x3F1,
    HUNK_END     = 0x3F2
};

enum {
    MAX_HUNKS = 64,
    /* Each segment is preceded by [size][next], the same two longwords
     * AmigaOS puts there, because relocations are expressed relative to a
     * segment's data start and the seglist is walked through `next`. */
    SEG_HEADER = 8
};

typedef struct reader {
    const uint8_t *data;
    uint32_t       size;
    uint32_t       pos;
    bool           overrun;
} reader_t;

static uint32_t rd32(reader_t *r)
{
    uint32_t v;

    if (r->pos + 4u > r->size) {
        r->overrun = true;
        return 0;
    }
    v = ((uint32_t)r->data[r->pos] << 24) |
        ((uint32_t)r->data[r->pos + 1] << 16) |
        ((uint32_t)r->data[r->pos + 2] << 8) |
         (uint32_t)r->data[r->pos + 3];
    r->pos += 4u;
    return v;
}

typedef struct mem {
    void *ctx;
    void (*write8)(void *, uint32_t, uint8_t);
    uint8_t (*read8)(void *, uint32_t);
} mem_t;

static void put32(const mem_t *m, uint32_t addr, uint32_t v)
{
    m->write8(m->ctx, addr,      (uint8_t)(v >> 24));
    m->write8(m->ctx, addr + 1u, (uint8_t)(v >> 16));
    m->write8(m->ctx, addr + 2u, (uint8_t)(v >> 8));
    m->write8(m->ctx, addr + 3u, (uint8_t)v);
}

static uint32_t get32(const mem_t *m, uint32_t addr)
{
    return ((uint32_t)m->read8(m->ctx, addr)      << 24) |
           ((uint32_t)m->read8(m->ctx, addr + 1u) << 16) |
           ((uint32_t)m->read8(m->ctx, addr + 2u) << 8)  |
            (uint32_t)m->read8(m->ctx, addr + 3u);
}

#define FAIL(...)                                       \
    do {                                                \
        if (err != NULL) snprintf(err, err_len, __VA_ARGS__); \
        return false;                                   \
    } while (0)

bool hunk_load(const uint8_t *file, uint32_t file_size,
               uint32_t load_addr, uint32_t limit,
               void *ctx,
               void (*write8)(void *ctx, uint32_t addr, uint8_t value),
               uint8_t (*read8)(void *ctx, uint32_t addr),
               hunk_image_t *out,
               char *err, size_t err_len)
{
    reader_t r = { file, file_size, 0, false };
    mem_t    m = { ctx, write8, read8 };

    uint32_t seg_data[MAX_HUNKS];   /* guest address of each segment's data */
    uint32_t seg_size[MAX_HUNKS];
    uint32_t first, last, count, i;
    uint32_t cursor = load_addr;
    uint32_t block = 0;

    if (file == NULL || out == NULL) FAIL("no image");

    if (rd32(&r) != HUNK_HEADER) FAIL("not a hunk file (no HUNK_HEADER)");
    /* Resident library name list; empty for an executable. */
    while (rd32(&r) != 0u) {
        if (r.overrun) FAIL("truncated name list");
    }

    (void)rd32(&r);          /* table size — first/last is what matters */
    first = rd32(&r);
    last  = rd32(&r);
    if (r.overrun) FAIL("truncated header");
    if (last < first) FAIL("bad hunk range %u..%u", first, last);

    count = last - first + 1u;
    if (count > MAX_HUNKS) FAIL("%u hunks, limit is %u", count, MAX_HUNKS);

    /*
     * Reserve every segment up front, exactly as LoadSeg does: relocations can
     * point at a hunk that has not been read yet, so all the addresses have to
     * exist before any data is placed.
     */
    for (i = 0; i < count; i++) {
        uint32_t longs = rd32(&r) & 0x3FFFFFFFu;   /* top bits are MEMF_ flags */
        uint32_t bytes = longs * 4u;

        if (cursor + SEG_HEADER + bytes > limit)
            FAIL("hunk %u needs %u bytes; only %u free at %08x",
                 i, bytes, limit > cursor ? limit - cursor : 0u, cursor);

        put32(&m, cursor, bytes);              /* h_Size */
        put32(&m, cursor + 4u, 0u);            /* h_Next, filled in below */
        seg_data[i] = cursor + SEG_HEADER;
        seg_size[i] = bytes;

        /* Segments are zeroed: a BSS hunk carries no file data and expects it. */
        {
            uint32_t b;
            for (b = 0; b < bytes; b++) m.write8(m.ctx, seg_data[i] + b, 0u);
        }

        cursor = (cursor + SEG_HEADER + bytes + 31u) & ~31u;
    }
    if (r.overrun) FAIL("truncated hunk size table");

    /* Chain the seglist: h_Next points at the next segment's h_Next field,
     * which is what the relocation walk below follows. */
    for (i = 0; i + 1u < count; i++)
        put32(&m, seg_data[i] - 4u, seg_data[i + 1u] - 4u);

    /* Second pass: place the data and relocate. */
    while (r.pos < r.size && block < count) {
        uint32_t type = rd32(&r) & 0x3FFFFFFFu;

        if (r.overrun) break;

        switch (type) {
        case HUNK_CODE:
        case HUNK_DATA: {
            uint32_t longs = rd32(&r);
            uint32_t bytes = longs * 4u;
            uint32_t b;

            if (r.pos + bytes > r.size) FAIL("truncated hunk %u", block);
            if (bytes > seg_size[block])
                FAIL("hunk %u is %u bytes but was reserved %u",
                     block, bytes, seg_size[block]);

            for (b = 0; b < bytes; b++)
                m.write8(m.ctx, seg_data[block] + b, r.data[r.pos + b]);
            r.pos += bytes;
            break;
        }

        case HUNK_BSS:
            (void)rd32(&r);      /* size; the segment is already zeroed */
            break;

        case HUNK_RELOC32: {
            /* Blocks of (count, target-hunk, offsets...) until a zero count. */
            for (;;) {
                uint32_t n = rd32(&r);
                uint32_t target;

                if (n == 0u || r.overrun) break;
                target = rd32(&r);
                if (target >= count) FAIL("relocation targets hunk %u", target);

                while (n-- > 0u) {
                    uint32_t off = rd32(&r);
                    if (r.overrun) FAIL("truncated relocations");
                    if (off + 4u > seg_size[block])
                        FAIL("relocation at %08x is past hunk %u", off, block);
                    put32(&m, seg_data[block] + off,
                          get32(&m, seg_data[block] + off) + seg_data[target]);
                }
            }
            break;
        }

        case HUNK_END:
            block++;
            break;

        case HUNK_SYMBOL: {
            /* Name-length-prefixed entries until a zero length. */
            for (;;) {
                uint32_t n = rd32(&r);
                if (n == 0u || r.overrun) break;
                r.pos += n * 4u + 4u;    /* name plus its value */
            }
            break;
        }

        case HUNK_DEBUG: {
            uint32_t longs = rd32(&r);
            r.pos += longs * 4u;
            break;
        }

        default:
            /* Anything else ends the useful part of the file. */
            r.pos = r.size;
            break;
        }
    }

    out->entry      = seg_data[0];
    out->base       = load_addr;
    out->end        = cursor;
    out->hunk_count = count;
    return true;
}
