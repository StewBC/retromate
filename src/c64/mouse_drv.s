;
; mouse_drv.s
; RetroMate
;
; Created by Stefan Wessels, 2025.
; Based on work by Oliver Schmidt, January 2020.
;
;

.include "c64.inc"

.export _mouse_setup, _mouse_shutdown, _mouse_move

xpos = SID_ADConv1
ypos = SID_ADConv2

.proc _mouse_setup
    lda xpos
    cmp #$ff                ; if mouse is present this is not 255
    beq :+
    lda IRQVec+1            ; only install once
    cmp #>mouse_irq
    beq :+
    sei
    lda IRQVec
    sta old_irq
    lda IRQVec+1
    sta old_irq+1
    lda #<mouse_irq
    sta IRQVec
    lda #>mouse_irq
    sta IRQVec+1
    cli
    lda #0
:   rts
.endproc

.proc _mouse_shutdown
    lda IRQVec+1            ; only if installed
    cmp #>mouse_irq
    bne :+
    sei
    lda old_irq
    sta IRQVec
    lda old_irq+1
    sta IRQVec+1
    cli
:   rts
.endproc

; Below pretty much from the 1351 user manual
.proc mouse_irq
    cld
    lda xpos
    ldy old_xpos
    jsr movchk
    sty old_xpos
    clc
    adc VIC_SPR0_X
    sta VIC_SPR0_X
    txa
    adc #$00
    and #%00000001
    eor VIC_SPR_HI_X
    sta VIC_SPR_HI_X
    lda ypos
    ldy old_ypos
    jsr movchk
    sty old_ypos
    eor #$ff
    sec                 ; This wasn't in the manual - mouse tracked up always
    adc VIC_SPR0_Y
    sta VIC_SPR0_Y
    jmp (old_irq)
.endproc

; --- movchk routine ---
; Entry:
;   Y = old value of pot register
;   A = current value of pot register
; Exit:
;   Y = value to use for old value
;   X,A = delta value for position
.proc movchk
    sty oldvalue
    sta newvalue
    ldx #0
    sec
    sbc oldvalue
    beq :+
    sta _mouse_move
    and #%01111110
    cmp #%01000000
    bcs :++
    lsr a
    ldy newvalue
:   rts
:   ora #%11000000
    cmp #$FF
    beq :+
    sec
    ror a
    ldx #$ff
    ldy newvalue
    rts
:   lda #0
    rts
.endproc

old_irq:        .word 0
old_xpos:       .byte 0
old_ypos:       .byte 0
newvalue:       .byte 0
oldvalue:       .byte 0
_mouse_move:    .byte 0
