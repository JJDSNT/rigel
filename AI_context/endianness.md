# Endianness

## Status

A biblioteca deve funcionar em qualquer byte order. O host (Bellatrix / Raspberry Pi
big-endian, ou x86 little-endian) escolhe seu endianness; Rigel não deve impor nenhum.

## Paths já corretos

**Chip RAM callbacks** — o contrato é: `read16` devolve um `uint16_t` host-endian
que representa a palavra Amiga naquele endereço. A montagem dos bytes é responsabilidade
do host. Rigel nunca assume byte order ao receber ou enviar esse valor.

**Blitter** — acessa chip RAM exclusivamente via `chip_read16`/`chip_write16`.
Opera em `uint16_t` host-endian. Sem acesso byte-a-byte de valores multi-byte.

**Disk / MFM** — todo I/O de bytes usa helpers explícitos:
- `disk_read_be16(uint8_t *src)` — monta big-endian explicitamente
- `write_be32(uint8_t *p, uint32_t v)` — escreve big-endian explicitamente
- `read_be32(const uint8_t *p)` — lê big-endian explicitamente

**Custom registers** — tudo via `rigel_u16` host-endian. Sem byte manipulation.

**Sem union tricks, sem bitfields, sem packed structs.**

## Problema conhecido: snapshot cross-platform

`rigel_save_state` / `rigel_load_state` fazem `memcpy` raw de `rigel_snapshot_t`:

```c
typedef struct rigel_snapshot {
    rigel_u64 cycles;   /* 8 bytes */
    rigel_u16 intreq;   /* 2 bytes */
    rigel_u16 intena;   /* 2 bytes */
} rigel_snapshot_t;
```

Um snapshot salvo em little-endian e carregado em big-endian (ou vice-versa)
terá todos os campos byte-swapped. Para uso no mesmo host, não é problema.

**Fix necessário:** serialização explícita com byte order fixo (big-endian / network
order, por ser o byte order nativo do Amiga e do Pi big-endian):

```c
static void write_u16_be(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v);
}

static void write_u64_be(uint8_t *p, uint64_t v) {
    for (int i = 7; i >= 0; i--) { p[i] = (uint8_t)(v & 0xFFu); v >>= 8; }
}

static uint16_t read_u16_be(const uint8_t *p) {
    return (uint16_t)((p[0] << 8) | p[1]);
}

static uint64_t read_u64_be(const uint8_t *p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) { v = (v << 8) | p[i]; }
    return v;
}
```

Substituir o `memcpy` por serialização/deserialização explícita usando esses helpers.
O formato do buffer resultante seria estável entre hosts.

## Verificação

`harness/tests/test_endian.c` verifica em runtime:
1. Byte order do host (compile-time e runtime)
2. Round-trip de 16 bits via Chip RAM callbacks
3. Blitter copy preserva bytes no destino
4. Round-trip de custom register
5. Snapshot single-host (documenta a limitação cross-platform)

O teste passa em little-endian e deve passar em big-endian sem modificações.
Para confirmar em big-endian, executar no Raspberry Pi com o harness compilado
nativamente em big-endian.
