#include "planar.h"

#ifndef RIGEL_ENABLE_SIMD
#define RIGEL_ENABLE_SIMD 1
#endif

#if RIGEL_ENABLE_SIMD && (defined(__aarch64__) || defined(_M_ARM64))
#include <arm_neon.h>
#define RIGEL_PLANAR_NEON 1
#endif

#if !defined(RIGEL_PLANAR_NEON)
static void planar_to_chunky_scalar(const rigel_u16 plane_words[6],
                                    unsigned num_planes,
                                    uint8_t pixels_out[16])
{
    unsigned px;

    if (num_planes > 6) {
        num_planes = 6;
    }

    for (px = 0; px < 16; px++) {
        unsigned shift = 15u - px;
        uint8_t index = 0;
        unsigned p;

        for (p = 0; p < num_planes; p++) {
            index |= (uint8_t)(((plane_words[p] >> shift) & 1u) << p);
        }
        pixels_out[px] = index;
    }
}
#endif

#if defined(RIGEL_PLANAR_NEON)
static uint8x8_t planar_neon_expand8(rigel_u16 word, uint16x8_t masks,
                                     rigel_u8 bit_value)
{
    uint16x8_t bits = vandq_u16(vdupq_n_u16(word), masks);
    uint16x8_t present = vcgtq_u16(bits, vdupq_n_u16(0u));

    return vmovn_u16(vandq_u16(present, vdupq_n_u16(bit_value)));
}

static void planar_to_chunky_neon(const rigel_u16 plane_words[6],
                                  unsigned num_planes,
                                  uint8_t pixels_out[16])
{
    static const rigel_u16 high_mask_data[8] = {
        0x8000u, 0x4000u, 0x2000u, 0x1000u,
        0x0800u, 0x0400u, 0x0200u, 0x0100u
    };
    static const rigel_u16 low_mask_data[8] = {
        0x0080u, 0x0040u, 0x0020u, 0x0010u,
        0x0008u, 0x0004u, 0x0002u, 0x0001u
    };
    uint16x8_t high_masks = vld1q_u16(high_mask_data);
    uint16x8_t low_masks = vld1q_u16(low_mask_data);
    uint8x8_t high = vdup_n_u8(0u);
    uint8x8_t low = vdup_n_u8(0u);
    unsigned p;

    if (num_planes > 6) {
        num_planes = 6;
    }

    for (p = 0; p < num_planes; ++p) {
        rigel_u8 bit_value = (rigel_u8)(1u << p);

        high = vorr_u8(high, planar_neon_expand8(plane_words[p], high_masks, bit_value));
        low = vorr_u8(low, planar_neon_expand8(plane_words[p], low_masks, bit_value));
    }

    vst1_u8(&pixels_out[0], high);
    vst1_u8(&pixels_out[8], low);
}
#endif

void planar_to_chunky(const rigel_u16 plane_words[6], unsigned num_planes,
                      uint8_t pixels_out[16])
{
#if defined(RIGEL_PLANAR_NEON)
    planar_to_chunky_neon(plane_words, num_planes, pixels_out);
#else
    planar_to_chunky_scalar(plane_words, num_planes, pixels_out);
#endif
}

uint8_t planar_pixel_at(const rigel_u16 plane_words[6], unsigned num_planes, unsigned bit)
{
    if (num_planes > 6) num_planes = 6;
    if (bit > 15) return 0;

    unsigned shift = 15u - bit;
    uint8_t index  = 0;
    for (unsigned p = 0; p < num_planes; p++)
        index |= (uint8_t)(((plane_words[p] >> shift) & 1u) << p);
    return index;
}
