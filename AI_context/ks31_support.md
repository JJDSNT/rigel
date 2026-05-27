# KS3.1 Support — Gap Analysis

_Data: 2026-05-27_

## Estado actual

O chipset Rigel está funcionalmente completo para os requisitos do KS3.1:
CIA + teclado, serial, disco (DSKSYNC/ADKCON/DMA), copper, blitter, bitplanes,
IRQ, beam/VERTB, BEAMCON0 PAL, VPOSR/VHPOSR, INTENAR/INTREQR.

O que falta **não é o chipset** — é o **harness** (`harness/harness.c`) que
liga o Musashi (68000) ao Rigel. O harness actual foi feito para testes de
timing internos, não para correr uma ROM real.

---

## Lacunas no Harness

### 1. CIA não está wired (crítico)

Os callbacks Musashi (`m68k_read_memory_*`, `m68k_write_memory_*`) não mapeiam
os endereços CIA. O KS3.1 lê/escreve CIA constantemente.

```c
/* A adicionar em m68k_read_memory_8/16 e m68k_write_memory_8/16 */

/* CIA-A: 0xBFExxx, bytes ímpares; CIA-B: 0xBFDxxx, bytes pares */
if (address >= 0xBFD000 && address <= 0xBFEFFF) {
    uint32_t cia_id  = (address & 0x001000) ? 0 : 1;  /* BFExxx=A, BFDxxx=B */
    uint8_t  reg     = (uint8_t)((address >> 8) & 0xF);
    /* leitura: */ return rigel_cia_read(h->rigel, cia_id, reg);
    /* escrita: */ rigel_cia_write(h->rigel, cia_id, reg, (uint8_t)value);
}
```

**Nota de endereçamento:** CIA-A ocupa os bytes ímpares do bus (A0=1), CIA-B
os pares (A0=0). A decodificação correcta usa bit 12 do endereço para distinguir
CIA-A (`0xBFE`) de CIA-B (`0xBFD`).

---

### 2. ROM mirror em $FC0000 (crítico)

O KS3.1 de 512KB está em `$F80000–$FFFFFF`. O CPU faz reset para `$FC0002`
(vector de reset em $FC0000). O harness tem ROM em `$F80000` mas não tem o
mirror em `$FC0000`.

```c
/* Em m68k_read_memory_8/16/32 */
if (address >= 0xFC0000) {
    uint32_t offset = (address - 0xFC0000) + 0x40000; /* mirror F80000+256K */
    if (offset < HARNESS_ROM_SIZE)
        return h->rom[offset]; /* ou read16 equivalente */
}
```

Para uma ROM de 512KB em `$F80000`, o mirror em `$FC0000` cobre os 256KB
superiores (`$F80000+256K = $FC0000`).

---

### 3. Overlay de Chip RAM (crítico)

No reset, o Amiga mapeia ROM em `$0–$7FFFF` (overlay activo). O KS copia
o exception vector table para chip RAM e depois desactiva o overlay escrevendo
o bit 0 de CIA-A PRA. Sem overlay, `$0–$7FFFF` é chip RAM.

```c
/* Estado de overlay no harness */
bool overlay_active = true; /* true = ROM em $0, false = Chip RAM em $0 */

/* Em m68k_read_memory_*: */
if (address < 0x080000) {
    if (h->overlay_active) {
        /* ROM at $0 */
        uint32_t offset = address & (HARNESS_ROM_SIZE - 1);
        return h->rom[offset];
    } else {
        return h->chip_ram[address]; /* Chip RAM at $0 */
    }
}

/* Hook CIA-A PRA write para detectar mudança de overlay */
/* CIA-A PRA bit 0: 1 = overlay desactivado (chip RAM em $0) */
/* Chamado após rigel_cia_write(ctx, 0, CIA_REG_PRA, value) */
void harness_cia_a_pra_hook(harness_t *h, uint8_t pra) {
    h->overlay_active = !(pra & 0x01);
}
```

**Como detectar a escrita a CIA-A PRA:** Após `rigel_cia_write(ctx, 0,
CIA_REG_PRA, value)` no callback Musashi, verificar se `reg == 0` (PRA) e
actualizar o estado de overlay do harness. Alternativa: expor um callback de
notificação de PRA no harness.

---

### 4. Bus stall / chip RAM arbitration (importante)

O harness tem `TODO: honour cpu_would_stall`. O KS3.1 funcionará sem isto
inicialmente, mas o timing das DMA será errado. Implementar após os itens
críticos.

