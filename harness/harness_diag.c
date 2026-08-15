#include "harness_diag.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m68k.h"
#include "rigel/rigel_bus.h"
#include "rigel/rigel_denise_video.h"
#include "rigel/rigel_irq.h"
#include "rigel/rigel_time.h"

/* -------------------------------------------------------------------------
 * Register names
 * ------------------------------------------------------------------------- */

typedef struct reg_info {
    uint16_t     reg;
    const char  *name;
    uint32_t     category;
} reg_info_t;

/* The registers worth naming in a trace. Anything absent still logs by number
 * under HARNESS_DIAG_REGS. */
static const reg_info_t k_regs[] = {
    { 0x000, "BLTDDAT",  HARNESS_DIAG_BLITTER },
    { 0x002, "DMACONR",  HARNESS_DIAG_DMA     },
    { 0x004, "VPOSR",    0                    },
    { 0x006, "VHPOSR",   0                    },
    { 0x008, "DSKDATR",  HARNESS_DIAG_DISK    },
    { 0x00A, "JOY0DAT",  0                    },
    { 0x00C, "JOY1DAT",  0                    },
    { 0x010, "ADKCONR",  0                    },
    { 0x016, "POTGOR",   0                    },
    { 0x018, "SERDATR",  0                    },
    { 0x01A, "DSKBYTR",  HARNESS_DIAG_DISK    },
    { 0x01C, "INTENAR",  HARNESS_DIAG_IRQ     },
    { 0x01E, "INTREQR",  HARNESS_DIAG_IRQ     },
    { 0x020, "DSKPTH",   HARNESS_DIAG_DISK    },
    { 0x022, "DSKPTL",   HARNESS_DIAG_DISK    },
    { 0x024, "DSKLEN",   HARNESS_DIAG_DISK    },
    { 0x02A, "VPOSW",    0                    },
    { 0x02C, "VHPOSW",   0                    },
    { 0x02E, "COPCON",   HARNESS_DIAG_COPPER  },
    { 0x030, "SERDAT",   0                    },
    { 0x032, "SERPER",   0                    },
    { 0x034, "POTGO",    0                    },
    { 0x038, "STREQU",   0                    },
    { 0x03E, "STRLONG",  0                    },
    { 0x040, "BLTCON0",  HARNESS_DIAG_BLITTER },
    { 0x042, "BLTCON1",  HARNESS_DIAG_BLITTER },
    { 0x044, "BLTAFWM",  HARNESS_DIAG_BLITTER },
    { 0x046, "BLTALWM",  HARNESS_DIAG_BLITTER },
    { 0x048, "BLTCPTH",  HARNESS_DIAG_BLITTER },
    { 0x04C, "BLTBPTH",  HARNESS_DIAG_BLITTER },
    { 0x050, "BLTAPTH",  HARNESS_DIAG_BLITTER },
    { 0x054, "BLTDPTH",  HARNESS_DIAG_BLITTER },
    { 0x058, "BLTSIZE",  HARNESS_DIAG_BLITTER },
    { 0x064, "BLTAMOD",  HARNESS_DIAG_BLITTER },
    { 0x066, "BLTDMOD",  HARNESS_DIAG_BLITTER },
    { 0x07C, "DENISEID", 0                    },
    { 0x07E, "DSKSYNC",  HARNESS_DIAG_DISK    },
    { 0x080, "COP1LCH",  HARNESS_DIAG_COPPER  },
    { 0x082, "COP1LCL",  HARNESS_DIAG_COPPER  },
    { 0x084, "COP2LCH",  HARNESS_DIAG_COPPER  },
    { 0x086, "COP2LCL",  HARNESS_DIAG_COPPER  },
    { 0x088, "COPJMP1",  HARNESS_DIAG_COPPER  },
    { 0x08A, "COPJMP2",  HARNESS_DIAG_COPPER  },
    { 0x08E, "DIWSTRT",  HARNESS_DIAG_VIDEO   },
    { 0x090, "DIWSTOP",  HARNESS_DIAG_VIDEO   },
    { 0x092, "DDFSTRT",  HARNESS_DIAG_VIDEO   },
    { 0x094, "DDFSTOP",  HARNESS_DIAG_VIDEO   },
    { 0x096, "DMACON",   HARNESS_DIAG_DMA     },
    { 0x09A, "INTENA",   HARNESS_DIAG_IRQ     },
    { 0x09C, "INTREQ",   HARNESS_DIAG_IRQ     },
    { 0x09E, "ADKCON",   HARNESS_DIAG_DISK    },
    { 0x0E0, "BPL1PTH",  HARNESS_DIAG_DMA     },
    { 0x0E2, "BPL1PTL",  HARNESS_DIAG_DMA     },
    { 0x0E4, "BPL2PTH",  HARNESS_DIAG_DMA     },
    { 0x0E8, "BPL3PTH",  HARNESS_DIAG_DMA     },
    { 0x0EC, "BPL4PTH",  HARNESS_DIAG_DMA     },
    { 0x100, "BPLCON0",  HARNESS_DIAG_VIDEO   },
    { 0x102, "BPLCON1",  HARNESS_DIAG_VIDEO   },
    { 0x104, "BPLCON2",  HARNESS_DIAG_VIDEO   },
    { 0x108, "BPL1MOD",  HARNESS_DIAG_VIDEO   },
    { 0x10A, "BPL2MOD",  HARNESS_DIAG_VIDEO   },
    { 0x180, "COLOR00",  HARNESS_DIAG_VIDEO   },
    { 0x182, "COLOR01",  HARNESS_DIAG_VIDEO   },
};

