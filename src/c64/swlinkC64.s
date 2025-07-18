;
; swlinkC64.s
; RetroMate
;
; Created by Stefan Wessels, January 2025.
; This is free and unencumbered software released into the public domain.
;
;

.include "c64.inc"
.include "zeropage.inc"

.export _sw_init, _sw_send, _sw_shutdown, _plat_net_update, command_mask
.import _c64, _fics_tcp_recv, pusha, pushax

.struct c64_t
    rop_line       .res 2*7     ; 14 bytes
    rop_color      .res 2*2     ; 4 bytes
    draw_colors    .res 4       ; uint32_t
    terminal_display_width .res 1
    send_buffer    .res 80*24   ; I need access to this buffer
    status_log_buffer .res 13*25
.endstruct

base        = $DE00         ; base ACIA address
data        = base
status      = base+1
command     = base+2
control     = base+3

NUM_RECEIVE_BANKS  = 4      ; Each bank is 256 bytes so keep low, and pow-2

.code

; NMI handler called when a byte is received on data
.proc nmi_vec
    pha
    txa
    pha
    tya
    pha

    lda status              ; Bit 7: Interrupt Flag: 1 = Interrupt caused by SwiftLink
                            ; Bit 6: Carrier Detect: 1 = Carrier present
                            ; Bit 5: DSR:            1 = High, 0 = Low
                            ; Bit 4: Transmit Flag:  1 = Ready to get next Byte to
                            ;      :                     transmit in Data Register
                            ;      :                 0 = Chip currently sending Byte
                            ; Bit 3: Receive Flag:   1 = Byte received in Data Register
                            ;      :                 0 = Nothing received
                            ; Bit 2: Overrun:        1 = Overrun occured (*)
                            ; Bit 1: Frame Error:    1 = Frame Error occured (*)
                            ; Bit 0: Parity Error:   1 = Parity Error occured (*)

    ldx #%00000011          ; Disable interrupts mask
    stx command

    and #%00001000          ; only interested in receive
    beq enable

    ldy receiver_size       ; get index into the receive bank
    lda data                ; get the byte received
save_rx:
    sta receive_buffer,y    ; put it in the bank
    iny
    sty receiver_size       ; set index for next byte to receive

    cmp #$20                ; Any char before ' ' is an eol for me
    bcc line_end
    cmp #$25                ; '%' (fics% also means end of line)
    bne enable

line_end:
    cpy #2                  ; empty lines (just a line-feed) get ignored
    bcs line_ok
    ldy #0                  ; reset size to 0 for the receiver
    sty receiver_size
    jmp enable
line_ok:
    clc
    lda receiver_bank       ; start from the receiver's bank
    adc #1                  ; Advance to next bank
    and #NUM_RECEIVE_BANKS-1
    tax
    lda consumer_size,x     ; see if the consumer is caught up (empty buffer at this point)
    beq :+                  ; yes, empty so consumer is running okay
    lda #%00001011          ; receiver irq off (flow control on)
    sta command_mask
:   ldx receiver_bank       ; make this bank available to the consumer
    tya                     ; line-length
    sta consumer_size,x     ; Set the length of the string/bank for the consumer
    lda #0
    sta receiver_size       ; reset the receiver length
    inx                     ; move to the next receiver bank
    txa
    and #NUM_RECEIVE_BANKS-1
    sta receiver_bank
    clc
    adc #>receive_buffer    ; point at the active bank
    sta save_rx+2           ; set the bank

enable:
    lda command_mask
    sta command

    pla                     ; Restore rgisters
    tay
    pla
    tax
    pla
    rti                     ; Can't chain old handler - very unstable
.endproc


; Start the ACIA with recv and xmit on, xmit irq off but recv irq on
; Also install a custom NMI handler in the chain
.proc _sw_init
    lda #0
    sta status              ; reset the SwiftLink (any write to status)

    lda #<data_start        ; Clear the memory blocks
    sta ptr1
    lda #>data_start
    sta ptr1+1
    cmp #>data_end
    beq remain
    ldy #0
bank:
    lda #0