```c
/* Em m68k_read_memory_16 para chip RAM: */
if (address < HARNESS_CHIP_RAM_SIZE) {
    if (!rigel_cpu_can_access_chip_ram(h->rigel)) {
        /* Musashi não tem wait-state granular; alternativa: step Rigel antes */
        rigel_cycle_t resume = rigel_get_cpu_resume_time(h->rigel);
        rigel_step_until(h->rigel, resume);
        m68k_set_irq(rigel_get_ipl(h->rigel));
    }
    return chip_ram_read16(h, address);
}
```

---

### 5. Slow RAM em $C00000 (menor)

O KS3.1 detecta Slow RAM (256KB) em `$C00000–$C7FFFF`. Sem ela, o sistema
funciona mas com menos memória. Adicionar um buffer de 256KB e mapear em
`m68k_read/write_memory_*`.

---

### 6. Carregar ROM de ficheiro real (infra)

O harness actual aceita bytes arbitrários via `harness_load_rom`. Precisa de:

```c
bool harness_load_rom_file(harness_t *h, const char *path);
```

O ficheiro `.rom` é raw big-endian. Para KS3.1 de 512KB, carregar directamente
em `h->rom[]` sem transformação (Musashi lê big-endian por defeito nos
callbacks).

---

## Verificações no Lado Rigel

### CIA-B keyboard handshake

O KS3.1 faz ACK ao teclado togglando CIA-B CRA bit 6 (SPMODE). O CIA em
Rigel implementa SPMODE? Verificar se `cia_write_reg(cia_b, CIA_REG_CRA, ...)`
com bit 6 activa a lógica de ACK no SP pin. Se não, o KS ficará preso à espera
do próximo keycode.

**Acção:** Verificar `src/cia/cia.c` — se SPMODE toggle no CRA não gera o
`/KDAT` de ACK, pode ser necessário simular o handshake: após KS ler SDR e
toggle CRA, reset o estado "pending keystroke" sem exigir pulso físico de ACK.

### DSKBYTR byte-ready (bit 15)

O bit 15 de DSKBYTR indica "novo byte disponível" (set quando DMA escreve
palavra, cleared na leitura). O disco actual seta-o em `disk_load_dskbytr` e
`disk_read_dskbytr` limpa-o. Verificar que KS não fica preso a polling
DSKBYTR[15] para sincronismo — o nosso disco DMA funciona em burst, não
byte-a-byte.

### ADKCON MSBSYNC vs WORDSYNC

KS3.1 usa WORDSYNC (ADKCON bit 10) para sincronismo de disco. Verificar que
o `dsksync` padrão de reset (`$4489`) não afecta a leitura antes da primeira
escrita de DSKSYNC pelo KS.

---

## Sequência de boot KS3.1 (referência)

```
Reset → $FC0002
  1. CPU reset, overlay activo (ROM em $0)
  2. Escreve BEAMCON0 (PAL/NTSC detection)
  3. Lê VPOSR para chip ID e LOF
  4. Configura CIA-A/B timers, ICR
  5. Desactiva overlay (CIA-A PRA bit 0 = 1)
  6. Copia exception vectors para chip RAM $0
  7. Activa INTENA: SETCLR | INTEN | PORTS | VERTB | EXTER | DSKBLK
  8. Configura copper list, activa BPLEN+COPEN+DMAEN
  9. DF0 boot: escreve DSKPTH/L, DSKLEN (x2), aguarda DSKBLK IRQ
 10. Verifica checksum do boot block; executa ou carrega AmigaOS
```

---

## Ordem de implementação recomendada

| # | Item | Ficheiro | Impacto |
|---|---|---|---|
| 1 | CIA read/write nos callbacks Musashi | `harness.c` | crítico — qualquer acesso CIA |
| 2 | ROM mirror $FC0000 | `harness.c` | crítico — CPU não arranca |
| 3 | Overlay chip RAM via CIA-A PRA | `harness.c` | crítico — vectors errados |
| 4 | `harness_load_rom_file()` | `harness.c` | infra para testes reais |
| 5 | Verificar CIA-B keyboard ACK (SPMODE) | `src/cia/cia.c` | necessário para input |
| 6 | Slow RAM $C00000 | `harness.c` | menor — KS funciona sem ela |
| 7 | Bus stall chip RAM | `harness.c` | timing correcto, não bloqueador |
