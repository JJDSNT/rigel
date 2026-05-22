#include "planar.h"

void planar_to_chunky(const rigel_u16 plane_words[6], unsigned num_planes,
                      uint8_t pixels_out[16])
{
    /* TODO(output): replace with SIMD-friendly implementation.
     * Current: portable scalar loop, MSB = pixel 0. */
    if (num_planes > 6) num_planes = 6;

    for (unsigned px = 0; px < 16; px++) {
        unsigned shift = 15u - px;
        uint8_t index  = 0;
        for (unsigned p = 0; p < num_planes; p++)
            index |= (uint8_t)(((plane_words[p] >> shift) & 1u) << p);
        pixels_out[px] = index;
    }
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