:   sta (ptr1),y
    dey
    bne :-
    inc ptr1+1
    lda ptr1+1
    cmp #>data_end
    bne bank
remain:
    lda #0
    ldx #<(data_end-data_start)
    beq done_clear
:   dex
    sta data_end-<(data_end-data_start),x
    bne :-
done_clear:

    sei                     ; install own NMI handler
    lda NMIVec
    sta old_nmi_vec
    lda NMIVec+1
    sta old_nmi_vec+1
    lda #<nmi_vec
    sta NMIVec
    lda #>nmi_vec
    sta NMIVec+1
    cli

    lda #%00011111          ; Control Register Mask
                            ; Bit 7  : Stop Bits: 1 = 2, 0 = 1 or 1.5
                            ; Bit 6-5: 00 = 8 Bit | 10 = 6 Bit
                            ;        : 01 = 7 Bit | 11 = 5 Bit
                            ; Bit 4  : Baud Rate Generator: 1 = Internal, 0 = External
                            ; Bit 3-0: Baud Rate: 0101 =  300 Bd | 1011 =  7200 Bd
                            ;        :            0110 =  600 Bd | 1100 =  9600 Bd
                            ;        :            0111 = 1200 Bd | 1101 = 14400 Bd
                            ;        :            1000 = 2400 Bd | 1110 = 19200 Bd
                            ;        :            1001 = 3600 Bd | 1111 = 38400 Bd
                            ;        :            1010 = 4800 Bd |
                            ;        : Turbo232:  0000 = enable Enhanced Speed ($DE07)
    sta control             ; Set Control mask

    lda #%00001001          ; Command Register Mask
                            ; Bits 7-5: Parity: 000 = None  | 101 = Mark
                            ;         :         001 = Odd   | 111 = Space
                            ;         :         011 = Even  |
                            ; Bit  4  : Echo: 0 = Off, 1 = On
                            ; Bit  3-2:       Transmitter IRQ |  RTS  | Transmitter
                            ;         :       ----------------+-------+------------
                            ;         :  00 =     Disabled    |  High |    Off
                            ;         :  01 =     Enabled     |  Low  |    On
                            ;         :  10 =     Disabled    |  Low  |    On
                            ;         :  11 =     Disabled    |  Low  |    BRK
                            ; Bit  1  : Receiver Interrupt: 0 = Enabled, 1 = Disabled
                            ; Bit  0  : IRQs: 0 = Off (Chip Receiver Disabled), 1 = On
                            ;-----------------------------------------------------------
                            ; Enhanced register $DE07 (Turbo232 - I don't use it)
                            ; Bit  7-3: Unused
                            ; Bit  2  : Mode Bit (read only): 1 = Bits 0-3 of $DE03 are
                            ;         : cleared and enhaced speed is enabled
                            ; Bits 1-0: Enhanced Baud Rate (read only if Mode Bit = 0):
                            ;         :            00 = 230400 Bd | 10 = 57600 Bd
                            ;         :            01 = 115200 Bd |
                            ;         :            11 = reserved for future expansions
    sta command
    sta command_mask
    rts
.endproc

; Called regularly from the "C" code to process the "network"
.proc _plat_net_update

    ldx consumer_bank      ; get the "C" side bank
    lda consumer_size,x    ; get the size of this bank
    bne process_line       ; There is a line to process

    inc idle_counter       ; Nothing yet, so do idle processing
    bne check_pause        ; Idle processing is needed if a "line" didn't
    inc idle_counter+1     ; end with a '\n', such as "login: "
    lda idle_counter+1
    cmp #64
    bcc check_pause
    lda #0
    sta idle_counter+1

    ldx #%00000011         ; Disable interrupts mask
    stx command

    ldy receiver_size      ; size of receiver data in y
    beq idle_done          ; no partial line then done here
    ldx receiver_bank      ; make (partial line) receiver bank available to consumer
    tya
    sta consumer_size,x    ; Set the size of the bank for the consumer
    lda #0
    sta receiver_size      ; reset the receiver bank length
    inx                    ; move ot the next receiver bank
    txa
    and #NUM_RECEIVE_BANKS-1
    sta receiver_bank
    pha                    ; save this bank
    clc
    adc #>receive_buffer   ; point at the active bank in the NMI
    sta nmi_vec::save_rx+2
    pla                    ; get bank back
    adc #1                 ; check if consumer is lagging
    and #NUM_RECEIVE_BANKS-1
    tax
    lda consumer_size,x    ; is next bank still being processed by consumer?
    beq idle_done          ; empty so consumer is running okay
    lda #%00001011         ; receiver irq off mask
    sta command_mask

idle_done:
    lda command_mask       ; resume interrupts (if flow control not active)
    sta command
    bne check_pause        ; bra

process_line:
    pha                    ; save length (size) in y
    txa                    ; put bank in a
    clc
    adc #>receive_buffer   ; add hi
    tax                    ; to x
    lda #<receive_buffer   ; lo to a
    jsr pushax             ; push ptr to bank

    ldx #0                 ; hi 0
    pla                    ; length (size) to a (lo)
    jsr _fics_tcp_recv
    lda #0
    ldx consumer_bank
    sta consumer_size,x    ; reset the consumer bank line length to 0
    inx                    ; step to next consumer bank
    txa
    and #NUM_RECEIVE_BANKS-1
    sta consumer_bank

check_pause:
    lda command_mask
    and #%00000010
    beq send               ; flow control not active
    lda receiver_bank      ; start from the receiver
    clc
    adc #1
    and #NUM_RECEIVE_BANKS-1
    tax
    lda consumer_size,x    ; reader size
    bne send               ; still blocked
    lda #%00001001         ; enable receive irq
    sta command_mask
    sta command            ; and turn receive back on

send:
    ldy send_tail
    cpy send_head
    beq :+                 ; nothing to send
    lda status
    and #%00010000         ; Check if transmit is free
    beq :+                 ; Can't send so recv
    lda send_buffer,y      ; send the tail of the send queue
    sta data
    iny                    ; inc / wrap tail
    sty send_tail
:   rts
.endproc

; Called from "C" to add the string to send to the send circular buffer
.proc _sw_send
    sta length+1           ; set how many bytes to copy
    ldx #0                 ; from start of buffer
    ldy send_head          ; to the current head
:   lda _c64 + c64_t::send_buffer,x
    sta send_buffer,y      ; copy the byte
    inx
    iny
length:
    cpx #$ff               ; loop for all bytes
    bne :-
    sty send_head          ; set the new head position
    rts
.endproc

; Turn off the IRQs, remove the handler and turn off send, receive and IRQs
; in the ACIA
.proc _sw_shutdown
    lda #%00000010         ; Turn off IRQs (and transmitter/receiver)
    sta command
    sta status             ; Reset the chip
    sei                    ; remove the NMI handler
    lda old_nmi_vec
    sta NMIVec
    lda old_nmi_vec+1
    sta NMIVec+1
    cli
    rts
.endproc

.data
data_start:
; The "C" side of chess will consume these values through callback
consumer_bank:  .byte $00   ; bank the "C" side uses
consumer_size:              ; number of bytes in received string, per bank
    .repeat NUM_RECEIVE_BANKS
                .byte $00
    .endrepeat

; The NMI internally uses this to place received bytes
receiver_bank:  .byte $00   ; bank the NMI uses
receiver_size:  .byte $00   ; # bytes in NMI receive buffer

; Sender ring of 256 bytes
send_head:      .byte $00
send_tail:      .byte $00

; Used to see if a buffer was received that didn't end in a \n
idle_counter:   .byte $00, $00

; Used to control the ASIC recv IRQ for flow control
command_mask:    .byte %00001001 ; Mask that enables transmit and recv (recv irq also)

; For chaining and unhooking, the original NMI vector
old_nmi_vec:    .res 2

; send buffer is a ring of 256 bytes (I won't send anything nearly as long)
send_buffer:    .res 256   ; Shared send buffer

; Shared receive buffer [in banks of 256 bytes]
receive_buffer: .res NUM_RECEIVE_BANKS * 256
data_end:
