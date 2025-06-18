;
; hiresC64.s
; RetroMate
;
; Created by Stefan Wessels, January 2025.
; Based on work by Oliver Schmidt, January 2020.
;
;

.export _hires_draw, _hires_mask, _hires_color

.include "c64.inc"
.include "zeropage.inc"

.import popa, popax, _hires_piece

.rodata

.define VIC_BASE_RAM			$C000
.define SCREEN_RAM				VIC_BASE_RAM + $2000
.define CHARMAP_RAM         VIC_BASE_RAM + $2800

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
        lda 1       ; kernel out
        and #$FD
        sta 1

        ldy #$00   ; source index
        ldx ypos+1      ; start row
yloop:  clc
xofflo: lda #$FF        ; Patched
        adc BASELO,x
        sta dst+1
xoffhi: lda #$FF        ; Patched
        adc BASEHI,x
        sta dst+2

xpos:   lda #$FF
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
        
        lda dst+1 ; next col
        clc
        adc #8
        sta dst+1
        bcc :+
        inc dst+2
: clc
        inc xcurr+1
xcurr:  ldx #$FF        ; Patched 
xmax:   cpx #$FF    ; Patched
        bne xloop
        inc ypos+1
ypos:   ldx #$FF    ; Patched
ymax:   cpx #$FF    ; Patched
        bne yloop

        lda 1       ; kernel in
        ora #$02
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
        lda 1       ; kernel out
        and #$FD
        sta 1

        ldy ypos+1
yloop:  clc
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
        lda src+1 ; adjust src & dest 
        adc #8  ; since the screen > 256 in x
        sta src+1
        bcc :+
        inc src+2
        clc
:       lda dst+1
        adc #8
        sta dst+1
        bcc :+
        inc dst+2
        clc
:       inx         ; next column
xmax:   cpx #$FF    ; Patched
        bne xloop
        inc ypos+1
ypos:   ldy #$FF    ; Patched
ymax:   cpy #$FF    ; Patched
        bne yloop

        lda 1       ; kernel in
        ora #$02
        sta 1
        cli         ; resume interrupts

        rts
.endproc



.proc _hires_color

        ; stx color+1     ; 'rop' hi
        sta color+1   ; 'rop' lo

        jsr popa    ; 'ysize'
        sta ymax+1

        jsr popa    ; 'xsize'
        sta xmax+1

        jsr popa    ; 'ypos'
        sta ypos+1
        tay

        clc
        adc ymax+1
        sta ymax+1

        jsr popa    ; 'xpos'
        sta xpos+1

        clc
        adc xmax+1
        sta xmax+1

        sei         ; stop interrupts
        lda 1       ; kernel out
        and #$FD
        sta 1

ypos:   ldy #$FF    ; Patched
yloop:  lda CBASELO,y
        sta dst+1
        lda CBASEHI,y
        sta dst+2

color:  lda #$15    ; Patched
xpos:   ldx #$FF    ; Patched
dst:    sta $FFFF,x ; Patched
        inx
xmax:   cpx #$FF    ; Patched
        bne dst

        iny
ymax:   cpy #$FF    ; Patched
        bne yloop

        lda 1       ; kernel in
        ora #$02
        sta 1
        cli         ; resume interrupts

        rts
.endproc
