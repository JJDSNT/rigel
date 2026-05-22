# Video Output

## Divisão de responsabilidades

**Rigel produz vídeo clássico correto.** Host apresenta vídeo.

```
Rigel/chipset:                    Host:
  bitplane DMA fetch                pixels Rigel → framebuffer
  BPLCONx (modo, resolução)         scale, aspect ratio
  dual playfield                    vsync real
  HAM / EHB                         RGB565 / RGBA8888 para hardware
  sprites (priority, collision)     overlay / debug UI
  palette (COLOR00–COLOR31)
  saída raster linha a linha
```

Planar→chunky não é conversão de formato — é a materialização do que Denise
realmente faz durante o DMA slot sequence. Essa lógica pertence ao chipset.

## API de saída

```c
typedef enum rigel_pixel_format {
    RIGEL_PIXEL_INDEXED_8BIT,   /* índice 0–31 (OCS) / 0–255 (AGA) — pré-palette */
    RIGEL_PIXEL_RGBA8888,       /* MVP: pronto para o host copiar */
    RIGEL_PIXEL_RGB565
} rigel_pixel_format_t;

typedef struct rigel_frame_delta {
    uint64_t dirty_lines[4];  /* bitmask 1 bit/linha — cobre 256 linhas */
    bool     full_redraw;     /* palette global mudou, resolução trocou, etc. */
} rigel_frame_delta_t;

typedef enum rigel_frame_flags {
    RIGEL_FRAME_INTERLACE_ODD   = 1 << 0,
    RIGEL_FRAME_INTERLACE_EVEN  = 1 << 1,
    RIGEL_FRAME_COPPER_ACTIVE   = 1 << 2,
    RIGEL_FRAME_SPRITES_ACTIVE  = 1 << 3,
    RIGEL_FRAME_HAM             = 1 << 4,
    RIGEL_FRAME_DUAL_PLAYFIELD  = 1 << 5
} rigel_frame_flags_t;

typedef struct rigel_frame {
    uint32_t              width;
    uint32_t              height;
    uint32_t              pitch;
    uint64_t              frame_count;   /* detectar frames perdidos */
    rigel_pixel_format_t  format;
    const void           *pixels;
    rigel_frame_flags_t   flags;         /* o que aconteceu neste frame */
    rigel_frame_delta_t   delta;         /* quais linhas mudaram */
} rigel_frame_t;

const rigel_frame_t    *rigel_get_frame(const RigelContext *ctx);
const rigel_scanline_t *rigel_get_scanline(const RigelContext *ctx, uint32_t y);
```

Current implemented baseline:

```c
bool rigel_denise_get_current_scanline(const RigelContext *ctx,
                                       rigel_denise_scanline_t *scanline);
```

Hoje o projeto expõe a linha atual montada por Denise para inspeção e teste
headless. Isso já permite validar efeitos dirigidos por copper e palette sem
backend gráfico, enquanto a API de frame/scanline completa ainda amadurece.

O formato é configurado em `rigel_create` via `rigel_config_t` — não por frame.
Isso permite que o chipset otimize seu pipeline interno para um único target.

## Lifetime do buffer

`pixels` é válido entre `RIGEL_EVENT_FRAME_READY` e o próximo `rigel_step` que
avança além do início do próximo frame. O host deve copiar dentro dessa janela,
ou usar o ponteiro diretamente se não houver concorrência.

## Dirty lines

`delta.dirty_lines` é um bitmask de quais linhas tiveram saída diferente do frame
anterior. Implementação correta: Denise marca linhas durante a geração (linha com
DMA ativo = dirty), não por comparação pós-geração com o frame anterior.

`delta.full_redraw = true` quando algo invalida o frame inteiro — troca de resolução
via BPLCONx, copper escreveu COLOR00 na linha 0, etc. Nesse caso o host deve
redesenhar tudo, independente do bitmask.

## Por que `rigel_get_scanline` importa

Efeitos copper-synchronous mudam palette ou resolução no meio do frame. Um host que
só recebe o frame completo não consegue reproduzir isso sem reconstruir o raster.
`rigel_get_scanline` expõe a saída linha a linha para hosts copper-aware.

É também o caminho correto para interlace: campos par/ímpar ficam acessíveis
individualmente sem post-process no host.

## `RIGEL_PIXEL_INDEXED_8BIT`

É a saída natural de Denise antes de aplicar os registradores COLOR. Hosts que
querem controle total da palette (shader, HDR, análise de debug) precisam desse
formato. Mais útil do que um `INDEXED_12BIT` porque o índice chunky já foi
resolvido pelo priority engine de sprites e playfields.

## O que não entra no Rigel

```c
rigel_planar_to_chunky_for_sdl()   /* não */
rigel_scale_2x()                   /* não */
rigel_apply_scanline_filter()      /* não */
```

Qualquer função que menciona o host de destino pertence ao host.

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
  - Double buffering interno
```
