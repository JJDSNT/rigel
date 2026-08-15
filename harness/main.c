/*
 * rigel-harness — Musashi + Rigel front-end.
 *
 * Interactive by default: opens an SDL2 window and runs the machine until the
 * window is closed. With --headless it runs to a frame or cycle budget and
 * exits, which is what CI and chipset validation use.
 *
 *   rigel-harness <kickstart.rom> [options]
 *   rigel-harness --exec <program> [options]     run a hunk executable, no ROM
 *
 *     --exec FILE       LoadSeg an AmigaOS hunk executable and run it instead
 *                       of a Kickstart. Emu68's bare-metal test programs work
 *                       this way; give them --fast, they want the memory.
 *     --exec-fb WxH     framebuffer geometry handed to it (default 640x480)
 *     --exec-fb-out F   write that framebuffer as a PPM when the run ends
 *     --adf FILE        insert into DF0 (repeat with --df1/--df2/--df3)
 *     --cpu TYPE        68000 | 68010 | 68ec020 | 68020 | 68030 | 68040
 *     --chip KB         Chip RAM size in KB (default 512)
 *     --slow KB         Slow RAM at 0xC00000 (default 0)
 *     --pal | --ntsc    video standard (default PAL)
 *     --ecs             ECS chipset (default OCS)
 *     --cycle-exact     enable Rigel's honest-hybrid cost model
 *     --trace           let Rigel's structured log events reach stderr
 *     --stop-on-halt    end the run when the PC stops moving (exit code 3);
 *                       off by default, since a delay loop looks the same
 *     --headless        no window
 *     --frames N        stop after N completed frames
 *     --cycles N        stop after N CPU cycles
 *     --scale N         window scale factor (default 2)
 *     --screenshot FILE write the last completed frame as a binary PPM
 *     --iso / --hdf     attach through the LIDE Zorro II board
 *     --lide-rom PATH   board ROM (default external/lide.device/lide.rom)
 *     --odfs PATH       ODFileSystem binary, needed to mount an ISO
 *     --serial MODE     Paula UART output: line (default), raw, off
 *     --serial-slow     pace SERDAT at the programmed baud rate
 *     --log CATS        trace categories: regs,dma,irq,disk,copper,blitter,
 *                       video,cia,all
 *     --status N        one-line machine summary every N frames
 *     --trace-cpu N     keep the last N instructions, disassembled, and dump
 *                       them when the run ends or the CPU wedges
 *     --trace-pc LO:HI  disassemble live while the PC is inside the range
 *     --disasm ADDR[:N] disassemble N instructions at ADDR when the run ends
 *     --break ADDR      stop when the PC reaches ADDR (exit code 4)
 *     --watch ADDR:LEN  report changed bytes in a region, once per frame
 *     --dump ADDR:LEN[:FILE]  dump memory at the end; hex to stdout with no FILE
 *     --screenshot-every N    write a PPM every N frames
 *     --screenshot-dir DIR    where those go (default: current directory)
 *     --key FRAME:CODE[,...]  press an Amiga rawkey at a frame, then release
 *                             it two frames later (Space is 40, Return 44,
 *                             Esc 45, F1 50)
 *     --lmb FRAME[:HOLD]      click the left mouse button, held HOLD frames
 *     --audio-out FILE.wav    write the Paula mix as a 16-bit stereo WAV
 *     --audio-rate HZ         output rate (default 48000)
 *     --no-audio              do not open an audio device in interactive mode
 *
 * Every option also has an environment fallback (KICKSTART, ADF, HARNESS_CPU,
 * ISO, HDF, ...) so the Go launcher in tools/launcher can drive it.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "harness.h"
#include "harness_diag.h"
#include "m68k.h"
#include "rigel/rigel_denise_video.h"
#include "rigel/rigel_input.h"
#include "rigel/rigel_keyboard.h"

#ifdef RIGEL_HARNESS_SDL
#include <SDL.h>
#include "harness_video.h"
#endif

typedef struct options {
    const char *rom;
    const char *adf[4];
    const char *screenshot;
    const char *iso;
    const char *hdf;
    const char *lide_rom;
    const char *exec_file;
    uint32_t    exec_fb_w, exec_fb_h;
    const char *exec_fb_out;
    const char *odfs;
    int         cpu_type;
    uint32_t    chip_kb;
    uint32_t    slow_kb;
    uint32_t    fast_mb;
    bool        ntsc;
    bool        ecs;
    bool        cycle_exact;
    bool        headless;
    bool        trace;
    bool        stop_on_halt;
    const char *serial_mode;
    bool        serial_slow;
    uint32_t    log_flags;
    uint32_t    status_every;
    uint32_t    trace_ring;
    uint32_t    trace_lo, trace_hi;
    uint32_t    disasm_addr, disasm_count;
    uint32_t    break_addr;
    bool        has_break;
    uint32_t    watch_addr, watch_len;
    uint32_t    dump_addr, dump_len;
    const char *dump_file;
    uint32_t    shot_every;
    const char *shot_dir;
    const char *key_script;
    const char *lmb_script;
    const char *audio_out;
    uint32_t    audio_rate;
    bool        no_audio;
    uint64_t    max_frames;
    uint64_t    max_cycles;
    int         scale;
} options_t;

/* ------------------------------------------------------------------------- */

