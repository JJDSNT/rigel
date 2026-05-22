/*
 * test_blitter_timing.c
 *
 * Compara tempo esperado versus tempo observado para conclusão do blitter,
 * e exibe o gap em relação ao tempo de referência do hardware real (Amiga OCS).
 *
 * Serve como verificação de consistência interna da API temporal e como
 * documentação explícita do gap entre o modelo atual e o hardware.
 *
 * Modelo atual (provisório):
 *   blitter_estimate_cycles() = W × H palavras
 *   dma_grants = cycles (razão 1:1)
 *   Conclusão em W × H ciclos
 *
 * Hardware real (referência):
 *   Cada palavra requer 1 slot DMA por canal ativo
 *   Cada slot DMA = 2 CCKs
 *   Conclusão em W × H × canais × 2 CCKs
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "harness.h"
#include "rigel/rigel_bus.h"
#include "rigel/rigel_custom.h"
#include "rigel/rigel_events.h"
#include "rigel/rigel_time.h"

/*
 * BLTCON0 = 0x09f0
 *   bits 11–8 = 1001b → USEA=1, USEB=0, USEC=0, USED=1 → canais A e D
 *   bits 15–8 = 0x09  → minterm 0xf0 (D = A)
 */
#define TEST_BLTCON0         0x09f0u
#define TEST_BLTSIZE_W       4u      /* palavras por linha */
#define TEST_BLTSIZE_H       8u      /* linhas */
#define TEST_CHANNELS_ACTIVE 2u      /* A + D */

/* (height << 6) | width */
#define TEST_BLTSIZE_REG  ((TEST_BLTSIZE_H << 6) | TEST_BLTSIZE_W)

/*
 * Tempo de referência do hardware (CCKs) para uma operação com os parâmetros dados.
 * 1 slot DMA por canal por palavra, 2 CCKs por slot.
 */
static uint64_t hw_reference_cycles(uint32_t w, uint32_t h, uint32_t channels)
{
    return (uint64_t)w * h * channels * 2;
}

