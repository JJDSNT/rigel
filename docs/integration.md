# Integration

## Scope

`rigel` deve poder ser integrado a um host externo sem depender de CPU, frontend ou plataforma especifica.

## Initial Notes

- o host traduz acessos externos para MMIO interno
- o host avanca o tempo da biblioteca
- o host consulta estado de IRQ, video e outros subsistemas
- recursos externos, como memoria e dispositivos, devem entrar por interfaces
