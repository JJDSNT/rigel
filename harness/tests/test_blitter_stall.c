#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "harness.h"
#include "rigel/rigel_bus.h"

/*
 * Verifica que cpu_would_stall é verdadeiro enquanto o blitter está ocupado
 * com BLTPRI ativo, e falso após a conclusão.
 *
 * TODO: quando o beam estiver modelado e o harness loop for cycle-exact,
 * expandir para verificar que o Musashi não avançou durante o stall.
 */

static bool blitter_done(harness_t *h)
{
    rigel_bus_state_t bus = rigel_get_bus_state(harness_rigel(h));
    return !bus.cpu_would_stall;
}

int main(void)
{
    harness_t *h = harness_create();
    if (h == NULL) return 1;

    /* Programa blitter via custom registers:
     * DMACON: habilita DMA geral (bit 9) + blitter DMA (bit 6) + BLTPRI (bit 10)
     * BLTSIZE com valor mínimo para disparar a operação */
    RigelContext *rigel = harness_rigel(h);
    rigel_custom_write16(rigel, 0x096, 0x8240); /* DMACON: SET + DMAEN + BLTEN */
    rigel_custom_write16(rigel, 0x096, 0x8400); /* DMACON: SET + BLTPRI */
    rigel_custom_write16(rigel, 0x058, 0x0001); /* BLTSIZE: 1 word, dispara */

    /* Stall deve estar ativo agora */
    rigel_bus_state_t bus = rigel_get_bus_state(rigel);
    if (!bus.cpu_would_stall) {
        fprintf(stderr, "FAIL: cpu_would_stall deveria ser true após BLTSIZE write\n");
        harness_destroy(h);
        return 1;
    }

    harness_run(h, blitter_done, 10000);

    bus = rigel_get_bus_state(rigel);
    if (bus.cpu_would_stall) {
        fprintf(stderr, "FAIL: cpu_would_stall deveria ser false após blitter concluir\n");
        harness_destroy(h);
        return 1;
    }

    harness_destroy(h);
    return 0;
}
