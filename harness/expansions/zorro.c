#include "zorro.h"

#include <string.h>

void zorro_autoconfig_build(uint8_t *out, const uint8_t *bytes)
{
    unsigned i;

    memset(out, 0x00, ZORRO_AC_DATA_SIZE);

    for (i = 0; i < ZORRO_AC_ROM_BYTES; i++) {
        uint8_t b = (i == 0) ? bytes[i] : (uint8_t)~bytes[i];
        out[i * 4u]      = (uint8_t)(b & 0xF0u);
        out[i * 4u + 2u] = (uint8_t)((b & 0x0Fu) << 4);
    }
}
