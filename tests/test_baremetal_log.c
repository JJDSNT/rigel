#include "rigel/rigel.h"
#include "agnus/copper/copper_exec.h"

typedef struct log_probe {
    unsigned count;
    rigel_log_event_id_t last_id;
    rigel_u32 last_reg;
    rigel_u32 last_value;
} log_probe_t;

static void event_sink(const rigel_log_event_t *event, void *opaque)
{
    log_probe_t *probe = (log_probe_t *)opaque;

    if (probe == 0 || event == 0) {
        return;
    }

    probe->count++;
    probe->last_id = event->id;
    if (event->field_count >= 2u) {
        probe->last_reg = event->fields[0];
        probe->last_value = event->fields[1];
    }
}

int main(void)
{
    rigel_config_t cfg = { 0 };
    log_probe_t probe = { 0 };
    RigelContext *ctx;

    cfg.log_event_fn = event_sink;
    cfg.log_event_opaque = &probe;

    ctx = rigel_create(&cfg);
    if (ctx == 0) {
        return 1;
    }

    copper_exec_move(ctx, 0x0096u, 0x8123u);

    if (probe.count != 1u ||
        probe.last_id != RIGEL_LOG_EVENT_COPPER_WRITE ||
        probe.last_reg != 0x0096u ||
        probe.last_value != 0x8123u) {
        rigel_destroy(ctx);
        return 1;
    }

    rigel_destroy(ctx);
    return 0;
}
