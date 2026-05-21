# IRQ Model

## Scope

O modelo de interrupcao cobre requisicao, mascara e consulta de estado pendente.

## Initial Notes

- `INTREQ` representa fontes requisitadas
- `INTENA` representa fontes habilitadas
- o estado pendente resulta da combinacao entre requisicao e habilitacao
- a entrega da interrupcao ao processador pertence ao host
