#include "floppy/floppy_mfm.h"

#include <string.h>

#define MFM_MASK 0x55555555u

static void write_be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)((v >> 24) & 0xff);
    p[1] = (uint8_t)((v >> 16) & 0xff);
    p[2] = (uint8_t)((v >> 8) & 0xff);
    p[3] = (uint8_t)(v & 0xff);
}

static uint32_t read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           ((uint32_t)p[3]);
}

uint32_t floppy_mfm_checksum(
    const uint32_t *data,
    size_t longwords)
{
    uint32_t sum = 0;

    if (!data) {
        return 0;
    }

    for (size_t i = 0; i < longwords; i++) {
        sum ^= (data[i] & MFM_MASK);
    }

    return sum;
}

uint32_t floppy_mfm_encode_even_odd(
    uint32_t value,
    uint32_t *even,
    uint32_t *odd)
{
    uint32_t e = (value >> 1) & MFM_MASK;
    uint32_t o = value & MFM_MASK;

    if (even) *even = e;
    if (odd)  *odd  = o;

    return 0;
}

uint16_t floppy_mfm_encode_word(
    uint16_t raw,
    uint16_t previous)
{
    uint16_t out = 0;

    for (int i = 15; i >= 0; i--) {
        uint16_t bit = (raw >> i) & 1u;
        uint16_t prev_data =
            (i == 15)
                ? (previous & 1u)
                : ((raw >> (i + 1)) & 1u);
        uint16_t clock = (!prev_data && !bit) ? 1u : 0u;

        out <<= 1;
        out |= clock;
        out <<= 1;
        out |= bit;
    }

    return out;
}

/* Insert MFM clock bits into an array of already-encoded longwords.
 * A clock bit at position p is 1 when both neighboring data bits are 0. */
static void mfmcode_u32(uint32_t *mfm, uint32_t longs)
{
    uint32_t prev = mfm[-1];

    while (longs-- != 0) {
        uint32_t v = *mfm & MFM_MASK;
        uint32_t mask1 = (prev << 31) | (v >> 1);
        uint32_t mask2 = v << 1;
        uint32_t clk   = ((uint32_t)(~mask1) & (uint32_t)(~mask2)) & 0xAAAAAAAAu;
        prev = v;
        *mfm++  = v | clk;
    }
}

bool floppy_mfm_encode_sector(
    uint8_t *dst,
    size_t dst_size,
    const uint8_t *sector_data,
    uint32_t track,
    uint32_t sector,
    uint32_t sectors_per_track)
{
    /*
     * Standard Amiga MFM sector, 272 longwords = 1088 bytes:
     *
     *  buf[0]       : 0xAAAAAAAA pre-sync gap
     *  buf[1]       : 0x44894489 (two sync marks)
     *  buf[2]       : info odd  = (info >> 1) & 0x55
     *  buf[3]       : info even = info & 0x55
     *                 where info = 0xFF | track | sector | sectors_to_gap
     *  buf[4..11]   : sector label (8 zero longwords)
     *  buf[12]      : header checksum odd
     *  buf[13]      : header checksum even
     *  buf[14]      : data checksum odd
     *  buf[15]      : data checksum even
     *  buf[16..143] : data odd  (128 longwords, bit 0,2,4,... of each raw LW)
     *  buf[144..271]: data even (128 longwords, bit 1,3,5,... shifted right)
     *
     * After filling, mfmcode_u32(buf+2, 270) inserts clock bits.
     */
    uint32_t buf[272];
    uint32_t info, odd, even;
    uint32_t hck = 0, dck = 0;
    uint32_t i;

    if (!dst || !sector_data || dst_size < AMIGA_MFM_SECTOR_SIZE) {
        return false;
    }

    memset(buf, 0, sizeof(buf));

    buf[0] = 0xAAAAAAAAu;
    buf[1] = 0x44894489u;

    info = (0xFFu << 24) |
           ((track & 0xFFu) << 16) |
           ((sector & 0xFFu) << 8) |
           ((sectors_per_track - sector) & 0xFFu);

    odd  = (info >> 1) & MFM_MASK;
    even = info & MFM_MASK;
    buf[2] = odd;
    buf[3] = even;
    hck ^= odd;
    hck ^= even;

    /* buf[4..11] remain zero (sector label) */

    for (i = 0; i < 128u; i++) {
        uint32_t raw = read_be32(sector_data + i * 4u);
        odd  = (raw >> 1) & MFM_MASK;
        even = raw & MFM_MASK;
        buf[16u + i]        = odd;
        buf[16u + 128u + i] = even;
        dck ^= odd;
        dck ^= even;
    }

    buf[12] = hck >> 1;
    buf[13] = hck;
    buf[14] = dck >> 1;
    buf[15] = dck;

    mfmcode_u32(buf + 2, 270u);

    for (i = 0; i < 272u; i++) {
        write_be32(dst + i * 4u, buf[i]);
    }

    return true;
}

bool floppy_mfm_encode_track(
    uint8_t *dst,
    size_t dst_size,
    const uint8_t *track_data,
    uint32_t track,
    uint32_t sectors_per_track)
{
    uint32_t sec;
    uint32_t data_bytes = sectors_per_track * AMIGA_MFM_SECTOR_SIZE;
    uint32_t gapsize;
    uint32_t gap_words;
    uint32_t pos = 0;
    uint32_t i;

    if (!dst || !track_data) {
        return false;
    }

    if (dst_size < data_bytes) {
        return false;
    }

    gapsize   = (dst_size > data_bytes) ? (uint32_t)(dst_size - data_bytes) : 0u;
    gap_words = (gapsize / 4u > 1u) ? (gapsize / 4u - 1u) : 0u;

    /* Leading gap */
    for (i = 0; i < gap_words; i++) {
        write_be32(dst + pos, 0xAAAAAAAAu);
        pos += 4u;
    }

    /* Sectors */
    for (sec = 0; sec < sectors_per_track; sec++) {
        const uint8_t *sector_ptr = track_data + sec * AMIGA_SECTOR_SIZE;
        uint32_t prev_last_bit = (pos >= 4u) ?
            (read_be32(dst + pos - 4u) & 1u) : 0u;
        floppy_mfm_encode_sector(
            dst + pos,
            AMIGA_MFM_SECTOR_SIZE,
            sector_ptr,
            track,
            sec,
            sectors_per_track);
        write_be32(dst + pos, prev_last_bit ? 0x2AAAAAAAu : 0xAAAAAAAAu);
        pos += AMIGA_MFM_SECTOR_SIZE;
    }

    /* Trailing longword, matching the legacy encoder's final zero word. */
    if (pos + 4u <= (uint32_t)dst_size) {
        uint32_t tail[2];

        tail[0] = (pos >= 4u) ? read_be32(dst + pos - 4u) : 0u;
        tail[1] = 0u;
        mfmcode_u32(&tail[1], 1u);
        write_be32(dst + pos, tail[1]);
    }

    return true;
}