static uint8_t *load_file(const char *path, uint32_t *size_out)
{
    FILE    *fp;
    long     size;
    uint8_t *buf;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "rigel-harness: cannot open %s\n", path);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return NULL; }
    size = ftell(fp);
    if (size <= 0) { fclose(fp); return NULL; }
    rewind(fp);

    buf = (uint8_t *)malloc((size_t)size);
    if (buf == NULL) { fclose(fp); return NULL; }

    if (fread(buf, 1, (size_t)size, fp) != (size_t)size) {
        free(buf);
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    *size_out = (uint32_t)size;
    return buf;
}

static int parse_cpu(const char *name)
{
    if (name == NULL)                    return M68K_CPU_TYPE_68000;
    if (!strcmp(name, "68000"))          return M68K_CPU_TYPE_68000;
    if (!strcmp(name, "68010"))          return M68K_CPU_TYPE_68010;
    if (!strcmp(name, "68ec020"))        return M68K_CPU_TYPE_68EC020;
    if (!strcmp(name, "68020"))          return M68K_CPU_TYPE_68020;
    if (!strcmp(name, "68030"))          return M68K_CPU_TYPE_68030;
    if (!strcmp(name, "68040"))          return M68K_CPU_TYPE_68040;
    fprintf(stderr, "rigel-harness: unknown --cpu %s, using 68000\n", name);
    return M68K_CPU_TYPE_68000;
}

/* An empty environment variable means "unset", which is how the Go launcher
 * writes an option the user left blank. */
static const char *env_or(const char *name, const char *fallback)
{
    const char *v = getenv(name);
    return (v != NULL && v[0] != '\0') ? v : fallback;
}

static bool write_ppm(const char *path, const rigel_frame_t *frame)
{
    FILE *fp;
    uint32_t y, x;

    if (frame->pixels == NULL || frame->width == 0u || frame->height == 0u) {
        fprintf(stderr, "rigel-harness: no frame to write\n");
        return false;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "rigel-harness: cannot write %s\n", path);
        return false;
    }

    fprintf(fp, "P6\n%u %u\n255\n", frame->width, frame->height);
    for (y = 0; y < frame->height; y++) {
        const uint32_t *row = (const uint32_t *)
            ((const uint8_t *)frame->pixels + (size_t)y * frame->pitch);
        for (x = 0; x < frame->width; x++) {
            uint32_t p = row[x];   /* 0x00RRGGBB */
            uint8_t rgb[3];
            rgb[0] = (uint8_t)(p >> 16);
            rgb[1] = (uint8_t)(p >> 8);
            rgb[2] = (uint8_t)(p);
            if (fwrite(rgb, 1, 3, fp) != 3) { fclose(fp); return false; }
        }
    }

    fclose(fp);
    return true;
}

/* -------------------------------------------------------------------------
 * Paula UART
 *
 * AROS and DiagROM write their boot log here, so this is the harness's main
 * window into a machine that is running but not drawing anything.
 * ------------------------------------------------------------------------- */

typedef struct serial_sink {
    bool raw;            /* pass bytes straight through, no framing */
    char line[512];
    size_t len;
    uint64_t bytes;
} serial_sink_t;

static void serial_flush_line(serial_sink_t *s)
{
    if (s->len == 0) return;
    s->line[s->len] = '\0';
    printf("[SERIAL] %s\n", s->line);
    fflush(stdout);
    s->len = 0;
}

static void serial_on_byte(void *opaque, uint8_t byte)
{
    serial_sink_t *s = (serial_sink_t *)opaque;

    s->bytes++;

    if (s->raw) {
        fputc((int)byte, stdout);
        fflush(stdout);
        return;
    }

    if (byte == '\n' || byte == '\r') {
        serial_flush_line(s);
        return;
    }

    /* Keep printable text, tabs and ESC — DiagROM colours its PASS/FAIL with
     * ANSI sequences and they are worth seeing. Everything else is dropped so
     * a stray control byte cannot scramble the terminal. */
    if (byte != '\t' && byte != 0x1B && (byte < 32 || byte > 126)) return;

    if (s->len + 1 >= sizeof(s->line)) serial_flush_line(s);
    s->line[s->len++] = (char)byte;
}

/* -------------------------------------------------------------------------
 * Audio capture
 *
 * Writing a WAV is the only way to check the sound from a headless run, and
 * it is how a test asserts that a game is actually making noise rather than
 * silently doing nothing. The header is patched with real sizes on close.
 * ------------------------------------------------------------------------- */

