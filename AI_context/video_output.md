# Saída de vídeo — arquitetura em camadas

## Divisão de responsabilidades

**Rigel (chipset) produz vídeo clássico correto:**
- Bitplane DMA fetch + modulos
- BPLCONx (modo, resolução, dual playfield)
- HAM / EHB
- Sprites (incluindo priority e collision)
- Palette (COLOR00–COLOR31 / AGA 256)
- Saída raster linha a linha
- Colisão sprite/playfield (CLXDAT/CLXCON)

**Host apresenta vídeo:**
- pixels Rigel → framebuffer / SDL / OpenGL / Pi
- scale, aspect ratio, vsync real
- RGB565 / RGBA8888 final para hardware
- overlay/debug UI

Planar→chunky **não é conversão de formato** — é a materialização do que Denise realmente faz
durante o DMA slot sequence. Tirar isso do chipset seria tirar Denise do Rigel.

---

## API de saída

```c
typedef enum rigel_pixel_format {
    RIGEL_PIXEL_INDEXED_8BIT,    /* índice 0-31 (OCS) / 0-255 (AGA) — pré-palette */
    RIGEL_PIXEL_RGBA8888,        /* MVP: pronto para host copiar */
    RIGEL_PIXEL_RGB565
} rigel_pixel_format_t;

typedef struct rigel_frame {
    uint32_t              width;
    uint32_t              height;
    uint32_t              pitch;       /* bytes por linha */
    uint64_t              frame_count; /* detectar frames perdidos */
    rigel_pixel_format_t  format;
    const void           *pixels;     /* válido até o próximo rigel_step */
} rigel_frame_t;

const rigel_frame_t     *rigel_get_frame(const RigelContext *ctx);
const rigel_scanline_t  *rigel_get_scanline(const RigelContext *ctx, uint32_t y);
```

O formato é configurado em `rigel_create` via `rigel_config_t`, não por frame —
permite que o chipset otimize o pipeline interno para um único target.

---

## Lifetime do buffer

Rigel possui o buffer de frame. A regra: `pixels` é válido entre `RIGEL_EVENT_FRAME_READY`
e o próximo `rigel_step` que avança além do início do próximo frame. Host que precisa de
zero-copy deve copiar dentro desse janela, ou usar double buffering (futuro: write target
via config callback).

---

## Por que `RIGEL_PIXEL_INDEXED_8BIT` importa

É a saída natural de Denise antes de aplicar as cores do COLOR register.
Hosts que querem controle total da palette (shader, HDR, color grading, análise de debug)
precisam desse formato. Mais útil do que um hipotético `INDEXED_12BIT` porque o índice
chunky já foi resolvido pelo priority engine.

---

## Por que `rigel_get_scanline` importa além de conveniência

Efeitos copper-synchronous mudam palette ou resolução no meio do frame.
Um host que só recebe o frame completo não consegue reproduzir isso corretamente
sem reconstruir o raster a partir do estado do copper. `rigel_get_scanline` expõe
a saída linha a linha sem que o host precise saber do copper.

Também é o que permite interlace real: campos par/ímpar ficam acessíveis
individualmente sem post-process no host.

---

## Delta frame e metadata

Dois conceitos distintos que se complementam:

### Dirty scanlines (update eficiente)

Para hosts com framebuffer físico (Pi, USB display) que não querem empurrar o
frame completo quando a maioria das linhas não mudou.

```c
typedef struct rigel_frame_delta {
    uint64_t dirty_lines[4]; /* bitmask 1 bit/linha, cobre 256 linhas */
    bool     full_redraw;    /* palette global mudou, resolução trocou, etc. */
} rigel_frame_delta_t;
```

`full_redraw = true` quando algo invalida o frame inteiro (copper escreveu COLOR00
na linha 0, BPLCONx mudou resolução, etc.).

Implementação correta: Denise marca linhas durante geração (linha com DMA ativo =
dirty), não por comparação com frame anterior. Comparação pós-geração exige guardar
o frame anterior — pior.

### Frame flags (metadata para decisão do host)

O que aconteceu naquele frame que o host precisa para decidir como apresentar:

```c
typedef enum rigel_frame_flags {
    RIGEL_FRAME_INTERLACE_ODD   = 1 << 0,
    RIGEL_FRAME_INTERLACE_EVEN  = 1 << 1,
    RIGEL_FRAME_COPPER_ACTIVE   = 1 << 2, /* copper escreveu registros mid-frame */
    RIGEL_FRAME_SPRITES_ACTIVE  = 1 << 3,
    RIGEL_FRAME_HAM             = 1 << 4,
    RIGEL_FRAME_DUAL_PLAYFIELD  = 1 << 5
} rigel_frame_flags_t;
```

Host de deinterlace precisa saber o campo. Host com color grading precisa saber
se HAM estava ativo. Host simples ignora tudo.

### Frame struct completo

```c
typedef struct rigel_frame {
    uint32_t             width;
    uint32_t             height;
    uint32_t             pitch;
    uint64_t             frame_count;  /* detectar frames perdidos */
    rigel_pixel_format_t format;
    const void          *pixels;
    rigel_frame_flags_t  flags;        /* o que aconteceu neste frame */
    rigel_frame_delta_t  delta;        /* o que mudou vs. frame anterior */
} rigel_frame_t;
```

Rigel já tem toda essa informação internamente — flags saem do estado do chipset
no fim do frame, dirty lines saem do pipeline de Denise durante geração.
Custo adicional: zero se Denise marcar linhas on-the-fly.

## Roadmap

```
MVP:
  - rigel_config_t aceita rigel_pixel_format_t (default RGBA8888)
  - Rigel entrega frame completo em RGBA8888 após RIGEL_EVENT_FRAME_READY
  - rigel_get_frame() retorna ponteiro para buffer interno

Depois:
  - rigel_get_scanline() para hosts copper-aware e interlace
  - RIGEL_PIXEL_INDEXED_8BIT para hosts com pipeline de cor próprio
  - Write target via config (zero-copy para Pi/PiStorm streaming)
```

---

## O que NÃO entra no Rigel

```c
rigel_planar_to_chunky_for_sdl()  /* NÃO */
rigel_scale_2x()                  /* NÃO */
rigel_apply_scanline_filter()     /* NÃO */
```

Qualquer função que menciona o host de destino fica no host.
Rigel não sabe o que é SDL, OpenGL, ou Pi.
