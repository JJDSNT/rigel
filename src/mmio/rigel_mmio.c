#include "rigel/rigel_mmio.h"

#include "mmio/custom_regs.h"
#include "rigel/rigel_cia.h"
#include "rigel/rigel_rtc.h"

#define RIGEL_M68K_ADDRESS_MAX 0x00ffffffu
#define RIGEL_CUSTOM_BASE      0x00dff000u
#define RIGEL_CUSTOM_END       0x00dfffffu
#define RIGEL_CIAB_BASE        0x00bfd000u
#define RIGEL_CIAB_END         0x00bfdf00u
#define RIGEL_CIA_SHARED_BASE  0x00bfe000u
#define RIGEL_CIA_SHARED_END   0x00bfef01u
#define RIGEL_RTC_BASE         0x00dc0000u
#define RIGEL_RTC_END          0x00dcffffu

static bool custom_address(rigel_u32 addr)
{
    return addr >= RIGEL_CUSTOM_BASE && addr <= RIGEL_CUSTOM_END;
}

static int cia_id_for_address(rigel_u32 addr)
{
    if ((addr & 1u) != 0u && addr >= RIGEL_CIA_SHARED_BASE &&
        addr <= RIGEL_CIA_SHARED_END) {
        return 0;
    }

    if ((addr & 1u) == 0u &&
        ((addr >= RIGEL_CIAB_BASE && addr <= RIGEL_CIAB_END) ||
         (addr >= RIGEL_CIA_SHARED_BASE && addr < RIGEL_CIA_SHARED_END))) {
        return 1;
    }

    return -1;
}

static rigel_u16 custom_read_word(RigelContext *ctx, rigel_u32 addr)
{
    return rigel_custom_read16(ctx, addr & 0x1feu);
}

static void custom_write_word(RigelContext *ctx, rigel_u32 addr,
                              rigel_u16 value)
{
    rigel_custom_write16(ctx, addr & 0x1feu, value);
}

rigel_mmio_result_t rigel_mmio_read(RigelContext *ctx, rigel_u32 addr,
                                    rigel_u8 size, rigel_u32 *value)
{
    int cia_id;

    if (ctx == NULL || value == NULL || addr > RIGEL_M68K_ADDRESS_MAX) {
        return RIGEL_MMIO_UNMAPPED;
    }

    if (custom_address(addr)) {
        rigel_u16 word;

        if ((size == 2u || size == 4u) && (addr & 1u) != 0u) {
            return RIGEL_MMIO_UNSUPPORTED;
        }
        if (size == 4u && !custom_address(addr + 3u)) {
            return RIGEL_MMIO_UNSUPPORTED;
        }

        word = custom_read_word(ctx, addr);
        if (size == 1u) {
            *value = (addr & 1u) != 0u ? (rigel_u32)(word & 0xffu)
                                        : (rigel_u32)(word >> 8);
        } else if (size == 2u) {
            *value = word;
        } else if (size == 4u) {
            *value = ((rigel_u32)word << 16) |
                     custom_read_word(ctx, addr + 2u);
        } else {
            return RIGEL_MMIO_UNSUPPORTED;
        }
        return RIGEL_MMIO_HANDLED;
    }

    cia_id = cia_id_for_address(addr);
    if (cia_id >= 0) {
        if (size != 1u) {
            return RIGEL_MMIO_UNSUPPORTED;
        }
        *value = rigel_cia_read(ctx, (rigel_u32)cia_id,
                                (rigel_u8)((addr >> 8) & 0x0fu));
        return RIGEL_MMIO_HANDLED;
    }

    if (addr >= RIGEL_RTC_BASE && addr <= RIGEL_RTC_END) {
        if (size != 1u) {
            return RIGEL_MMIO_UNSUPPORTED;
        }
        *value = rigel_rtc_read_reg(ctx, (rigel_u8)((addr >> 2) & 0x0fu));
        return RIGEL_MMIO_HANDLED;
    }

    return RIGEL_MMIO_UNMAPPED;
}

rigel_mmio_result_t rigel_mmio_write(RigelContext *ctx, rigel_u32 addr,
                                     rigel_u8 size, rigel_u32 value)
{
    int cia_id;

    if (ctx == NULL || addr > RIGEL_M68K_ADDRESS_MAX) {
        return RIGEL_MMIO_UNMAPPED;
    }

    if (custom_address(addr)) {
        if ((size == 2u || size == 4u) && (addr & 1u) != 0u) {
            return RIGEL_MMIO_UNSUPPORTED;
        }
        if (size == 4u && !custom_address(addr + 3u)) {
            return RIGEL_MMIO_UNSUPPORTED;
        }

        if (size == 1u) {
            rigel_u16 word = custom_read_word(ctx, addr);
            if ((addr & 1u) != 0u) {
                word = (rigel_u16)((word & 0xff00u) | (value & 0xffu));
            } else {
                word = (rigel_u16)((word & 0x00ffu) |
                                   ((value & 0xffu) << 8));
            }
            custom_write_word(ctx, addr, word);
        } else if (size == 2u) {
            custom_write_word(ctx, addr, (rigel_u16)value);
        } else if (size == 4u) {
            custom_write_word(ctx, addr, (rigel_u16)(value >> 16));
            custom_write_word(ctx, addr + 2u, (rigel_u16)value);
        } else {
            return RIGEL_MMIO_UNSUPPORTED;
        }
        return RIGEL_MMIO_HANDLED;
    }

    cia_id = cia_id_for_address(addr);
    if (cia_id >= 0) {
        if (size != 1u) {
            return RIGEL_MMIO_UNSUPPORTED;
        }
        rigel_cia_write(ctx, (rigel_u32)cia_id,
                        (rigel_u8)((addr >> 8) & 0x0fu), (rigel_u8)value);
        return RIGEL_MMIO_HANDLED;
    }

    if (addr >= RIGEL_RTC_BASE && addr <= RIGEL_RTC_END) {
        if (size != 1u) {
            return RIGEL_MMIO_UNSUPPORTED;
        }
        rigel_rtc_write_reg(ctx, (rigel_u8)((addr >> 2) & 0x0fu),
                            (rigel_u8)(value & 0x0fu));
        return RIGEL_MMIO_HANDLED;
    }

    return RIGEL_MMIO_UNMAPPED;
}

rigel_u16 rigel_custom_read16(RigelContext *ctx, rigel_u32 addr)
{
    return custom_regs_read16(ctx, addr);
}

void rigel_custom_write16(RigelContext *ctx, rigel_u32 addr, rigel_u16 value)
{
    custom_regs_write16(ctx, addr, value);
}