const char *harness_diag_reg_name(uint32_t reg)
{
    size_t i;
    for (i = 0; i < sizeof(k_regs) / sizeof(k_regs[0]); i++)
        if (k_regs[i].reg == reg) return k_regs[i].name;
    return NULL;
}

static uint32_t reg_category(uint32_t reg)
{
    size_t i;
    for (i = 0; i < sizeof(k_regs) / sizeof(k_regs[0]); i++)
        if (k_regs[i].reg == reg) return k_regs[i].category;
    return 0;
}

/* -------------------------------------------------------------------------
 * Category parsing
 * ------------------------------------------------------------------------- */

typedef struct cat_name {
    const char *name;
    uint32_t    flag;
} cat_name_t;

static const cat_name_t k_cats[] = {
    { "regs",    HARNESS_DIAG_REGS    },
    { "dma",     HARNESS_DIAG_DMA     },
    { "irq",     HARNESS_DIAG_IRQ     },
    { "disk",    HARNESS_DIAG_DISK    },
    { "copper",  HARNESS_DIAG_COPPER  },
    { "blitter", HARNESS_DIAG_BLITTER },
    { "video",   HARNESS_DIAG_VIDEO   },
    { "cia",     HARNESS_DIAG_CIA     },
    { "all",     HARNESS_DIAG_ALL     },
};

const char *harness_diag_categories(void)
{
    return "regs, dma, irq, disk, copper, blitter, video, cia, all";
}

bool harness_diag_parse(const char *list, uint32_t *out, const char **bad)
{
    char copy[256];
    char *tok;
    uint32_t flags = 0;

    if (list == NULL || out == NULL) return false;

    snprintf(copy, sizeof(copy), "%s", list);

    for (tok = strtok(copy, ","); tok != NULL; tok = strtok(NULL, ",")) {
        size_t i;
        bool found = false;
        for (i = 0; i < sizeof(k_cats) / sizeof(k_cats[0]); i++) {
            if (strcmp(tok, k_cats[i].name) == 0) {
                flags |= k_cats[i].flag;
                found = true;
                break;
            }
        }
        if (!found) {
            if (bad != NULL) *bad = list;
            return false;
        }
    }

    *out = flags;
    return true;
}

