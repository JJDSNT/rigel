#include <stdio.h>
#include <assert.h>

#include "denise/sprites/sprites.h"

int main(void)
{
    denise_sprites_state_t sprites;
    denise_sprite_t *sp;
    uint8_t pix;

    denise_sprites_reset(&sprites);

    /* --- reset state --- */
    for (unsigned i = 0; i < DENISE_SPRITE_COUNT; i++) {
        assert(!sprites.sp[i].armed);
        assert(!sprites.sp[i].visible);
    }
    assert(sprites.active_mask   == 0);
    assert(sprites.attached_mask == 0);

    /* --- hstart/vstart/vstop decoding ---
     * SPRxPOS = VSTART[7:0] | HSTART[8:1]
     * SPRxCTL = VSTOP[7:0]  | HSTART[0] | VSTART[8]
     *
     * Example: HSTART=0x50 (80 decimal), VSTART=0x2C, VSTOP=0x3A
     *   pos = (0x2C << 8) | (0x50 >> 1) = 0x2C28
     *   ctl = (0x3A << 8) | (0x50 & 1)  = 0x3A00  (HSTART bit0 = 0) */
    sp = &sprites.sp[2];
    sp->pos = 0x2C28u;
    sp->ctl = 0x3A00u;
    assert(denise_sprite_hstart(sp) == 0x50u);
    assert(denise_sprite_vstart(sp) == 0x2Cu);
    assert(denise_sprite_vstop(sp)  == 0x3Au);

    /* HSTART with bit0 set: pos=0x2C28, ctl=0x3A01 → HSTART = 0x51 */
    sp->ctl = 0x3A01u;
    assert(denise_sprite_hstart(sp) == 0x51u);

    /* --- pixel shifting ---
     * Arm sprite 0 at hstart=0x10, data=0xA000, datb=0x4000
     * hstart=0x10, data=1010_0000_0000_0000, datb=0100_0000_0000_0000
     * pixel at hpos=0x10: data_bit=1, datb_bit=0 → pixel=1
     * pixel at hpos=0x11: data_bit=0, datb_bit=1 → pixel=2
     * pixel at hpos=0x12: data_bit=1, datb_bit=0 → pixel=1
     * pixel at hpos=0x1F: last pixel in range */
    denise_sprites_reset(&sprites);
    denise_sprite_receive_ctrl(&sprites, 0,
        (rigel_u16)((0x40u << 8) | (0x10u >> 1)),  /* pos: vstart=0x40, hstart_hi */
        (rigel_u16)((0x50u << 8) | (0x10u & 1u))   /* ctl: vstop=0x50, hstart_lo=0 */
    );
    denise_sprite_receive_data(&sprites, 0, 0xA000u, 0x4000u);

    /* pixel at hpos exactly at hstart */
    pix = denise_sprite_pixel(&sprites, 0, 0x10u);
    assert(pix == 1);   /* data=1, datb=0 */

    pix = denise_sprite_pixel(&sprites, 0, 0x11u);
    assert(pix == 2);   /* data=0, datb=1 */

    pix = denise_sprite_pixel(&sprites, 0, 0x12u);
    assert(pix == 1);   /* data=1, datb=0 */

    /* transparent pixel: data=0, datb=0 at hpos=0x13 */
    pix = denise_sprite_pixel(&sprites, 0, 0x13u);
    assert(pix == 0);

    /* outside horizontal range → transparent */
    pix = denise_sprite_pixel(&sprites, 0, 0x0Fu);
    assert(pix == 0);
    pix = denise_sprite_pixel(&sprites, 0, 0x20u);
    assert(pix == 0);

    /* unarmed sprite → transparent */
    pix = denise_sprite_pixel(&sprites, 1, 0x10u);
    assert(pix == 0);

    /* --- attached sprite detection ---
     * Sprite 1 CTL bit 7 set → pair (0,1) is attached */
    denise_sprite_receive_ctrl(&sprites, 1, 0x0000u, 0x0080u);  /* ctl bit7=1 */
    assert( denise_sprite_is_attached(&sprites, 1));
    assert(!denise_sprite_is_attached(&sprites, 0));  /* even sprite: never attached */

    denise_sprite_receive_ctrl(&sprites, 1, 0x0000u, 0x0000u);  /* clear bit7 */
    assert(!denise_sprite_is_attached(&sprites, 1));

    printf("test_sprites: PASS\n");
    return 0;
}
