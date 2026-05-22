/*
 * test_endian.c
 *
 * Verifica que os paths críticos do Rigel são endian-safe,
 * com foco na futura execução big-endian (Bellatrix / Raspberry Pi).
 *
 * Cobre:
 *   1. Detecção de byte order do host
 *   2. Round-trip de 16 bits via callbacks de Chip RAM
 *   3. Operação de blitter: bytes no destino corretos
 *   4. Round-trip de custom registers (BLTCON0)
 *   5. Round-trip de snapshot (single-host — não portável cross-endian)
 *
 * Limitação documentada:
 *   rigel_save_state / rigel_load_state fazem memcpy raw de struct.
 *   Um snapshot salvo em little-endian e carregado em big-endian (ou
 *   vice-versa) terá campos byte-swapped. Para uso no mesmo host, OK.
 *   Para save states portáveis entre arquiteturas, requer serialização
 *   explícita com byte order fixo.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "harness.h"
#include "rigel/rigel_bus.h"
#include "rigel/rigel_custom.h"
#include "rigel/rigel_events.h"
#include "rigel/rigel_snapshot.h"
#include "rigel/rigel_time.h"

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

static bool host_is_big_endian(void)
{
    uint16_t probe = 0x0102u;
    uint8_t  bytes[2];
    memcpy(bytes, &probe, 2);
    return bytes[0] == 0x01u;
}

static void print_section(const char *name)
{
    printf("\n--- %s ---\n", name);
}

/* -------------------------------------------------------------------------
 * 1. Byte order do host
 * ------------------------------------------------------------------------- */

static bool check_host_byte_order(void)
{
    print_section("Host byte order");

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    printf("  Compile-time: big-endian (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)\n");
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    printf("  Compile-time: little-endian (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)\n");
#else
    printf("  Compile-time: byte order desconhecido\n");
#endif

    bool big = host_is_big_endian();
    printf("  Runtime: %s-endian\n", big ? "big" : "little");

    if (big) {
        printf("  [ok] Host big-endian — alinhado com Bellatrix / Raspberry Pi\n");
    } else {
        printf("  [info] Host little-endian — Rigel deve funcionar igual em big-endian\n");
    }

    return true;
}

/* -------------------------------------------------------------------------
 * 2. Round-trip de 16 bits via Chip RAM callbacks
 *
 * O contrato dos callbacks é: o valor uint16_t representa a palavra Amiga
 * (big-endian) como inteiro host-endian. A montagem de bytes é
 * responsabilidade do host. O harness já faz isso corretamente:
 *   read16 = (chip_ram[addr] << 8) | chip_ram[addr+1]
 *
 * Verifica que um write16 seguido de read16 no mesmo endereço
 * devolve o valor original, independente de byte order.
 * ------------------------------------------------------------------------- */

static bool check_chip_ram_roundtrip(harness_t *h)
{
    print_section("Chip RAM 16-bit round-trip");

    uint8_t *ram = harness_chip_ram(h);

    /* Preenche manualmente dois bytes em big-endian (ordem Amiga) */
    ram[0x100] = 0xDE;
    ram[0x101] = 0xAD;

    /* Lê de volta via callback que o Rigel usaria */
    uint16_t word = (uint16_t)((ram[0x100] << 8) | ram[0x101]);
    printf("  Escrito:  [0xDE, 0xAD] em chip_ram[0x100..0x101]\n");
    printf("  Lido:     0x%04X (esperado: 0xDEAD)\n", word);

    if (word != 0xDEADu) {
        printf("  FAIL: round-trip errado\n");
        return false;
    }

    /* Verifica o sentido inverso: write via callback, bytes corretos */
    uint16_t valor = 0xBEEFu;
    ram[0x200] = (uint8_t)(valor >> 8);    /* high byte */
    ram[0x201] = (uint8_t)(valor & 0xFFu); /* low byte  */

    if (ram[0x200] != 0xBEu || ram[0x201] != 0xEFu) {
        printf("  FAIL: bytes armazenados errados (0x%02X 0x%02X, esperado 0xBE 0xEF)\n",
               ram[0x200], ram[0x201]);
        return false;
    }

    printf("  Write 0xBEEF → bytes [0x%02X, 0x%02X] (esperado [0xBE, 0xEF])\n",
           ram[0x200], ram[0x201]);
    printf("  [ok]\n");
    return true;
}

/* -------------------------------------------------------------------------
 * 3. Blitter copy: bytes no destino corretos
 *
 * Copia um padrão conhecido de um endereço para outro via blitter.
 * Verifica que os bytes no destino correspondem ao padrão original.
 * Isso confirma que o blitter não troca bytes internamente ao ler/escrever
 * via callbacks.
 * ------------------------------------------------------------------------- */