/* -------------------------------------------------------------------------
 * CPU trace
 *
 * patches/musashi/0001 turns M68K_INSTRUCTION_HOOK on, which is what makes
 * m68k_set_instr_hook_callback fire per instruction. Without that patch this
 * whole section is inert.
 * ------------------------------------------------------------------------- */

enum { TRACE_RING_MAX = 4096 };

typedef struct cpu_trace {
    uint32_t ring[TRACE_RING_MAX];
    uint32_t size;      /* 0 = ring disabled */
    uint32_t head;
    uint64_t total;
    uint32_t pc_lo, pc_hi;
    int      cpu_type;
    uint32_t bp_addr;
    bool     bp_armed;
    bool     bp_hit;
} cpu_trace_t;

static cpu_trace_t g_trace;

static void trace_instr(unsigned int pc)
{
    if (g_trace.size != 0) {
        g_trace.ring[g_trace.head] = (uint32_t)pc;
        g_trace.head = (g_trace.head + 1u) % g_trace.size;
        g_trace.total++;
    }

    if (g_trace.pc_hi > g_trace.pc_lo &&
        pc >= g_trace.pc_lo && pc <= g_trace.pc_hi) {
        char text[256];
        m68k_disassemble(text, pc, (unsigned int)g_trace.cpu_type);
        printf("[CPU ] %08x  %s\n", pc, text);
    }

    if (g_trace.bp_armed && !g_trace.bp_hit && pc == g_trace.bp_addr) {
        char text[256];
        g_trace.bp_hit = true;
        m68k_disassemble(text, pc, (unsigned int)g_trace.cpu_type);
        printf("[BRK ] hit %08x  %s\n", pc, text);
        fflush(stdout);
        /* End the timeslice so the front-end regains control this step
         * instead of after the rest of the quantum. */
        m68k_end_timeslice();
    }
}

void harness_diag_set_breakpoint(uint32_t addr, int cpu_type)
{
    g_trace.bp_addr  = addr;
    g_trace.bp_armed = true;
    g_trace.bp_hit   = false;
    if (g_trace.cpu_type == 0) g_trace.cpu_type = cpu_type;
    m68k_set_instr_hook_callback(trace_instr);
}

bool harness_diag_breakpoint_hit(uint32_t *addr_out)
{
    if (!g_trace.bp_hit) return false;
    if (addr_out != NULL) *addr_out = g_trace.bp_addr;
    return true;
}

void harness_diag_set_cpu_trace(uint32_t ring, uint32_t pc_lo, uint32_t pc_hi,
                                int cpu_type)
{
    /* Preserve a breakpoint armed before this call. */
    uint32_t bp_addr = g_trace.bp_addr;
    bool     bp_armed = g_trace.bp_armed;

    memset(&g_trace, 0, sizeof(g_trace));
    g_trace.bp_addr = bp_addr;
    g_trace.bp_armed = bp_armed;
    g_trace.size    = ring > TRACE_RING_MAX ? TRACE_RING_MAX : ring;
    g_trace.pc_lo   = pc_lo;
    g_trace.pc_hi   = pc_hi;
    g_trace.cpu_type = cpu_type;

    if (g_trace.size != 0 || pc_hi > pc_lo)
        m68k_set_instr_hook_callback(trace_instr);
}

void harness_diag_dump_trace(void)
{
    uint32_t count, i, start;

    if (g_trace.size == 0) return;

    count = (g_trace.total < g_trace.size)
        ? (uint32_t)g_trace.total : g_trace.size;
    start = (g_trace.head + g_trace.size - count) % g_trace.size;

    printf("[CPU ] last %u of %llu instructions:\n",
           count, (unsigned long long)g_trace.total);

    for (i = 0; i < count; i++) {
        uint32_t pc = g_trace.ring[(start + i) % g_trace.size];
        char text[256];
        m68k_disassemble(text, pc, (unsigned int)g_trace.cpu_type);
        printf("[CPU ]   %08x  %s\n", pc, text);
    }
    fflush(stdout);
}

