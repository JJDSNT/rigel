#include "domains/blitter/blitter_domain.h"

bool rigel_blitter_domain_owns_reg(rigel_u32 addr)
{
    switch (addr) {
    case 0x000:
    case 0x002:
    case 0x040:
    case 0x042:
    case 0x044:
    case 0x046:
    case 0x048:
    case 0x04a:
    case 0x04c:
    case 0x04e:
    case 0x050:
    case 0x052:
    case 0x054:
    case 0x056:
    case 0x058:
    case 0x05A:  /* ECS BLTCON0L */
    case 0x05C:  /* ECS BLTSIZV */
    case 0x05E:  /* ECS BLTSIZH */
    case 0x060:
    case 0x062:
    case 0x064:
    case 0x066:
    case 0x070:
    case 0x072:
    case 0x074:
        return true;
    default:
        return false;
    }
}

void rigel_blitter_domain_reset(BlitterState *blitter)
{
    blitter_reset(blitter);
}

rigel_u16 rigel_blitter_domain_read_reg(const BlitterState *blitter, rigel_u32 addr)
{
    return blitter_read_reg16(blitter, addr);
}

void rigel_blitter_domain_write_reg(BlitterState *blitter, rigel_u32 addr, rigel_u16 value)
{
    blitter_write_reg16(blitter, addr, value);
}

void rigel_blitter_domain_step_dma(
    BlitterState *blitter,
    BlitterMemory memory,
    BlitterIrqSink irq,
    rigel_u32 dma_slots
)
{
    if (blitter == NULL || dma_slots == 0) {
        return;
    }

    blitter_step_dma(blitter, memory, irq, dma_slots);
}
