#include "irq/intreq.h"

rigel_u16 intreq_apply_write(rigel_u16 current, rigel_u16 value)
{
    rigel_u16 mask = (rigel_u16)(value & 0x7fffU);

    if ((value & 0x8000U) != 0) {
        return (rigel_u16)(current | mask);
    }

    return (rigel_u16)(current & (rigel_u16)(~mask));
}