void harness_diag_disasm(harness_t *h, uint32_t addr, uint32_t count,
                         int cpu_type)
{
    uint32_t i;

    (void)h;
    printf("[DIS ] %u instructions at %08x:\n", count, addr);
    for (i = 0; i < count; i++) {
        char text[256];
        unsigned int len = m68k_disassemble(text, addr, (unsigned int)cpu_type);
        printf("[DIS ]   %08x  %s\n", addr, text);
        if (len == 0) break;   /* undecodable: stop rather than walk off */
        addr += len;
    }
    fflush(stdout);
}

/* -------------------------------------------------------------------------
 * State watcher
 * ------------------------------------------------------------------------- */

typedef struct diag_state {
    uint32_t flags;
    uint32_t status_every;

    bool     have_last;
    uint16_t dmacon;
    uint16_t intena;
    uint16_t intreq;
    uint8_t  ipl;
    uint32_t frame_w, frame_h;

    uint64_t reg_writes;
    uint64_t reg_reads;
    uint64_t serial_bytes;
} diag_state_t;

static diag_state_t g_diag;

static void watch_poll(harness_t *h, uint64_t frame);

/* Bit names, LSB first, for the DMACON and INTENA decoders. */
static const char *k_dma_bits[] = {
    "AUD0", "AUD1", "AUD2", "AUD3", "DSK", "SPR", "BLT", "COP",
    "BPL",  "DMAEN", "BLTPRI", NULL, NULL, "BZERO", "BBUSY", NULL
};

static const char *k_int_bits[] = {
    "TBE", "DSKBLK", "SOFT", "PORTS", "COPER", "VERTB", "BLIT", "AUD0",
    "AUD1", "AUD2", "AUD3", "RBF", "DSKSYN", "EXTER", "INTEN", "SET"
};

static void print_bits(const char *label, uint16_t value, const char **names)
{
    int i;
    printf("%s=%04x [", label, value);
    for (i = 0; i < 16; i++) {
        if ((value & (1u << i)) == 0 || names[i] == NULL) continue;
        printf(" %s", names[i]);
    }
    printf(" ]");
}

static void diag_on_mmio(void *opaque, uint32_t reg, uint16_t value, bool is_write)
{
    const char *name;
    uint32_t cat;

    (void)opaque;

    if (!is_write) {
        /* Reads are mostly noise, but a guest spinning on a status register is
         * exactly the case where seeing the returned value settles it. */
        if ((g_diag.flags & HARNESS_DIAG_IRQ) != 0 &&
            (reg == 0x01Cu || reg == 0x01Eu)) {
            g_diag.reg_reads++;
            if (g_diag.reg_reads <= 40u || (g_diag.reg_reads % 100000u) == 0u)
                printf("[REG ] %-8s (%03x) -> %04x  (read #%llu)\n",
                       harness_diag_reg_name(reg), reg, value,
                       (unsigned long long)g_diag.reg_reads);
        }
        return;
    }

    g_diag.reg_writes++;

    cat = reg_category(reg);
    if ((g_diag.flags & HARNESS_DIAG_REGS) == 0 &&
        (cat == 0 || (g_diag.flags & cat) == 0))
        return;

    name = harness_diag_reg_name(reg);
    if (name != NULL)
        printf("[REG ] %-8s (%03x) <- %04x\n", name, reg, value);
    else
        printf("[REG ] %-8s (%03x) <- %04x\n", "?", reg, value);
    fflush(stdout);
}

void harness_diag_attach(harness_t *h, uint32_t flags, uint32_t status_every)
{
    memset(&g_diag, 0, sizeof(g_diag));
    g_diag.flags = flags;
    g_diag.status_every = status_every;

    /* The status line reports the write count, so the sink must be installed
     * whenever either is enabled — otherwise regw reads as a convincing zero. */
    if (flags != 0 || status_every != 0)
        harness_set_mmio_sink(h, diag_on_mmio, &g_diag);
}