typedef struct wav_writer {
    FILE    *fp;
    uint64_t frames;      /* stereo frames written */
    uint32_t rate;
    int64_t  peak;        /* largest absolute sample seen */
    double   energy;      /* sum of squares, for the RMS report */
} wav_writer_t;

static void wav_put32(FILE *fp, uint32_t v)
{
    fputc((int)(v & 0xFF), fp);        fputc((int)((v >> 8) & 0xFF), fp);
    fputc((int)((v >> 16) & 0xFF), fp); fputc((int)((v >> 24) & 0xFF), fp);
}

static void wav_put16(FILE *fp, uint16_t v)
{
    fputc((int)(v & 0xFF), fp); fputc((int)((v >> 8) & 0xFF), fp);
}

static bool wav_open(wav_writer_t *w, const char *path, uint32_t rate)
{
    w->fp = fopen(path, "wb");
    if (w->fp == NULL) return false;

    w->frames = 0;
    w->rate = rate;
    w->peak = 0;
    w->energy = 0.0;

    /* Placeholder sizes; wav_close rewrites them. */
    fwrite("RIFF", 1, 4, w->fp); wav_put32(w->fp, 0);
    fwrite("WAVEfmt ", 1, 8, w->fp);
    wav_put32(w->fp, 16);            /* fmt chunk size  */
    wav_put16(w->fp, 1);             /* PCM             */
    wav_put16(w->fp, 2);             /* stereo          */
    wav_put32(w->fp, rate);
    wav_put32(w->fp, rate * 4u);     /* byte rate       */
    wav_put16(w->fp, 4);             /* block align     */
    wav_put16(w->fp, 16);            /* bits per sample */
    fwrite("data", 1, 4, w->fp); wav_put32(w->fp, 0);
    return true;
}

static void wav_write(void *opaque, int16_t left, int16_t right)
{
    wav_writer_t *w = (wav_writer_t *)opaque;
    int64_t a;

    if (w->fp == NULL) return;
    wav_put16(w->fp, (uint16_t)left);
    wav_put16(w->fp, (uint16_t)right);
    w->frames++;

    a = left < 0 ? -(int64_t)left : left;
    if (a > w->peak) w->peak = a;
    a = right < 0 ? -(int64_t)right : right;
    if (a > w->peak) w->peak = a;
    w->energy += (double)left * left + (double)right * right;
}

static void wav_close(wav_writer_t *w, const char *path)
{
    uint32_t data_bytes;

    if (w->fp == NULL) return;

    data_bytes = (uint32_t)(w->frames * 4u);
    fseek(w->fp, 4, SEEK_SET);  wav_put32(w->fp, 36u + data_bytes);
    fseek(w->fp, 40, SEEK_SET); wav_put32(w->fp, data_bytes);
    fclose(w->fp);
    w->fp = NULL;

    {
        double rms = (w->frames > 0)
            ? sqrt(w->energy / (double)(w->frames * 2u)) : 0.0;
        printf("[AUDIO] %s: %llu frames at %u Hz, peak %lld, rms %.1f%s\n",
               path, (unsigned long long)w->frames, w->rate,
               (long long)w->peak, rms,
               w->peak == 0 ? "  (SILENT)" : "");
    }
}

/* -------------------------------------------------------------------------
 * Scripted input
 *
 * A headless run has no keyboard, and plenty of software waits for one before
 * it will show anything — Battle Squadron's loader asks you to press Space,
 * and a Workbench boot wants a mouse. Scripting the input is what makes those
 * reachable from a test.
 * ------------------------------------------------------------------------- */

enum { INPUT_MAX_EVENTS = 32, KEY_HOLD_FRAMES = 2 };

typedef struct input_event {
    uint64_t frame;
    uint8_t  code;      /* Amiga rawkey, or the mouse button index */
    uint32_t hold;      /* frames to hold before releasing */
    bool     is_key;
    bool     pressed;   /* tracks whether the press has been delivered */
} input_event_t;

typedef struct input_script {
    input_event_t events[INPUT_MAX_EVENTS];
    size_t        count;
} input_script_t;

/* "frame:code,frame:code" for keys, "frame[:hold]" for the mouse button. */
static bool input_parse(input_script_t *s, const char *spec, bool is_key)
{
    const char *p = spec;

    while (*p != '\0' && s->count < INPUT_MAX_EVENTS) {
        char *end;
        input_event_t *e = &s->events[s->count];

        e->frame = strtoull(p, &end, 0);
        if (end == p) return false;
        p = end;

        e->is_key = is_key;
        e->code   = 0;
        e->hold   = KEY_HOLD_FRAMES;

        if (*p == ':') {
            unsigned long v = strtoul(p + 1, &end, is_key ? 16 : 0);
            if (end == p + 1) return false;
            if (is_key) e->code = (uint8_t)v; else e->hold = (uint32_t)v;
            p = end;
        } else if (is_key) {
            return false;   /* a key event needs a code */
        }

        s->count++;
        if (*p == ',') p++;
        else if (*p != '\0') return false;
    }

    return true;
}

