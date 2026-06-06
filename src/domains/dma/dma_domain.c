#include "domains/dma/dma_domain.h"
#include "debug/log.h"

#include <stddef.h>

#if RIGEL_ENABLE_STDLIB_ENV
#include <stdio.h>
#include <stdlib.h>
#endif

static bool rigel_dma_trace_enabled(void)
{
#if RIGEL_ENABLE_STDLIB_ENV
    static int enabled = -1;

    if (enabled < 0) {
        const char *env = getenv("RIGEL_DMA_TRACE");
        enabled = (env != NULL && env[0] != '\0' && env[0] != '0') ? 1 : 0;
    }

    return enabled != 0;
#else
    return false;
#endif
}

static void rigel_dma_trace(const char *reason, rigel_u16 old_value,
                            rigel_u16 new_value, rigel_u16 raw_value)
{
#if RIGEL_ENABLE_STDLIB_ENV
    char msg[96];

    if (!rigel_dma_trace_enabled() || old_value == new_value) {
        return;
    }

    snprintf(msg, sizeof(msg),
             "[RIGEL-DMA] %s raw=%04x old=%04x new=%04x",
             reason ? reason : "?",
             (unsigned)raw_value,
             (unsigned)old_value,
             (unsigned)new_value);
    rigel_log_info(msg);
#else
    (void)reason;
    (void)old_value;
    (void)new_value;
    (void)raw_value;
#endif
}

static rigel_u16 rigel_dma_apply_setclr(rigel_u16 current, rigel_u16 value)
{
    rigel_u16 mask = (rigel_u16)(value & 0x7fffU);

    if ((value & RIGEL_SETCLR) != 0) {
        return (rigel_u16)(current | mask);
    }

    return (rigel_u16)(current & (rigel_u16)(~mask));
}

void rigel_dma_domain_reset(dma_state_t *dma)
{
    if (dma == NULL) {
        return;
    }

    dma->dmacon = 0;
    dma->enabled = false;
    dma->blitter_enabled = false;
}

rigel_u16 rigel_dma_domain_read_dmacon(const dma_state_t *dma)
{
    if (dma == NULL) {
        return 0;
    }

    return dma->dmacon;
}

void rigel_dma_domain_write_dmacon(dma_state_t *dma, rigel_u16 value)
{
    rigel_u16 old_value;
    rigel_u16 new_value;

    if (dma == NULL) {
        return;
    }

    old_value = dma->dmacon;
    new_value = rigel_dma_apply_setclr(dma->dmacon, value);
    rigel_dma_trace("write-dmacon", old_value, new_value, value);
    dma_set_dmacon(dma, new_value);
    rigel_dma_domain_sync_dmacon(dma, dma->dmacon);
}

void rigel_dma_domain_sync_dmacon(dma_state_t *dma, rigel_u16 dmacon)
{
    rigel_u16 old_value;

    if (dma == NULL) {
        return;
    }

    old_value = dma->dmacon;
    rigel_dma_trace("sync-dmacon", old_value, dmacon, dmacon);
    dma_set_dmacon(dma, dmacon);
    dma_set_enabled(dma, (dmacon & RIGEL_DMACON_DMAEN) != 0);
    dma->blitter_enabled = (dmacon & RIGEL_DMACON_BLTEN) != 0;
}

rigel_u32 rigel_dma_domain_blitter_grants(const dma_state_t *dma, rigel_u32 cycles)
{
    if (dma == NULL || !dma->enabled || !dma->blitter_enabled) {
        return 0;
    }

    return cycles;
}