void harness_diag_frame(harness_t *h, uint64_t frame)
{
    RigelContext *ctx = harness_rigel(h);
    uint16_t dmacon, intena, intreq;
    uint8_t  ipl;

    if (g_diag.flags == 0 && g_diag.status_every == 0) return;

    dmacon = rigel_custom_read16(ctx, 0x002);
    intena = rigel_get_intena(ctx);
    intreq = rigel_get_intreq(ctx);
    ipl    = rigel_get_ipl(ctx);

    /* Report the transitions rather than the levels: for a boot that goes
     * quiet, when a mask last changed is the useful fact. */
    if (g_diag.have_last) {
        if (dmacon != g_diag.dmacon && (g_diag.flags & HARNESS_DIAG_DMA)) {
            printf("[DMA ] frame %-5llu ", (unsigned long long)frame);
            print_bits("DMACON", dmacon, k_dma_bits);
            printf("\n");
        }
        if (intena != g_diag.intena && (g_diag.flags & HARNESS_DIAG_IRQ)) {
            printf("[IRQ ] frame %-5llu ", (unsigned long long)frame);
            print_bits("INTENA", intena, k_int_bits);
            printf("\n");
        }
        if (ipl != g_diag.ipl && (g_diag.flags & HARNESS_DIAG_IRQ)) {
            printf("[IRQ ] frame %-5llu IPL %u -> %u (INTREQ=%04x)\n",
                   (unsigned long long)frame, g_diag.ipl, ipl, intreq);
        }
    }

    {
        rigel_frame_t f;
        if (rigel_get_frame(ctx, &f) &&
            (f.width != g_diag.frame_w || f.height != g_diag.frame_h)) {
            if (g_diag.have_last && (g_diag.flags & HARNESS_DIAG_VIDEO))
                printf("[VID ] frame %-5llu display %ux%u -> %ux%u\n",
                       (unsigned long long)frame,
                       g_diag.frame_w, g_diag.frame_h, f.width, f.height);
            g_diag.frame_w = f.width;
            g_diag.frame_h = f.height;
        }
    }

    g_diag.dmacon = dmacon;
    g_diag.intena = intena;
    g_diag.intreq = intreq;
    g_diag.ipl    = ipl;
    g_diag.have_last = true;

    watch_poll(h, frame);

    if (g_diag.status_every != 0 && (frame % g_diag.status_every) == 0) {
        /* Both views of INTREQ: the API accessor and what a guest actually
         * reads at DFF01E. They must agree — a guest polling INTREQR sees the
         * second one. */
        printf("[STAT] frame %-6llu pc=%08x sr=%04x  DMACON=%04x INTENA=%04x "
               "INTREQ=%04x INTREQR@1e=%04x IPL=%u  regw=%llu\n",
               (unsigned long long)frame,
               m68k_get_reg(NULL, M68K_REG_PC),
               (unsigned)m68k_get_reg(NULL, M68K_REG_SR),
               dmacon, intena, intreq,
               rigel_custom_read16(ctx, 0x01E), ipl,
               (unsigned long long)g_diag.reg_writes);
        fflush(stdout);
    }
}

void harness_diag_summary(harness_t *h, uint64_t frames)
{
    RigelContext *ctx = harness_rigel(h);

    if (g_diag.flags == 0 && g_diag.status_every == 0) return;

    printf("[STAT] final: frames=%llu pc=%08x DMACON=%04x INTENA=%04x "
           "INTREQ=%04x IPL=%u register-writes=%llu\n",
           (unsigned long long)frames,
           m68k_get_reg(NULL, M68K_REG_PC),
           rigel_custom_read16(ctx, 0x002),
           rigel_get_intena(ctx), rigel_get_intreq(ctx), rigel_get_ipl(ctx),
           (unsigned long long)g_diag.reg_writes);
}

