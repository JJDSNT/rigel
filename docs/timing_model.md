# Timing Model

## Scope

O tempo observado pelo chipset deve avancar de forma deterministica a partir de passos explicitos.

## Initial Notes

- o runtime avanca ciclos do contexto principal
- subsistemas internos devem reagir ao mesmo relogio base
- efeitos visiveis precisam respeitar a ordem temporal do chipset
