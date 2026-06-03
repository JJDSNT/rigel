#include "floppy/floppy_mfm.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint16_t read_be16(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static uint32_t read_be32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

static uint32_t get_mfm_long(const uint8_t *raw, uint32_t word_index)
{
    return read_be32(raw + word_index * 2u) & 0x55555555u;
}

static uint32_t get_decoded_long(const uint8_t *raw, uint32_t word_index, uint32_t offset_words)
{
    uint32_t odd = get_mfm_long(raw, word_index);
    uint32_t even = get_mfm_long(raw, word_index + offset_words);
    return (odd << 1) | even;
}

static uint32_t get_decoded_long_chk(
    const uint8_t *raw,
    uint32_t word_index,
    uint32_t offset_words,
    uint32_t *checksum)
{
    uint32_t odd = get_mfm_long(raw, word_index);
    uint32_t even = get_mfm_long(raw, word_index + offset_words);

    *checksum ^= odd ^ even;
    return (odd << 1) | even;
}

static int decode_track(const uint8_t *raw, uint32_t raw_len, uint8_t *decoded)
{
    uint32_t sector_bits = 0;
    uint32_t raw_words = raw_len / 2u;
    uint32_t pos = 0;

    while (sector_bits != ((1u << AMIGA_SECTORS_PER_TRACK) - 1u)) {
        uint32_t rawnext;
        uint32_t checksum;
        uint32_t id;
        uint32_t data_checksum;
        uint32_t track_offset;
        uint32_t i;

        if (pos != 0) {
            while (pos < raw_words && read_be16(raw + pos * 2u) != 0x4489u) {
                pos++;
            }
        }

        while (pos < raw_words && read_be16(raw + pos * 2u) == 0x4489u) {
            pos++;
        }

        if (pos + 544u >= raw_words) {
            return 26;
        }

        rawnext = pos + 544u - 3u;
        checksum = 0;
        id = get_decoded_long_chk(raw, pos, 2u, &checksum);
        pos += 4u;

        track_offset = (id >> 8) & 0xffu;
        if ((id & 0xff000000u) != 0xff000000u ||
            ((id >> 16) & 0xffu) != 0u ||
            track_offset >= AMIGA_SECTORS_PER_TRACK) {
            continue;
        }

        if ((sector_bits & (1u << track_offset)) != 0u) {
            pos = rawnext;
            continue;
        }

        for (i = 0; i < 4u; i++) {
            (void)get_decoded_long_chk(raw, pos, 8u, &checksum);
            pos += 2u;
        }

        pos += 8u;
        if (get_decoded_long(raw, pos, 2u) != checksum) {
            return 24;
        }
        pos += 4u;

        data_checksum = get_decoded_long(raw, pos, 2u);
        pos += 4u;

        for (i = 0; i < 128u; i++) {
            uint32_t value = get_decoded_long_chk(raw, pos, 256u, &data_checksum);
            uint8_t *dst = decoded + track_offset * AMIGA_SECTOR_SIZE + i * 4u;

            dst[0] = (uint8_t)(value >> 24);
            dst[1] = (uint8_t)(value >> 16);
            dst[2] = (uint8_t)(value >> 8);
            dst[3] = (uint8_t)value;
            pos += 2u;
        }

        if (data_checksum != 0u) {
            return 25;
        }

        sector_bits |= 1u << track_offset;
    }

    return 0;
}

int main(void)
{
    uint8_t track[AMIGA_TRACK_SIZE];
    uint8_t encoded[RIGEL_FLOPPY_MFM_TRACK_BYTES_PAL];
    uint8_t dma_buffer[14716u];
    uint8_t decoded[AMIGA_TRACK_SIZE];
    uint32_t i;

    for (i = 0; i < (uint32_t)sizeof(track); i++) {
        track[i] = (uint8_t)((i * 37u + 13u) & 0xffu);
    }

    memset(encoded, 0, sizeof(encoded));
    memset(dma_buffer, 0, sizeof(dma_buffer));
    memset(decoded, 0, sizeof(decoded));

    if (!floppy_mfm_encode_track(encoded, sizeof(encoded), track, 0, AMIGA_SECTORS_PER_TRACK)) {
        return 1;
    }

    for (i = 0; i < (uint32_t)sizeof(dma_buffer); i += 2u) {
        uint32_t src = i % (uint32_t)sizeof(encoded);
        dma_buffer[i] = encoded[src];
        dma_buffer[i + 1u] = encoded[src + 1u];
    }

    int err = decode_track(dma_buffer, (uint32_t)sizeof(dma_buffer), decoded);
    if (err != 0) {
        fprintf(stderr, "decode_track failed err=%d\n", err);
        return 1;
    }

    if (memcmp(track, decoded, sizeof(track)) != 0) {
        fprintf(stderr, "decoded track mismatch\n");
        return 1;
    }

    return 0;
}
