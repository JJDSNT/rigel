# Memory Map

## Scope

O projeto trabalha com offsets de registradores e interfaces de acesso, nao com um mapa completo da maquina.

## Initial Notes

- MMIO deve ser exposto por offsets internos
- Chip RAM deve ser acessada por interfaces dedicadas
- Decodificacao de enderecos globais pertence ao host
- offsets publicos devem ser nomeados e estaveis na API

## Initial Register Set

- `0x096`: `DMACON`
- `0x09a`: `INTENA`
- `0x09c`: `INTREQ`
- `0x180`: `COLOR00`