static void input_tick(input_script_t *s, harness_t *h, uint64_t frame)
{
    RigelContext *ctx = harness_rigel(h);
    size_t i;

    for (i = 0; i < s->count; i++) {
        input_event_t *e = &s->events[i];

        if (!e->pressed && frame == e->frame) {
            e->pressed = true;
            if (e->is_key) {
                rigel_keyboard_inject(ctx, e->code, true);
                printf("[INPUT] frame %llu: key %02x down\n",
                       (unsigned long long)frame, e->code);
            } else {
                rigel_input_set_fire(ctx, 0u, true);
                printf("[INPUT] frame %llu: left button down\n",
                       (unsigned long long)frame);
            }
        } else if (e->pressed && frame == e->frame + e->hold) {
            if (e->is_key) {
                rigel_keyboard_inject(ctx, e->code, false);
                printf("[INPUT] frame %llu: key %02x up\n",
                       (unsigned long long)frame, e->code);
            } else {
                rigel_input_set_fire(ctx, 0u, false);
                printf("[INPUT] frame %llu: left button up\n",
                       (unsigned long long)frame);
            }
        }
    }
}

/* ------------------------------------------------------------------------- */

static bool parse_args(int argc, char **argv, options_t *o)
{
    int i;

    memset(o, 0, sizeof(*o));
    o->rom         = env_or("KICKSTART", NULL);
    o->adf[0]      = env_or("ADF", NULL);
    o->iso         = env_or("ISO", NULL);
    o->hdf         = env_or("HDF", NULL);
    o->lide_rom    = env_or("LIDE_ROM", "external/lide.device/lide.rom");
    o->odfs        = env_or("ODFS", NULL);
    o->cpu_type    = parse_cpu(env_or("HARNESS_CPU", "68000"));
    o->chip_kb     = 512;
    o->slow_kb     = 0;
    o->scale       = 2;
    o->serial_mode = env_or("HARNESS_SERIAL_MODE", "line");
    o->audio_rate  = 48000;
    o->exec_fb_w   = 640;
    o->exec_fb_h   = 480;

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];

