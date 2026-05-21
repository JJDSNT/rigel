# riegel

Scaffold inicial para um projeto C modular voltado a MMIO, IRQ, timing e subsistemas inspirados em chipset.

## Estrutura

- `include/riegel/`: API publica.
- `src/`: implementacao por dominio.
- `tests/`: executaveis de teste simples.
- `docs/`: notas de arquitetura e integracao.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Status

Esta base contem stubs compilaveis para orientar a implementacao futura.
