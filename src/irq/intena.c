#include "irq/intena.h"

riegel_u16 intena_apply_write(riegel_u16 current, riegel_u16 value)
{
    riegel_u16 mask = (riegel_u16)(value & 0x7fffU);

    if ((value & 0x8000U) != 0) {
        return (riegel_u16)(current | mask);
    }

    return (riegel_u16)(current & (riegel_u16)(~mask));
}