#define NEED_VALUE()                                                          \
        do {                                                                  \
            if (i + 1 >= argc) {                                              \
                fprintf(stderr, "rigel-harness: %s needs a value\n", a);      \
                return false;                                                 \
            }                                                                 \
        } while (0)

        if (!strcmp(a, "--exec"))           { NEED_VALUE(); o->exec_file = argv[++i]; }
        else if (!strcmp(a, "--exec-fb-out")) { NEED_VALUE(); o->exec_fb_out = argv[++i]; }
        else if (!strcmp(a, "--exec-fb")) {
            char *sep;
            NEED_VALUE();
            o->exec_fb_w = (uint32_t)strtoul(argv[++i], &sep, 10);
            if (sep == NULL || (*sep != 'x' && *sep != 'X')) {
                fprintf(stderr, "rigel-harness: --exec-fb wants WxH\n");
                return false;
            }
            o->exec_fb_h = (uint32_t)strtoul(sep + 1, NULL, 10);
            if (o->exec_fb_w == 0 || o->exec_fb_h == 0) {
                fprintf(stderr, "rigel-harness: --exec-fb wants non-zero WxH\n");
                return false;
            }
        }
        else if (!strcmp(a, "--adf"))       { NEED_VALUE(); o->adf[0] = argv[++i]; }
        else if (!strcmp(a, "--df1"))       { NEED_VALUE(); o->adf[1] = argv[++i]; }
        else if (!strcmp(a, "--df2"))       { NEED_VALUE(); o->adf[2] = argv[++i]; }
        else if (!strcmp(a, "--df3"))       { NEED_VALUE(); o->adf[3] = argv[++i]; }
        else if (!strcmp(a, "--cpu"))       { NEED_VALUE(); o->cpu_type = parse_cpu(argv[++i]); }
        else if (!strcmp(a, "--chip"))      { NEED_VALUE(); o->chip_kb = (uint32_t)strtoul(argv[++i], NULL, 0); }
        else if (!strcmp(a, "--slow"))      { NEED_VALUE(); o->slow_kb = (uint32_t)strtoul(argv[++i], NULL, 0); }
        else if (!strcmp(a, "--fast"))      { NEED_VALUE(); o->fast_mb = (uint32_t)strtoul(argv[++i], NULL, 0); }
        else if (!strcmp(a, "--frames"))    { NEED_VALUE(); o->max_frames = strtoull(argv[++i], NULL, 0); }
        else if (!strcmp(a, "--cycles"))    { NEED_VALUE(); o->max_cycles = strtoull(argv[++i], NULL, 0); }
        else if (!strcmp(a, "--scale"))     { NEED_VALUE(); o->scale = (int)strtol(argv[++i], NULL, 0); }
        else if (!strcmp(a, "--screenshot")){ NEED_VALUE(); o->screenshot = argv[++i]; }
        else if (!strcmp(a, "--iso"))       { NEED_VALUE(); o->iso = argv[++i]; }
        else if (!strcmp(a, "--hdf"))       { NEED_VALUE(); o->hdf = argv[++i]; }
        else if (!strcmp(a, "--lide-rom"))  { NEED_VALUE(); o->lide_rom = argv[++i]; }
        else if (!strcmp(a, "--odfs"))      { NEED_VALUE(); o->odfs = argv[++i]; }
        else if (!strcmp(a, "--serial"))    { NEED_VALUE(); o->serial_mode = argv[++i]; }
        else if (!strcmp(a, "--serial-slow")) { o->serial_slow = true; }
        else if (!strcmp(a, "--status"))    { NEED_VALUE(); o->status_every = (uint32_t)strtoul(argv[++i], NULL, 0); }
        else if (!strcmp(a, "--trace-cpu")) { NEED_VALUE(); o->trace_ring = (uint32_t)strtoul(argv[++i], NULL, 0); }
        else if (!strcmp(a, "--break")) {
            NEED_VALUE();
            o->break_addr = (uint32_t)strtoul(argv[++i], NULL, 16);
            o->has_break = true;
        }
        else if (!strcmp(a, "--screenshot-every")) { NEED_VALUE(); o->shot_every = (uint32_t)strtoul(argv[++i], NULL, 0); }
        else if (!strcmp(a, "--screenshot-dir"))   { NEED_VALUE(); o->shot_dir = argv[++i]; }
        else if (!strcmp(a, "--key"))       { NEED_VALUE(); o->key_script = argv[++i]; }
        else if (!strcmp(a, "--lmb"))       { NEED_VALUE(); o->lmb_script = argv[++i]; }
        else if (!strcmp(a, "--audio-out")) { NEED_VALUE(); o->audio_out = argv[++i]; }
        else if (!strcmp(a, "--audio-rate")){ NEED_VALUE(); o->audio_rate = (uint32_t)strtoul(argv[++i], NULL, 0); }
        else if (!strcmp(a, "--no-audio"))  { o->no_audio = true; }
        else if (!strcmp(a, "--watch")) {
            char *sep;
            NEED_VALUE();
            o->watch_addr = (uint32_t)strtoul(argv[++i], &sep, 16);
            if (sep == NULL || *sep != ':') {
                fprintf(stderr, "rigel-harness: --watch wants ADDR:LEN in hex\n");
                return false;
            }
            o->watch_len = (uint32_t)strtoul(sep + 1, NULL, 16);
        }
        else if (!strcmp(a, "--dump")) {
            char *sep;
            NEED_VALUE();
            o->dump_addr = (uint32_t)strtoul(argv[++i], &sep, 16);
            if (sep == NULL || *sep != ':') {
                fprintf(stderr, "rigel-harness: --dump wants ADDR:LEN[:FILE] in hex\n");
                return false;
            }
            o->dump_len = (uint32_t)strtoul(sep + 1, &sep, 16);
            if (sep != NULL && *sep == ':') o->dump_file = sep + 1;
        }
        else if (!strcmp(a, "--disasm")) {
            char *sep;
            NEED_VALUE();
            o->disasm_addr = (uint32_t)strtoul(argv[++i], &sep, 16);
            o->disasm_count = (sep != NULL && *sep == ':')
                ? (uint32_t)strtoul(sep + 1, NULL, 0) : 16u;
            if (o->disasm_count == 0) o->disasm_count = 16u;
        }
        else if (!strcmp(a, "--trace-pc")) {
            char *sep;
            NEED_VALUE();
            o->trace_lo = (uint32_t)strtoul(argv[++i], &sep, 16);
            if (sep == NULL || *sep != ':') {
                fprintf(stderr, "rigel-harness: --trace-pc wants LO:HI in hex\n");
                return false;
            }
            o->trace_hi = (uint32_t)strtoul(sep + 1, NULL, 16);
        }
        else if (!strcmp(a, "--log")) {
            const char *bad = NULL;
            NEED_VALUE();
            if (!harness_diag_parse(argv[++i], &o->log_flags, &bad)) {
                fprintf(stderr, "rigel-harness: unknown --log category in '%s'\n"
                                "  known categories: %s\n",
                        bad != NULL ? bad : argv[i], harness_diag_categories());
                return false;
            }
        }
        else if (!strcmp(a, "--pal"))       { o->ntsc = false; }
        else if (!strcmp(a, "--ntsc"))      { o->ntsc = true; }
        else if (!strcmp(a, "--ecs"))       { o->ecs = true; }
        else if (!strcmp(a, "--cycle-exact")) { o->cycle_exact = true; }
        else if (!strcmp(a, "--headless"))  { o->headless = true; }
        else if (!strcmp(a, "--trace"))     { o->trace = true; }
        else if (!strcmp(a, "--stop-on-halt")) { o->stop_on_halt = true; }
        else if (a[0] == '-') {
            fprintf(stderr, "rigel-harness: unknown option %s\n", a);
            return false;
        }
        else o->rom = a;

