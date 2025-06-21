;
; hiresAtari.s
; RetroMate
;
; Created by Stefan Wessels, June 2025.
; Based on work by Oliver Schmidt, January 2020.
;
;

;-----------------------------------------------------------------------
.include "atari.inc"
.include "zeropage.inc"

.export _hires_init, _hires_done, _hires_draw, _hires_mask
.import popa, popax, _hires_piece


;-----------------------------------------------------------------------
; Display-list related defenitions
scrn     = $A000                                 ; screen starts here
scnd     = $B000                                 ; lower part of screen here
top      = ((scnd - scrn) / $28)                 ; display list entries before scnd
bot      = ($c0-top)                             ; display list entries after scnd
gfx_mode = $0f                                   ; mode to run the screen at
txt_scrn = $BC00
txt_mode = $02                                   ; mode to run the screen at

;-----------------------------------------------------------------------
; Display list - mode 0x0f (320x192, 2 color).
; Goes in its own segment so it won't cross a boundry
.segment "DLIST"

hires_list:
        .byte $70,$70,$70                       ; 24 blank lines
        .byte $40 + gfx_mode,<scrn,>scrn            ; Mode $0x + LMS, setting screen memory to $9100
        .repeat top-1                           ; 96 lines of mode $0x (incl LMS row above)
            .byte gfx_mode
        .endrep
        .byte $40 +gfx_mode, <(scnd), >(scnd)       ; clear the 4k boundry and start another row of mode $0x
        .repeat bot-1                           ; another 96 lines of mode $0x (incl. LMS row above)
            .byte gfx_mode
        .endrep
        .byte $41,<hires_list,>hires_list       ; Vertical Blank jump to start of hires_list

text_list:
        .byte $70,$70,$70                       ; 24 blank lines
        .byte $40 + txt_mode,<txt_scrn,>txt_scrn            ; Mode $0x + LMS, setting screen memory to $9100
        .repeat 24                              ; 96 lines of mode $0x (incl LMS row above)
            .byte txt_mode
        .endrep
        .byte $41,<text_list,>text_list         ; Vertical Blank jump to start of text_list

.rodata

;-----------------------------------------------------------------------
; lookup to the start of a graphics row
BASELO:
    .repeat top, I                              ; $60 rows at $9100
        .byte <(scrn + I * 40)
    .endrep
    .repeat bot, I                              ; $60 rows at $a000
        .byte <(scnd + I * 40)
    .endrep

BASEHI:
    .repeat top, I 
        .byte >(scrn + I * 40)
    .endrep
    .repeat bot, I 
        .byte >(scnd + I * 40)
    .endrep

;-----------------------------------------------------------------------

.segment "SHADOW_RAM2"

;-----------------------------------------------------------------------
; Init the hires screen with the display list
.proc _hires_init

    lda #0                                      ; stop the dma
    sta DMACTL
    lda #<hires_list                           ; install the new display list
    sta SDLSTL
    lda #>hires_list
    sta SDLSTH
    lda #$22                                    ; resume the DMA
    sta DMACTL

    rts

.endproc


.proc   _hires_done

    lda #0                                      ; stop the dma
    sta DMACTL
    lda #<text_list                           ; install the new display list
    sta SDLSTL
    lda #>text_list
    sta SDLSTH
    lda #$22                                    ; resume the DMA
    sta DMACTL
    lda #<txt_scrn
    sta SAVMSC
    lda #>txt_scrn
    sta SAVMSC+1
    rts

.endproc

.proc   _hires_draw

        sta     src+1       ; 'src' lo
        stx     src+2       ; 'src' hi

        jsr     popax       ; 'rop'
        stx     rop
        sta     rop+1

        jsr     popa        ; 'ysize'
        sta     ymax+1

        jsr     popa        ; 'xsize'
        sta     xmax+1

        jsr     popa        ; 'ypos'
        sta     ypos+1
        tax

        clc
        adc     ymax+1
        sta     ymax+1

        jsr     popa        ; 'xpos'
        sta     xpos+1

        clc
        adc     xmax+1
        sta     xmax+1
yloop:
        lda     BASELO,x
        sta     dst+1
        lda     BASEHI,x
        sta     dst+2

xpos:   ldx     #$FF        ; Patched
xloop:
src:    lda     $FFFF,y     ; Patched
        iny
rop:    nop                 ; Patched
        nop                 ; Patched
dst:    sta     $FFFF,x     ; Patched
        inx
xmax:   cpx     #$FF        ; Patched
        bne     xloop

        inc     ypos+1
ypos:   ldx     #$FF        ; Patched
ymax:   cpx     #$FF        ; Patched
        bne     yloop
        rts

.endproc


.proc   _hires_mask

        stx     rop         ; 'rop' hi
        sta     rop+1       ; 'rop' lo

        jsr     popa        ; 'ysize'
        sta     ymax+1

        jsr     popa        ; 'xsize'
        sta     xmax+1

        jsr     popa        ; 'ypos'
        tax

        clc
        adc     ymax+1
        sta     ymax+1

        jsr     popa        ; 'xpos'
        sta     xpos+1

        clc
        adc     xmax+1
        sta     xmax+1

yloop:
        lda     BASELO,x
        sta     src+1
        sta     dst+1
        lda     BASEHI,x
        sta     src+2
        sta     dst+2

xpos:   ldy     #$FF        ; Patched
xloop:
src:    lda     $FFFF,y     ; Patched
rop:    nop                 ; Patched
        nop                 ; Patched
dst:    sta     $FFFF,y     ; Patched
        iny
xmax:   cpy     #$FF        ; Patched
        bne     xloop

        inx
ymax:   cpx     #$FF        ; Patched
        bne     yloop
        rts

.endproc