int main(void)
{
    harness_t       *h;
    RigelContext    *rigel;
    rigel_cycle_t    t_start;
    rigel_cycle_t    t_expected;
    rigel_cycle_t    t_observed;
    uint64_t         t_ref_duration;
    rigel_bus_state_t bus;
    rigel_step_result_t r;
    int64_t          rigel_delta;
    int64_t          reference_gap;
    int              result = 0;

    h = harness_create();
    if (h == NULL) {
        fprintf(stderr, "FAIL: harness_create\n");
        return 1;
    }
    rigel = harness_rigel(h);

    /* Habilita DMA: master (DMAEN) + blitter (BLTEN) */
    rigel_custom_write16(rigel, RIGEL_REG_DMACON,
                         (rigel_u16)(RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BLTEN));

    /* Configura blitter: canais A+D, origem e destino em chip RAM */
    rigel_custom_write16(rigel, 0x040, TEST_BLTCON0); /* BLTCON0               */
    rigel_custom_write16(rigel, 0x042, 0x0000u);      /* BLTCON1               */
    rigel_custom_write16(rigel, 0x044, 0xffffu);      /* BLTAFWM               */
    rigel_custom_write16(rigel, 0x046, 0xffffu);      /* BLTALWM               */
    rigel_custom_write16(rigel, 0x050, 0x0000u);      /* BLTAPTH (src A alto)  */
    rigel_custom_write16(rigel, 0x052, 0x0000u);      /* BLTAPTL (src A baixo) */
    rigel_custom_write16(rigel, 0x054, 0x0000u);      /* BLTDPTH (dst alto)    */
    rigel_custom_write16(rigel, 0x056, 0x0400u);      /* BLTDPTL (dst baixo)   */
    rigel_custom_write16(rigel, 0x064, 0x0000u);      /* BLTAMOD               */
    rigel_custom_write16(rigel, 0x066, 0x0000u);      /* BLTDMOD               */

    t_start = rigel_get_time(rigel);

    /* Disparar: escrita em BLTSIZE inicia a operação */
    rigel_custom_write16(rigel, 0x058, (rigel_u16)TEST_BLTSIZE_REG);

    /* Tempo esperado segundo o modelo atual (bus.next_change) */
    bus = rigel_get_bus_state(rigel);
    t_expected = bus.next_change;

    /* Referência do hardware */
    t_ref_duration = hw_reference_cycles(TEST_BLTSIZE_W, TEST_BLTSIZE_H,
                                         TEST_CHANNELS_ACTIVE);

    /*
     * Avança exatamente até t_expected.
     *
     * Este é o padrão canônico de uso da Temporal API:
     *   1. ler next_change
     *   2. step_until(next_change)
     *   3. checar eventos no resultado
     *
     * Nota: a API atual não resolve o sub-step exato em que o evento ocorreu.
     * Se o step for maior que cycles_remaining, t_observed != t_expected mesmo
     * que BLIT_DONE tenha disparado dentro do intervalo.
     */
    r = rigel_step_until(rigel, t_expected);
    t_observed = rigel_get_time(rigel);

    rigel_delta   = (int64_t)t_observed - (int64_t)t_expected;
    reference_gap = (int64_t)t_ref_duration
                  - (int64_t)(t_observed - t_start);

    /* Relatório */
    printf("=== Blitter timing: expected vs observed ===\n");
    printf("  Operação   : BLTCON0=0x%04x  W=%u palavras  H=%u linhas  "
           "canais=%u\n",
           TEST_BLTCON0, TEST_BLTSIZE_W, TEST_BLTSIZE_H, TEST_CHANNELS_ACTIVE);
    printf("  Palavras   : %u  (W × H)\n", TEST_BLTSIZE_W * TEST_BLTSIZE_H);
    printf("\n");
    printf("  t_start    : %" PRIu64 "\n",  (uint64_t)t_start);
    printf("  Esperado   : %" PRIu64 "  (+%" PRIu64 " ciclos)  "
           "[bus.next_change]\n",
           (uint64_t)t_expected,
           (uint64_t)(t_expected - t_start));
    printf("  Observado  : %" PRIu64 "  (+%" PRIu64 " ciclos)  "
           "[quando BLIT_DONE disparou]\n",
           (uint64_t)t_observed,
           (uint64_t)(t_observed - t_start));
    printf("  Referência : +%" PRIu64 " CCKs  "
           "[hardware Amiga: W×H×canais×2]\n",
           t_ref_duration);
    printf("\n");
    printf("  Rigel delta   : %+" PRId64 " ciclos  "
           "(esperado vs observado — deve ser 0)\n",
           rigel_delta);
    printf("  Reference gap : %+" PRId64 " CCKs  "
           "(observado vs hardware — gap do modelo atual)\n",
           reference_gap);
    printf("\n");

    if (reference_gap > 0) {
        printf("  [!] Modelo atual é %+" PRId64 " CCKs mais rápido que o hardware real.\n"
               "      Causa: estimate = W×H (sem fator slot/CCK e contagem de canais).\n",
               reference_gap);
    } else if (reference_gap < 0) {
        printf("  [!] Modelo atual é %+" PRId64 " CCKs mais lento que o hardware real.\n",
               -reference_gap);
    } else {
        printf("  [ok] Tempo observado coincide com a referência de hardware.\n");
    }

    /* Verificações de correção */
    if (!(r.events & RIGEL_EVENT_BLIT_DONE)) {
        fprintf(stderr, "\nFAIL: RIGEL_EVENT_BLIT_DONE não disparou.\n");
        result = 1;
    }

    if (rigel_delta != 0) {
        fprintf(stderr,
                "\nFAIL: inconsistência interna — observado (%" PRIu64 ") != "
                "esperado (%" PRIu64 "). bus.next_change está errado.\n",
                (uint64_t)t_observed, (uint64_t)t_expected);
        result = 1;
    }

    if (result == 0) {
        printf("PASS: consistência interna OK. "
               "Reference gap documentado acima.\n");
    }

    harness_destroy(h);
    return result;
}
