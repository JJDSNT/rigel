# Musashi patches

`external/musashi` tracks upstream [kstenerud/Musashi](https://github.com/kstenerud/Musashi)
unmodified, currently pinned at `313ebf1`. Every local change lives here as a
patch so it stays reviewable and can be rebased when the pin moves.

```sh
git submodule update --init external/musashi
scripts/apply_musashi_patches.sh           # apply (idempotent)
scripts/apply_musashi_patches.sh --check   # report status, change nothing
scripts/apply_musashi_patches.sh --revert  # back to pristine upstream
```

`RIGEL_BUILD_HARNESS=ON` will not configure until the patches are applied.

## The set

| Patch | File | What it does |
| --- | --- | --- |
| `0001-enable-instruction-hook` | `m68kconf.h` | Turns `M68K_INSTRUCTION_HOOK` on so the host can trace per instruction. |
| `0002-fsave-an-and-d16-an-modes` | `m68kfpu.c` | Adds the missing `(An)` and `(d16,An)` addressing modes to `FSAVE`. |
| `0003-fpu-test-condition-no-fatalerror` | `m68kfpu.c` | An undefined FPU predicate no longer calls `fatalerror()`/`exit(1)`. Real 68881 hardware decodes the low 5 bits; this masks and warns instead of killing the host. |
| `0004-fline-trap-diagnostic` | `m68kcpu.h` | Logs PC / IR / opcode bytes on an F-line trap, capped at 30 hits. Without it an F-line fault is silent on the host and only shows up as a guest Alert. |
| `0005-fpu-missing-opmodes` | `m68kfpu.c` | Implements missing FPU opmodes: `FGETMAN`, `FSINH`, `FCOSH`, `FTANH`, `FATANH` and others. |
| `0006-widen-fpu-cpu-gate` | `m68k_in.c` | Dispatches `040fpu0`/`040fpu1` when either `CPU_TYPE_IS_040_PLUS` or `CPU_TYPE_IS_030_PLUS` holds, for configs that enable 040 without 030. Logs gate failures so a real bug is distinguishable from the expected 000/010/020 fall-through. |

## Diagnostic output

`0004` and `0006` print through `RIGEL_M68K_LOG`, defined in `m68kcpu.h` as
`fprintf(stderr, ...)`. Musashi builds as a standalone library with its own
include path and cannot reach a Rigel header, so hosts with no libc stdio
override it on the compiler command line:

```
-DRIGEL_M68K_LOG=my_serial_printf
```

## Provenance

These come from the Bellatrix `legacy` branch (`patches/0006`, `0013`, `0014`,
`0015`, `0017`, `0018`), verified to reproduce that tree byte-for-byte against
the same upstream pin. `0004` and `0006` were rewritten here: they originally
aliased Bellatrix's bare-metal `kprintf()`, which does not exist in Rigel.