/* -------------------------------------------------------------------------
 * Memory monitoring
 * ------------------------------------------------------------------------- */

typedef struct watch_region {
    uint32_t addr;
    uint32_t len;
    uint8_t *shadow;
    bool     primed;
} watch_region_t;

static watch_region_t g_watch[HARNESS_DIAG_WATCH_MAX];
static uint32_t       g_watch_count;

bool harness_diag_watch(harness_t *h, uint32_t addr, uint32_t len)
{
    watch_region_t *w;

    if (g_watch_count >= HARNESS_DIAG_WATCH_MAX) return false;
    if (len == 0u || len > (1u << 20)) return false;

    w = &g_watch[g_watch_count];
    w->shadow = (uint8_t *)calloc(1, len);
    if (w->shadow == NULL) return false;

    w->addr = addr;
    w->len = len;
    w->primed = false;
    g_watch_count++;

    harness_peek(h, addr, w->shadow, len);
    w->primed = true;
    return true;
}

/* Called once per frame from harness_diag_frame. Reports the changed span
 * rather than every byte, which is what stays readable when a screen buffer
 * is being filled. */
static void watch_poll(harness_t *h, uint64_t frame)
{
    uint32_t i;

    for (i = 0; i < g_watch_count; i++) {
        watch_region_t *w = &g_watch[i];
        uint8_t *now;
        uint32_t j, first = 0, last = 0, changed = 0;

        if (!w->primed) continue;

        now = (uint8_t *)malloc(w->len);
        if (now == NULL) return;
        harness_peek(h, w->addr, now, w->len);

        for (j = 0; j < w->len; j++) {
            if (now[j] == w->shadow[j]) continue;
            if (changed == 0) first = j;
            last = j;
            changed++;
        }

        if (changed != 0) {
            printf("[MEM ] frame %-5llu %08x+%u: %u bytes changed, span %08x..%08x\n",
                   (unsigned long long)frame, w->addr, w->len, changed,
                   w->addr + first, w->addr + last);
            fflush(stdout);
            memcpy(w->shadow, now, w->len);
        }

        free(now);
    }
}

bool harness_diag_dump_mem(harness_t *h, uint32_t addr, uint32_t len,
                           const char *path)
{
    uint8_t *buf;
    bool ok = true;

    if (len == 0u || len > (16u << 20)) return false;

    buf = (uint8_t *)malloc(len);
    if (buf == NULL) return false;
    harness_peek(h, addr, buf, len);

    if (path != NULL) {
        FILE *fp = fopen(path, "wb");
        if (fp == NULL) {
            free(buf);
            return false;
        }
        ok = (fwrite(buf, 1, len, fp) == len);
        fclose(fp);
        printf("[MEM ] wrote %u bytes from %08x to %s\n", len, addr, path);
    } else {
        uint32_t i;
        /* Say whether the overlay was in the way: a low-address dump that
         * comes back as ROM is otherwise a confusing surprise. */
        printf("[MEM ] %08x + %u bytes%s:\n", addr, len,
               harness_overlay_active(h) && addr < 0x100000u
                   ? " (OVL on: low addresses read ROM, not Chip RAM)" : "");
        for (i = 0; i < len; i += 16) {
            uint32_t j, n = (len - i < 16u) ? (len - i) : 16u;
            printf("[MEM ]   %08x ", addr + i);
            for (j = 0; j < 16; j++)
                if (j < n) printf("%02x ", buf[i + j]); else printf("   ");
            printf(" |");
            for (j = 0; j < n; j++) {
                uint8_t c = buf[i + j];
                putchar((c >= 32 && c < 127) ? (int)c : '.');
            }
            printf("|\n");
        }
    }

    free(buf);
    fflush(stdout);
    return ok;
}
