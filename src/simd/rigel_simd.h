#ifndef RIGEL_SIMD_H
#define RIGEL_SIMD_H

#include "rigel/rigel_types.h"

void rigel_simd_fill_u32(rigel_u32 *dst, rigel_u32 value, rigel_u32 count);
void rigel_simd_fill_u16(rigel_u16 *dst, rigel_u16 value, rigel_u32 count);
void rigel_simd_fill_u8(rigel_u8 *dst, rigel_u8 value, rigel_u32 count);
void rigel_simd_zero_u8(rigel_u8 *dst, rigel_u32 count);
void rigel_simd_copy_u16(rigel_u16 *dst, const rigel_u16 *src, rigel_u32 count);
void rigel_simd_copy_u16_to_le(rigel_u8 *dst, const rigel_u16 *src, rigel_u32 count);

#endif
