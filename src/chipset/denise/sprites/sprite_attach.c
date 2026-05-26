#include "sprites.h"

/* Attached sprite mode — pairs (0,1), (2,3), (4,5), (6,7).
 *
 * When the odd sprite of a pair has CTL bit 7 set, both sprites are attached:
 *   - Even sprite provides DATA/DATB as the lower 2 bits
 *   - Odd  sprite provides DATA/DATB as the upper 2 bits
 *   - Combined 4-bit index → 16-color sprite using colors 16–31
 *
 * The even sprite's position registers define the combined sprite's position. */

/* Compute the 4-bit color index for an attached sprite pair at `hpos`.
 * `even_sp` is the index of the even sprite (0, 2, 4, or 6).
 * Returns 0 if transparent. */
uint8_t denise_sprite_attached_pixel(const denise_sprites_state_t *s,
                                     unsigned even_sp, rigel_u16 hpos)
{
    unsigned odd_sp;
    uint8_t lo, hi;

    if (even_sp >= DENISE_SPRITE_COUNT - 1 || (even_sp & 1u)) return 0;
    odd_sp = even_sp + 1;

    lo = denise_sprite_pixel(s, even_sp, hpos);
    hi = denise_sprite_pixel(s, odd_sp,  hpos);

    if (!lo && !hi) return 0;

    /* Colors 16-31 for attached sprites */
    return (uint8_t)(16 + ((hi << 2) | lo));
}
