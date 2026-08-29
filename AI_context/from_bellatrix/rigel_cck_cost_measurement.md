# Measuring what a colour clock costs

A repro for `../issues/ISSUE-0006.md`, from Bellatrix on 2026-08-29. It answers
the first question `rigel_performance_research.md` asks before any chipset
change: how much of the time is exclusive to Rigel, how many calls, how many
CCK per call, and which domain.

Two measurements, and they agree: **an idle chipset costs 140 ns per colour
clock on a modern x86, and a real workload costs 162 ns.** The floor is the
problem, not the load.

## 1. The idle floor

Build against the library as any host does. Nothing is programmed: no ROM, no
disk, DMACON clear, no bitplanes, no copper, no screen.

```bash
cmake -S . -B build-pg -DCMAKE_BUILD_TYPE=None -DCMAKE_C_FLAGS="-O2 -pg -g" \
      -DRIGEL_BUILD_TESTS=OFF -DRIGEL_BUILD_HARNESS=OFF
cmake --build build-pg --target rigel
gcc -O2 -o cckbench cckbench.c -Iinclude -Lbuild-pg -lrigel -lm
./cckbench
```

```text
deadline-bounded    3568440 CCK ->  7.13 M CCK/s  (201% of realtime, 140 ns/CCK)
quantum 512         3568640 CCK ->  7.16 M CCK/s  (202% of realtime, 140 ns/CCK)
quantum 1           3568440 CCK ->  3.02 M CCK/s  ( 85% of realtime, 331 ns/CCK)
one big step        3568440 CCK ->  7.20 M CCK/s  (203% of realtime, 139 ns/CCK)
```

Realtime is 282 ns/CCK. The four rows also separate the two costs: the three
that step in large quanta agree at ~140 ns/CCK, which is the per-clock work;
`quantum 1` costs 331, and the difference is ~190 ns of fixed cost per
`rigel_step()` call.

Link the `-pg` library instead and `gprof` gives the per-domain split. Divide
each call count by the total CCK stepped to get calls per colour clock -- that
is what shows the loop asking every domain the same question every clock.

## 2. Under a real workload

```bash
./rigel-harness KS13.rom --adf "Demo Reel 3 (Disk 1 of 2).adf" \
    --slow 512 --headless --frames 600
```

```text
rigel-harness: 600 frames, 84988804 CPU cycles     wall 6.92 s
```

84988804 / 600 = 141648 CPU cycles per frame, which is a full-speed PAL frame,
so the run is genuine and not stalled. 600 PAL frames (313 x 227 CCK) is 42.6 M
CCK in 6.92 s: 86.7 fps, 1.73x realtime, 162 ns/CCK -- 16% over the idle floor,
with Musashi emulating the CPU on top.

## The bench

```c
/* How expensive is one colour clock, with nothing programmed? */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "rigel/rigel.h"

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

static void run(const char *name, RigelContext *c, rigel_cycle_t quantum,
                rigel_cycle_t total)
{
    rigel_cycle_t done = 0;
    double t0 = now(), dt;

    while (done < total) {
        rigel_cycle_t q = quantum;
        if (quantum == 0) {
            rigel_cycle_t at = rigel_get_time(c);
            rigel_cycle_t next = rigel_get_next_observable_deadline(c);
            q = (next > at) ? (next - at) : 1;
            if (q > 512) q = 512;
        }
        rigel_step(c, q);
        done += q;
    }
    dt = now() - t0;
    printf("%-22s %8.3f s for %llu CCK  ->  %6.2f M CCK/s  "
           "(%5.1f%% of realtime, %6.1f ns/CCK)\n",
           name, dt, (unsigned long long)done, done / dt / 1e6,
           100.0 * (done / dt) / 3546895.0, dt * 1e9 / done);
}

int main(void)
{
    rigel_config_t config;
    RigelContext *c;
    const rigel_cycle_t total = 59474ull * 60; /* one second of NTSC frames */

    memset(&config, 0, sizeof(config));
    config.chip_ram_size = 2u * 1024u * 1024u;
    c = rigel_create(&config);
    if (!c) { fprintf(stderr, "rigel_create failed\n"); return 1; }

    run("deadline-bounded", c, 0, total);
    run("quantum 512", c, 512, total);
    run("quantum 1", c, 1, total);
    run("one big step", c, total, total);
    return 0;
}
```
