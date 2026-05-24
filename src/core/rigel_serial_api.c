#include "rigel/rigel_serial.h"

#include "core/rigel_context.h"
#include "chipset/paula/serial.h"

void rigel_serial_receive_byte(RigelContext *ctx, rigel_u8 byte)
{
    if (ctx == NULL) {
        return;
    }

    serial_receive_byte(&ctx->chipset.paula.serial, byte);
}

bool rigel_serial_tx_available(const RigelContext *ctx)
{
    if (ctx == NULL) {
        return false;
    }

    return serial_tx_available(&ctx->chipset.paula.serial);
}

bool rigel_serial_pop_tx_byte(RigelContext *ctx, rigel_u8 *byte_out)
{
    if (ctx == NULL || byte_out == NULL) {
        return false;
    }

    return serial_pop_tx_byte(&ctx->chipset.paula.serial, byte_out);
}
