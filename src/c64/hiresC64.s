;
; hiresC64.s
; RetroMate
;
; Created by Stefan Wessels, 2025.
; Based on work by Oliver Schmidt, January 2020.
;
;

.export _hires_draw, _hires_mask, _hires_color

.include "c64.inc"
.include "zeropage.inc"

.import popa, popax, _hires_piece

.rodata

.define VIC_BASE_RAM    $C000
.define SCREEN_RAM      VIC_BASE_RAM + $2000
.define CHARMAP_RAM     VIC_BASE_RAM + $2800

BASELO:
    .repeat 25, I
    .byte   <(VIC_BASE_RAM + 320 * I)
    .endrep

BASEHI:
    .repeat 25, I
    .byte   >(VIC_BASE_RAM + 320 * I)
    .endrep

CBASELO:
    .repeat 25, I
    .byte   <(SCREEN_RAM + 40 * I)
    .endrep

CBASEHI:
    .repeat 25, I
    .byte   >(SCREEN_RAM + 40 * I)
    .endrep

.code

.proc   _hires_draw

        sta src+1   ; 'src' lo
        stx src+2   ; 'src' hi

        jsr popax   ; 'rop'
        stx rop
        sta rop+1

        jsr popa    ; 'ysize'
        sta ymax+1

        jsr popa    ; 'xsize'
        sta xmax+1

        jsr popa    ; 'ypos'
        sta ypos+1

        clc
        adc ymax+1
        sta ymax+1

        jsr popa    ; 'xpos'
        sta xpos+1

        clc
        adc xmax+1
        sta xmax+1

        lda #0
        sta xoffhi+1
        lda xpos+1
        beq :+
        asl         ; mult 8
        rol xoffhi+1
        asl
        rol xoffhi+1
        asl
        rol xoffhi+1
:       sta xofflo+1

        sei         ; stop interrupts
        lda #$34    ; Basic ROM off; Kernal ROM off; I/O off
        sta 1

        ldy #$00
        ldx ypos+1      ; start row
yloop:  clc
xofflo: lda #$FF        ; Patched
        adc BASELO,x
        sta dst+1
xoffhi: lda #$FF        ; Patched
        adc BASEHI,x
        sta dst+2

xpos:   lda #$FF        ; Patched
        sta xcurr+1
xloop:  ldx #0          ; do one character column
src:    lda $ffff,y ; Patched
        iny
rop:    nop
        nop
dst:    sta $ffff,x ; Patched
        inx
        cpx #8
        bne src

        clc
        lda dst+1 ; next col
        adc #8
        sta dst+1
        bcc :+
        inc dst+2
:       inc xcurr+1
xcurr:  ldx #$FF        ; Patched
xmax:   cpx #$FF    ; Patched
        bne xloop
        inc ypos+1
ypos:   ldx #$FF    ; Patched
ymax:   cpx #$FF    ; Patched
        bne yloop

        lda #$36    ; Basic ROM off; Kernal ROM on; I/O on
        sta 1
        cli         ; resume interrupts
        rts
.endproc


.proc   _hires_mask
        stx rop     ; 'rop' hi
        sta rop+1   ; 'rop' lo

        jsr popa    ; 'ysize'
        sta ymax+1

        jsr popa    ; 'xsize'
        sta xmax+1

        jsr popa    ; 'ypos'
        sta ypos+1

        clc
        adc ymax+1
        sta ymax+1

        jsr popa    ; 'xpos'
        sta xpos+1

        ; clc
        adc xmax+1
        sta xmax+1

        lda #0
        sta xoffhi+1
        lda xpos+1
        beq :+
        asl         ; mult 8
        rol xoffhi+1
        asl
        rol xoffhi+1
        asl
        rol xoffhi+1
:       sta xofflo+1

        sei
        lda #$34
        sta 1

        ldy ypos+1
        ; clc
xofflo: lda #$FF ; Patched
        adc BASELO,y
        sta src+1
        sta dst+1
xoffhi: lda #$FF ; Patched
        adc BASEHI,y
        sta src+2
        sta dst+2

xpos:   ldx #$FF
xloop:  ldy #7          ; do one character column
src:    lda $ffff,y ; Patched
rop:    nop
        nop
dst:    sta $ffff,y ; Patched
        dey
        bpl src
        clc       ; probably uneccesary but maybe rop set it
        lda src+1 ; adjust src & dest
        adc #8    ; next col is 8 away
        sta src+1 ; and screen > 256 wide
        bcc :+
        inc src+2
        clc
:       lda dst+1
        adc #8
        sta dst+1
        bcc :+
        inc dst+2
:       inx         ; next column
xmax:   cpx #$FF    ; Patched
        bne xloop
        inc ypos+1
ypos:   ldy #$FF    ; Patched
ymax:   cpy #$FF    ; Patched
        bne xofflo

        lda #$36
        sta 1
        cli
        rts
.endproc



.proc _hires_color

        sta color+1   ; color

        jsr popa    ; 'ysize'
        sta ymax+1

        jsr popa    ; 'xsize'
        sta xmax+1

        jsr popa    ; 'ypos'
        sta ypos+1

        clc
        adc ymax+1
        sta ymax+1

        jsr popa    ; 'xpos'
        sta xpos+1

        ; clc
        adc xmax+1
        sta xmax+1

        sei
        lda #$34
        sta 1

ypos:   ldy #$FF    ; Patched
yloop:  lda CBASELO,y
        sta dst+1
        lda CBASEHI,y
        sta dst+2

color:  lda #$FF    ; Patched
xpos:   ldx #$FF    ; Patched
dst:    sta $FFFF,x ; Patched
        inx
xmax:   cpx #$FF    ; Patched
        bne dst

        iny
ymax:   cpy #$FF    ; Patched
        bne yloop

        lda #$36
        sta 1
        cli
        rts
.endproc
