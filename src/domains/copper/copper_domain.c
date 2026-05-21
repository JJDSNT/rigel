#include "domains/copper/copper_domain.h"

#include <stddef.h>

void rigel_copper_domain_reset(copper_state_t *copper)
{
    copper_reset(copper);
}

void rigel_copper_domain_step(copper_state_t *copper, rigel_u32 cycles)
{
    (void)copper;
    (void)cycles;
}