#undef NEED_VALUE
    }

    if (o->rom == NULL && o->exec_file == NULL) {
        fprintf(stderr,
                "usage: rigel-harness <kickstart.rom> [--adf disk.adf] "
                "[--headless] [--frames N] [--cycles N]\n"
                "       rigel-harness --exec <program> [--fast 8] [options]\n");
        return false;
    }

    /* A screenshot with no budget would never be taken. */
    if (o->screenshot != NULL && o->max_frames == 0u && o->max_cycles == 0u)
        o->max_frames = 1;

    return true;
}

int main(int argc, char **argv)
{
    options_t        o;
    harness_config_t cfg;
    static serial_sink_t s_serial;
    static input_script_t s_input;
    static wav_writer_t   s_wav;
    harness_t       *h;
    uint8_t         *rom;
    uint32_t         rom_size;
    uint64_t         frames = 0;
    bool             halt_reported = false;
    int              rc = 0;
    int              i;
#ifdef RIGEL_HARNESS_SDL
    harness_video_t *video = NULL;
#endif

    if (!parse_args(argc, argv, &o)) return 2;

#ifndef RIGEL_HARNESS_SDL
    if (!o.headless) {
        fprintf(stderr, "rigel-harness: built without SDL2, forcing --headless\n");
        o.headless = true;
    }
#endif

    memset(&cfg, 0, sizeof(cfg));
    cfg.serial_slow      = o.serial_slow;
    cfg.chip_ram_size    = o.chip_kb * 1024u;
    cfg.slow_ram_size    = o.slow_kb * 1024u;
    cfg.cpu_type         = o.cpu_type;
    cfg.video_std        = o.ntsc ? RIGEL_VIDEO_NTSC : RIGEL_VIDEO_PAL;
    cfg.chipset_model    = o.ecs ? RIGEL_CHIPSET_ECS : RIGEL_CHIPSET_OCS;
    cfg.cycle_exact      = o.cycle_exact;
    cfg.trace_events     = o.trace;
    cfg.want_framebuffer = true;

    h = harness_create_ex(&cfg);
    if (h == NULL) {
        fprintf(stderr, "rigel-harness: harness_create_ex failed\n");
        return 1;
    }

    if (o.key_script != NULL && !input_parse(&s_input, o.key_script, true)) {
        fprintf(stderr, "rigel-harness: bad --key script '%s'\n"
                        "  expected FRAME:HEXCODE[,FRAME:HEXCODE...]\n",
                o.key_script);
        rc = 2;
        goto out;
    }
    if (o.lmb_script != NULL && !input_parse(&s_input, o.lmb_script, false)) {
        fprintf(stderr, "rigel-harness: bad --lmb script '%s'\n"
                        "  expected FRAME[:HOLD][,FRAME[:HOLD]...]\n",
                o.lmb_script);
        rc = 2;
        goto out;
    }

#ifdef RIGEL_HARNESS_SDL
    /* An interactive run wants to be heard. The WAV sink wins if both were
     * asked for, since only one sink can be installed. */
    if (!o.headless && !o.no_audio && o.audio_out == NULL) {
        uint32_t got = harness_audio_open(o.audio_rate);
        if (got != 0u) {
            harness_set_audio_sink(h, harness_audio_push, NULL, got);
            printf("rigel-harness: audio out at %u Hz\n", got);
        }
    }
#endif

    if (o.audio_out != NULL) {
        if (wav_open(&s_wav, o.audio_out, o.audio_rate)) {
            harness_set_audio_sink(h, wav_write, &s_wav, o.audio_rate);
            printf("rigel-harness: audio -> %s (%u Hz)\n", o.audio_out, o.audio_rate);
        } else {
            fprintf(stderr, "rigel-harness: cannot write %s\n", o.audio_out);
        }
    }

    harness_diag_attach(h, o.log_flags, o.status_every);
    if (o.has_break) harness_diag_set_breakpoint(o.break_addr, o.cpu_type);
    if (o.watch_len != 0 && !harness_diag_watch(h, o.watch_addr, o.watch_len))
        fprintf(stderr, "rigel-harness: --watch rejected %08x:%x\n",
                o.watch_addr, o.watch_len);
    if (o.trace_ring != 0 || o.trace_hi > o.trace_lo)
        harness_diag_set_cpu_trace(o.trace_ring, o.trace_lo, o.trace_hi,
                                   o.cpu_type);

    if (strcmp(o.serial_mode, "off") != 0) {
        s_serial.raw = (strcmp(o.serial_mode, "raw") == 0);
        harness_set_serial_sink(h, serial_on_byte, &s_serial);
    }

    if (o.fast_mb == 0u && o.exec_file != NULL) {
        /* Emu68's own programs assume a machine with Fast RAM, and the
         * Buddhabrot renderer alone needs 2.6 MB. Default it on rather than
         * failing with an out-of-memory message the user has to decode. */
        o.fast_mb = 8u;
        printf("rigel-harness: --exec implies --fast 8\n");
    }

    if (o.fast_mb != 0u) {
        if (harness_attach_fastram(h, o.fast_mb))
            printf("rigel-harness: Fast RAM %u MB (Zorro II)\n", o.fast_mb);
        else
            fprintf(stderr, "rigel-harness: --fast %u rejected; use 1, 2, 4 or 8\n",
                    o.fast_mb);
    }

    if (o.exec_file != NULL) {
        char hunk_err[256] = "";

        rom = load_file(o.exec_file, &rom_size);
        if (rom == NULL) { harness_destroy(h); return 1; }

        if (!harness_load_hunk(h, rom, rom_size, o.exec_fb_w, o.exec_fb_h,
                               hunk_err, sizeof(hunk_err))) {
            fprintf(stderr, "rigel-harness: cannot load %s: %s\n",
                    o.exec_file, hunk_err[0] ? hunk_err : "not a hunk executable");
            free(rom);
            harness_destroy(h);
            return 1;
        }
        free(rom);
        printf("rigel-harness: exec %s (%u bytes), Chip %u KB, %s %s\n",
               o.exec_file, rom_size, o.chip_kb,
               o.ntsc ? "NTSC" : "PAL", o.ecs ? "ECS" : "OCS");
    } else {
        rom = load_file(o.rom, &rom_size);
        if (rom == NULL) { harness_destroy(h); return 1; }

        if (!harness_load_kickstart(h, rom, rom_size)) {
            fprintf(stderr,
                    "rigel-harness: %s is %u bytes; expected a 256K, 512K or "
                    "1 MB ROM image\n", o.rom, rom_size);
            free(rom);
            harness_destroy(h);
            return 1;
        }
        free(rom);
        printf("rigel-harness: ROM %s (%u KB), Chip %u KB, Slow %u KB, %s %s\n",
               o.rom, rom_size / 1024u, o.chip_kb, o.slow_kb,
               o.ntsc ? "NTSC" : "PAL", o.ecs ? "ECS" : "OCS");
    }

    for (i = 0; i < 4; i++) {
        uint8_t *adf;
        uint32_t adf_size;

        if (o.adf[i] == NULL) continue;
        adf = load_file(o.adf[i], &adf_size);
        if (adf == NULL) { rc = 1; goto out; }
        if (!harness_insert_adf(h, (rigel_floppy_drive_id_t)i, adf, adf_size))
            fprintf(stderr, "rigel-harness: DF%d rejected %s\n", i, o.adf[i]);
        else
            printf("rigel-harness: DF%d <- %s (%u KB)\n", i, o.adf[i], adf_size / 1024u);
        free(adf);
    }

    /* HDF and ISO both hang off the LIDE board, so it only gets presented
     * when there is actually media for it. */
    if (o.hdf != NULL || o.iso != NULL) {
        if (!harness_attach_lide(h, o.lide_rom)) {
            fprintf(stderr,
                    "rigel-harness: cannot read the LIDE ROM at %s\n"
                    "  build it with scripts/build-lide-rom.sh, or point at one "
                    "with --lide-rom\n", o.lide_rom);
            rc = 1;
            goto out;
        }
        printf("rigel-harness: LIDE board, ROM %s\n", o.lide_rom);

        if (o.hdf != NULL) {
            if (harness_attach_hdf(h, o.hdf))
                printf("rigel-harness: HD <- %s\n", o.hdf);
            else
                fprintf(stderr, "rigel-harness: cannot open HDF %s\n", o.hdf);
        }
        if (o.iso != NULL) {
            if (harness_attach_iso(h, o.iso))
                printf("rigel-harness: CD <- %s\n", o.iso);
            else
                fprintf(stderr, "rigel-harness: cannot open ISO %s\n", o.iso);

            if (o.odfs != NULL && !harness_load_odfs(h, o.odfs))
                fprintf(stderr, "rigel-harness: cannot read ODFS %s\n", o.odfs);
            else if (o.odfs == NULL)
                fprintf(stderr, "rigel-harness: no --odfs given; the CD will "
                                "have no filesystem to mount\n");
        }
    }

#ifdef RIGEL_HARNESS_SDL
    if (!o.headless) {
        video = harness_video_open("rigel-harness", o.scale);
        if (video == NULL) { rc = 1; goto out; }
    }
#endif

    for (;;) {
        uint32_t events = harness_step(h, 0u);

        if (events & RIGEL_EVENT_FRAME_READY) {
            frames++;
            harness_diag_frame(h, frames);
            input_tick(&s_input, h, frames);

#ifdef RIGEL_HARNESS_SDL
            if (video != NULL) {
                rigel_frame_t frame;
                if (rigel_get_frame(harness_rigel(h), &frame))
                    harness_video_present(video, &frame);
                if (!harness_video_pump(video, h)) break;
            }
#endif
            if (o.shot_every != 0u && (frames % o.shot_every) == 0u) {
                rigel_frame_t frame;
                char path[512];
                snprintf(path, sizeof(path), "%s/frame_%06llu.ppm",
                         o.shot_dir != NULL ? o.shot_dir : ".",
                         (unsigned long long)frames);
                if (rigel_get_frame(harness_rigel(h), &frame) &&
                    write_ppm(path, &frame))
                    printf("[SHOT] %s (%ux%u)\n", path, frame.width, frame.height);
            }

            if (o.max_frames != 0u && frames >= o.max_frames) break;
        }

        if (harness_diag_breakpoint_hit(NULL)) {
            rc = 4;
            break;
        }

        if (o.max_cycles != 0u && harness_cpu_cycles(h) >= o.max_cycles) break;

        /*
         * A frozen PC is not proof of a wedge: `dbra Dn,*` is an ordinary
         * delay loop and looks identical from outside, which is how Battle
         * Squadron's title screen used to end a run early. Report the
         * suspicion once and keep going; --stop-on-halt is there for a caller
         * that would rather have the run end.
         */
        if (!halt_reported && harness_cpu_halted(h)) {
            halt_reported = true;
            fprintf(stderr,
                    "rigel-harness: CPU has not moved from PC=%08x for a while "
                    "(%llu cycles / %llu frames). This is a delay loop as often "
                    "as it is a hang%s\n",
                    m68k_get_reg(NULL, M68K_REG_PC),
                    (unsigned long long)harness_cpu_cycles(h),
                    (unsigned long long)frames,
                    o.stop_on_halt ? "." : "; still running.");
            if (o.stop_on_halt) {
                harness_diag_dump_trace();
                rc = 3;
                break;
            }
        }
    }

    serial_flush_line(&s_serial);
    if (o.audio_out != NULL) wav_close(&s_wav, o.audio_out);
    if (rc != 3) harness_diag_dump_trace();
    if (o.disasm_count != 0)
        harness_diag_disasm(h, o.disasm_addr, o.disasm_count, o.cpu_type);
    if (o.dump_len != 0)
        harness_diag_dump_mem(h, o.dump_addr, o.dump_len, o.dump_file);

    if (o.exec_fb_out != NULL) {
        uint32_t fb, fw, fh, fpitch;
        if (harness_exec_framebuffer(h, &fb, &fw, &fh, &fpitch)) {
            FILE *fp = fopen(o.exec_fb_out, "wb");
            if (fp == NULL) {
                fprintf(stderr, "rigel-harness: cannot write %s\n", o.exec_fb_out);
            } else {
                uint32_t y, x;
                uint64_t nonzero = 0;
                fprintf(fp, "P6\n%u %u\n255\n", fw, fh);
                for (y = 0; y < fh; y++) {
                    for (x = 0; x < fw; x++) {
                        uint32_t a = fb + y * fpitch + x * 2u;
                        /* Emu68's examples byte-swap before storing (the
                         * `ror.w #8` in their pixel packer), so the buffer
                         * holds little-endian RGB565. */
                        uint16_t p = (uint16_t)(harness_peek8(h, a) |
                                                (harness_peek8(h, a + 1u) << 8));
                        uint8_t rgb[3];
                        if (p != 0) nonzero++;
                        /* RGB565 -> 8 bits per channel, replicating the high
                         * bits so full-scale stays full-scale. */
                        rgb[0] = (uint8_t)(((p >> 11) & 0x1F) * 255 / 31);
                        rgb[1] = (uint8_t)(((p >> 5)  & 0x3F) * 255 / 63);
                        rgb[2] = (uint8_t)(( p        & 0x1F) * 255 / 31);
                        fwrite(rgb, 1, 3, fp);
                    }
                }
                fclose(fp);
                printf("[HUNK] wrote %s (%ux%u, %llu non-black pixels)\n",
                       o.exec_fb_out, fw, fh, (unsigned long long)nonzero);
            }
        } else {
            fprintf(stderr, "rigel-harness: --exec-fb-out needs --exec\n");
        }
    }
    harness_diag_summary(h, frames);
    printf("rigel-harness: %llu frames, %llu CPU cycles, %llu serial bytes\n",
           (unsigned long long)frames,
           (unsigned long long)harness_cpu_cycles(h),
           (unsigned long long)s_serial.bytes);

    if (o.screenshot != NULL) {
        rigel_frame_t frame;
        if (rigel_get_frame(harness_rigel(h), &frame) &&
            write_ppm(o.screenshot, &frame))
            printf("rigel-harness: wrote %s (%ux%u)\n",
                   o.screenshot, frame.width, frame.height);
        else
            rc = rc ? rc : 1;
    }

out:
#ifdef RIGEL_HARNESS_SDL
    if (video != NULL) harness_video_close(video);
    harness_audio_close();
#endif
    harness_destroy(h);
    return rc;
}
