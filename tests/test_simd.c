#include "simd/rigel_simd.h"
#include "denise/output/planar.h"

#include <string.h>

static int test_fill_u32(void)
{
    rigel_u32 data[19];
    unsigned i;

    for (i = 0; i < 19u; ++i) {
        data[i] = 0x11111111u + i;
    }

    rigel_simd_fill_u32(&data[1], 0xaabbccddu, 17u);

    if (data[0] != 0x11111111u || data[18] != 0x11111123u) {
        return 1;
    }
    for (i = 1; i < 18u; ++i) {
        if (data[i] != 0xaabbccddu) {
            return 1;
        }
    }

    return 0;
}

static int test_fill_u16_and_zero(void)
{
    rigel_u8 bytes[23];
    rigel_u8 fill[21];
    rigel_u16 words[17];
    unsigned i;

    memset(bytes, 0x5a, sizeof(bytes));
    memset(fill, 0x11, sizeof(fill));
    for (i = 0; i < 17u; ++i) {
        words[i] = (rigel_u16)i;
    }

    rigel_simd_fill_u16(&words[2], 0x7beeu, 13u);
    if (words[0] != 0u || words[1] != 1u || words[15] != 15u || words[16] != 16u) {
        return 1;
    }
    for (i = 2; i < 15u; ++i) {
        if (words[i] != 0x7beeu) {
            return 1;
        }
    }

    rigel_simd_zero_u8(&bytes[3], 17u);
    if (bytes[0] != 0x5au || bytes[1] != 0x5au || bytes[2] != 0x5au ||
        bytes[20] != 0x5au || bytes[21] != 0x5au || bytes[22] != 0x5au) {
        return 1;
    }
    for (i = 3; i < 20u; ++i) {
        if (bytes[i] != 0u) {
            return 1;
        }
    }

    rigel_simd_fill_u8(&fill[1], 0xc3u, 19u);
    if (fill[0] != 0x11u || fill[20] != 0x11u) {
        return 1;
    }
    for (i = 1; i < 20u; ++i) {
        if (fill[i] != 0xc3u) {
            return 1;
        }
    }

    return 0;
}

static int test_copy_u16_to_le(void)
{
    const rigel_u16 src[] = { 0x0000u, 0x1234u, 0xbeefu, 0xff00u, 0x00ffu };
    rigel_u8 dst[sizeof(src)];

    memset(dst, 0xaa, sizeof(dst));
    rigel_simd_copy_u16_to_le(dst, src, 5u);

    return dst[0] != 0x00u || dst[1] != 0x00u ||
           dst[2] != 0x34u || dst[3] != 0x12u ||
           dst[4] != 0xefu || dst[5] != 0xbeu ||
           dst[6] != 0x00u || dst[7] != 0xffu ||
           dst[8] != 0xffu || dst[9] != 0x00u;
}

static uint8_t reference_planar_pixel(const rigel_u16 plane_words[6],
                                      unsigned num_planes,
                                      unsigned bit)
{
    uint8_t index = 0u;
    unsigned p;

    if (num_planes > 6u) {
        num_planes = 6u;
    }
    for (p = 0u; p < num_planes; ++p) {
        index |= (uint8_t)(((plane_words[p] >> (15u - bit)) & 1u) << p);
    }

    return index;
}

static int test_planar_to_chunky(void)
{
    static const rigel_u16 patterns[][6] = {
        { 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u },
        { 0xffffu, 0x0000u, 0x0000u, 0x0000u, 0x0000u, 0x0000u },
        { 0xaaaau, 0x5555u, 0xf0f0u, 0x0f0fu, 0xcc33u, 0x33ccu },
        { 0x8001u, 0x4002u, 0x2004u, 0x1008u, 0x0810u, 0x0420u },
        { 0x1357u, 0x2468u, 0x369cu, 0x5a5au, 0xc3c3u, 0x7e81u }
    };
    rigel_u8 pixels[16];
    unsigned pattern;
    unsigned planes;
    unsigned bit;

    for (pattern = 0u; pattern < sizeof(patterns) / sizeof(patterns[0]); ++pattern) {
        for (planes = 0u; planes <= 7u; ++planes) {
            memset(pixels, 0xa5, sizeof(pixels));
            planar_to_chunky(patterns[pattern], planes, pixels);

            for (bit = 0u; bit < 16u; ++bit) {
                if (pixels[bit] != reference_planar_pixel(patterns[pattern], planes, bit)) {
                    return 1;
                }
                if (pixels[bit] != planar_pixel_at(patterns[pattern], planes, bit)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

int main(void)
{
    if (test_fill_u32() ||
        test_fill_u16_and_zero() ||
        test_copy_u16_to_le() ||
        test_planar_to_chunky()) {
        return 1;
    }

    return 0;
}
