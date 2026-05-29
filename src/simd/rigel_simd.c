#include "simd/rigel_simd.h"

#include <string.h>

#ifndef RIGEL_ENABLE_SIMD
#define RIGEL_ENABLE_SIMD 1
#endif

#if RIGEL_ENABLE_SIMD && (defined(__x86_64__) || defined(_M_X64))
#include <emmintrin.h>
#define RIGEL_SIMD_SSE2 1
#elif RIGEL_ENABLE_SIMD && (defined(__aarch64__) || defined(_M_ARM64))
#include <arm_neon.h>
#define RIGEL_SIMD_NEON 1
#endif

static int rigel_host_is_little_endian(void)
{
    const rigel_u16 value = 1u;
    return *((const rigel_u8 *)&value) == 1u;
}

void rigel_simd_fill_u32(rigel_u32 *dst, rigel_u32 value, rigel_u32 count)
{
    rigel_u32 i = 0;

    if (dst == NULL || count == 0u) {
        return;
    }

#if defined(RIGEL_SIMD_SSE2)
    {
        __m128i v = _mm_set1_epi32((int)value);
        for (; i + 4u <= count; i += 4u) {
            _mm_storeu_si128((__m128i *)(void *)&dst[i], v);
        }
    }
#elif defined(RIGEL_SIMD_NEON)
    {
        uint32x4_t v = vdupq_n_u32(value);
        for (; i + 4u <= count; i += 4u) {
            vst1q_u32(&dst[i], v);
        }
    }
#endif

    for (; i < count; ++i) {
        dst[i] = value;
    }
}

void rigel_simd_fill_u16(rigel_u16 *dst, rigel_u16 value, rigel_u32 count)
{
    rigel_u32 i = 0;

    if (dst == NULL || count == 0u) {
        return;
    }

#if defined(RIGEL_SIMD_SSE2)
    {
        __m128i v = _mm_set1_epi16((short)value);
        for (; i + 8u <= count; i += 8u) {
            _mm_storeu_si128((__m128i *)(void *)&dst[i], v);
        }
    }
#elif defined(RIGEL_SIMD_NEON)
    {
        uint16x8_t v = vdupq_n_u16(value);
        for (; i + 8u <= count; i += 8u) {
            vst1q_u16(&dst[i], v);
        }
    }
#endif

    for (; i < count; ++i) {
        dst[i] = value;
    }
}

void rigel_simd_fill_u8(rigel_u8 *dst, rigel_u8 value, rigel_u32 count)
{
    rigel_u32 i = 0;

    if (dst == NULL || count == 0u) {
        return;
    }

#if defined(RIGEL_SIMD_SSE2)
    {
        __m128i v = _mm_set1_epi8((char)value);
        for (; i + 16u <= count; i += 16u) {
            _mm_storeu_si128((__m128i *)(void *)&dst[i], v);
        }
    }
#elif defined(RIGEL_SIMD_NEON)
    {
        uint8x16_t v = vdupq_n_u8(value);
        for (; i + 16u <= count; i += 16u) {
            vst1q_u8(&dst[i], v);
        }
    }
#endif

    for (; i < count; ++i) {
        dst[i] = value;
    }
}

void rigel_simd_zero_u8(rigel_u8 *dst, rigel_u32 count)
{
    rigel_u32 i = 0;

    if (dst == NULL || count == 0u) {
        return;
    }

#if defined(RIGEL_SIMD_SSE2)
    {
        __m128i z = _mm_setzero_si128();
        for (; i + 16u <= count; i += 16u) {
            _mm_storeu_si128((__m128i *)(void *)&dst[i], z);
        }
    }
#elif defined(RIGEL_SIMD_NEON)
    {
        uint8x16_t z = vdupq_n_u8(0u);
        for (; i + 16u <= count; i += 16u) {
            vst1q_u8(&dst[i], z);
        }
    }
#endif

    for (; i < count; ++i) {
        dst[i] = 0u;
    }
}

void rigel_simd_copy_u16(rigel_u16 *dst, const rigel_u16 *src, rigel_u32 count)
{
    if (dst == NULL || src == NULL || count == 0u) {
        return;
    }

    (void)memcpy(dst, src, count * sizeof(rigel_u16));
}

void rigel_simd_copy_u16_to_le(rigel_u8 *dst, const rigel_u16 *src, rigel_u32 count)
{
    rigel_u32 i;

    if (dst == NULL || src == NULL || count == 0u) {
        return;
    }

    if (rigel_host_is_little_endian()) {
        (void)memcpy(dst, src, count * sizeof(rigel_u16));
        return;
    }

    for (i = 0; i < count; ++i) {
        rigel_u16 value = src[i];
        dst[i * 2u] = (rigel_u8)(value & 0xffu);
        dst[i * 2u + 1u] = (rigel_u8)(value >> 8);
    }
}
