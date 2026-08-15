* smoke.S — minimal chipset exercise for the Rigel harness.
*
* Assembled to a raw 256K ROM image. The first eight bytes are the reset
* vector the 68000 reads through the OVL overlay at address 0.
*
* It writes a known pattern into Chip RAM, sets a background colour, then
* spins. --dump checks the pattern, --break checks the PC got there.

ROMBASE     equ $fc0000
CUSTOM      equ $dff000
COLOR00     equ $180
PATTERN     equ $ca5eb0a7
MARKER      equ $1000           * where the pattern lands in Chip RAM

            org     ROMBASE

            dc.l    $00080000       * initial SP: top of 512K Chip RAM
            dc.l    start           * initial PC

CIAA_PRA    equ $bfe001
CIAA_DDRA   equ $bfe201

start:
            lea     CUSTOM,a6
            move.w  #$0f0f,COLOR00(a6)

            * Drop OVL so Chip RAM, not the ROM, answers at address 0. Until
            * this runs the overlay is on and every low read returns ROM --
            * including the ones a debugger makes.
            move.b  #$01,CIAA_DDRA      * PRA bit 0 becomes an output
            move.b  #$00,CIAA_PRA       * drive it low

            * Fill 16 longs at MARKER so --watch and --dump have something
            * unambiguous to find.
            lea     MARKER,a0
            move.l  #PATTERN,d0
            moveq   #15,d1
fill:
            move.l  d0,(a0)+
            addq.l  #1,d0
            dbra    d1,fill

            move.w  #$00f0,COLOR00(a6)

done:
            bra.s   done
