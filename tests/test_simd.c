#include "simd/rigel_simd.h"

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

int main(void)
{
    if (test_fill_u32() ||
        test_fill_u16_and_zero() ||
        test_copy_u16_to_le()) {
        return 1;
    }

    return 0;
}