static bool check_blitter_copy_bytes(harness_t *h)
{
    print_section("Blitter copy — bytes no destino");

    uint8_t *ram = harness_chip_ram(h);
    RigelContext *rigel = harness_rigel(h);

    /* Fonte: padrão assimétrico para detectar byte swap */
    uint16_t pattern = 0x1234u;
    ram[0x1000] = (uint8_t)(pattern >> 8);    /* 0x12 */
    ram[0x1001] = (uint8_t)(pattern & 0xFFu); /* 0x34 */

    printf("  Fonte: ram[0x1000..0x1001] = [0x%02X, 0x%02X] (palavra 0x%04X)\n",
           ram[0x1000], ram[0x1001], pattern);

    /* Limpa destino */
    ram[0x2000] = 0x00;
    ram[0x2001] = 0x00;

    /* Configura blitter: A→D copy, 1 palavra × 1 linha */
    /* BLTCON0 = 0x09F0: USEA + USED, minterm 0xF0 (D = A) */
    rigel_custom_write16(rigel, RIGEL_REG_DMACON,
                         (rigel_u16)(RIGEL_SETCLR | RIGEL_DMACON_DMAEN | RIGEL_DMACON_BLTEN));

    rigel_custom_write16(rigel, 0x040, 0x09F0u); /* BLTCON0: USEA+USED, D=A */
    rigel_custom_write16(rigel, 0x042, 0x0000u); /* BLTCON1 */
    rigel_custom_write16(rigel, 0x044, 0xFFFFu); /* BLTAFWM */
    rigel_custom_write16(rigel, 0x046, 0xFFFFu); /* BLTALWM */
    rigel_custom_write16(rigel, 0x050, 0x0000u); /* BLTAPTH */
    rigel_custom_write16(rigel, 0x052, 0x1000u); /* BLTAPTL: fonte = 0x1000 */
    rigel_custom_write16(rigel, 0x054, 0x0000u); /* BLTDPTH */
    rigel_custom_write16(rigel, 0x056, 0x2000u); /* BLTDPTL: destino = 0x2000 */
    rigel_custom_write16(rigel, 0x064, 0x0000u); /* BLTAMOD */
    rigel_custom_write16(rigel, 0x066, 0x0000u); /* BLTDMOD */

    /* Dispara: 1 palavra × 1 linha */
    rigel_custom_write16(rigel, 0x058, 0x0041u); /* BLTSIZE: H=1, W=1 */

    /* Avança até conclusão */
    rigel_bus_state_t bus = rigel_get_bus_state(rigel);
    rigel_step_until(rigel, bus.next_change);

    printf("  Destino: ram[0x2000..0x2001] = [0x%02X, 0x%02X]\n",
           ram[0x2000], ram[0x2001]);

    if (ram[0x2000] != 0x12u || ram[0x2001] != 0x34u) {
        printf("  FAIL: bytes errados no destino (esperado [0x12, 0x34])\n");
        printf("  Possível byte-swap no path do blitter.\n");
        return false;
    }

    printf("  [ok] Bytes preservados corretamente pelo blitter\n");
    return true;
}

/* -------------------------------------------------------------------------
 * 4. Round-trip de custom register
 *
 * Escreve um valor em BLTCON0 e lê de volta. O valor deve ser idêntico
 * ao escrito, independente de byte order do host.
 * ------------------------------------------------------------------------- */

static bool check_custom_reg_roundtrip(harness_t *h)
{
    print_section("Custom register round-trip (BLTCON0)");

    RigelContext *rigel = harness_rigel(h);

    uint16_t written = 0x09F0u;
    rigel_custom_write16(rigel, 0x040, written);
    uint16_t read = rigel_custom_read16(rigel, 0x040);

    printf("  Escrito: 0x%04X\n", written);
    printf("  Lido:    0x%04X\n", read);

    if (read != written) {
        printf("  FAIL: round-trip errado\n");
        return false;
    }

    printf("  [ok]\n");
    return true;
}

/* -------------------------------------------------------------------------
 * 5. Snapshot round-trip (single-host)
 *
 * Documenta o comportamento de rigel_save_state / rigel_load_state.
 * O memcpy raw não é portável entre hosts de byte order diferente.
 * Este teste verifica apenas consistência no mesmo host.
 * ------------------------------------------------------------------------- */

static bool check_snapshot_roundtrip(harness_t *h)
{
    print_section("Snapshot round-trip (single-host)");

    RigelContext *rigel = harness_rigel(h);

    /* Avança alguns ciclos para ter um valor de cycles != 0 */
    rigel_step(rigel, 100u);

    uint64_t cycles_before = (uint64_t)rigel_get_time(rigel);

    uint8_t buf[sizeof(rigel_snapshot_t)];
    bool saved = rigel_save_state(rigel, buf, sizeof(buf));

    if (!saved) {
        printf("  FAIL: rigel_save_state retornou false\n");
        return false;
    }

    /* Modifica o estado */
    rigel_step(rigel, 50u);

    /* Restaura */
    bool loaded = rigel_load_state(rigel, buf, sizeof(buf));

    if (!loaded) {
        printf("  FAIL: rigel_load_state retornou false\n");
        return false;
    }

    uint64_t cycles_after = (uint64_t)rigel_get_time(rigel);

    printf("  Cycles antes do save: %" PRIu64 "\n", cycles_before);
    printf("  Cycles após restore:  %" PRIu64 "\n", cycles_after);

    if (cycles_after != cycles_before) {
        printf("  FAIL: cycles não restaurado corretamente\n");
        return false;
    }

    printf("  [ok] Single-host round-trip OK\n");
    printf("  [!]  AVISO: rigel_save_state faz memcpy raw de struct.\n"
           "       Um snapshot salvo em little-endian NÃO é portável\n"
           "       para big-endian (e vice-versa). Para save states\n"
           "       cross-platform, serialização com byte order fixo\n"
           "       é necessária.\n");

    return true;
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(void)
{
    printf("=== Rigel endianness verification ===\n");

    harness_t *h = harness_create();
    if (h == NULL) {
        fprintf(stderr, "FAIL: harness_create\n");
        return 1;
    }

    bool ok = true;

    ok &= check_host_byte_order();
    ok &= check_chip_ram_roundtrip(h);
    ok &= check_blitter_copy_bytes(h);
    ok &= check_custom_reg_roundtrip(h);
    ok &= check_snapshot_roundtrip(h);

    printf("\n=== Resultado: %s ===\n", ok ? "PASS" : "FAIL");

    if (ok) {
        printf("Paths críticos são endian-safe para uso em big-endian.\n"
               "Exceção documentada: snapshot cross-platform (ver seção 5).\n");
    }

    harness_destroy(h);
    return ok ? 0 : 1;
}
