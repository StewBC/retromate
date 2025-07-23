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

.export _sw_init, _sw_send, _sw_shutdown, _plat_net_update
.import _c64, _fics_tcp_recv, pusha, pushax

.struct c64_t
    rop_line               .res 2*7     ; 14 bytes
    rop_color              .res 2*2     ; 4 bytes
    draw_colors            .res 4       ; uint32_t
    terminal_display_width .res 1
    send_buffer            .res 80*24   ; I need access to this buffer
    status_log_buffer      .res 13*25
.endstruct

base        = $DE00         ; base ACIA address
data        = base          ; read / write port
status      = base+1        ; ready to receive / send
command     = base+2        ; parity, echo, irq's and rx/tx on/off
control     = base+3        ; stop bits / baud
turbo232    = $DE07         ; turbo232 baud select

NUM_RECEIVE_BANKS  = 4      ; Each bank is 256 bytes so keep low, and pow-2

.code

; Start the ACIA with recv and xmit on, irq's off
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

    lda #%00010000          ; Control Register Mask
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

    lda #0                  ;-----------------------------------------------------------
                            ; Enhanced register $DE07 (Turbo232)
                            ; Bit  7-3: Unused
                            ; Bit  2  : Mode Bit (read only): 1 = Bits 0-3 of $DE03 are
                            ;         : cleared and enhaced speed is enabled
                            ; Bits 1-0: Enhanced Baud Rate (read only if Mode Bit = 0):
                            ;         :            00 = 230400 Bd | 10 = 57600 Bd
                            ;         :            01 = 115200 Bd |
                            ;         :            11 = reserved for future expansions
    sta turbo232

    lda #%00001011          ; Command Register Mask
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
    sta command
    rts
.endproc

; Called regularly from the "C" code to process the "network"
.proc _plat_net_update
    ldx receiver_bank
    lda consumer_bank,x     ; is the consumer overloaded
    beq read_byte           ; no - it can handle input
    jmp send
read_byte:
    lda status              ; see if there's a byte can be read
    and #%00001000
    beq idle_check          ; no, see about an idle timeout
    ldy receiver_size       ; where to put the byte to be received
    lda data                ; receive the byte
save_rx: 
    sta receive_buffer,y    ; put it in the bank
    iny
    sty receiver_size       ; set index for next byte to receive

    cmp #$20                ; Any char before ' ' is an eol for me
    bcc line_end
    cmp #$25                ; '%' (fics% also means end of line)
    bne read_byte

line_end:
    cpy #2                  ; empty lines (just a line-feed) get ignored
    bcs line_ok
    ldy #0                  ; reset size to 0 for the receiver
    sty receiver_size
    beq read_byte
line_ok:
    clc
    tya                     ; line-length in a
    ldx receiver_bank       ; start from the receiver's bank
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
    tya                     ; line length to a
    pha                     ; save length (size) in y
    lda consumer_bank       ; put bank in a
    clc
    adc #>receive_buffer    ; add hi
    tax                     ; to x
    lda #<receive_buffer    ; lo to a
    jsr pushax              ; push ptr to bank
    ldx #0                  ; hi 0
    pla                     ; length (size) to a (lo)
    jsr _fics_tcp_recv
    lda #0
    ldx consumer_bank
    sta consumer_size,x     ; reset the consumer bank line length to 0
    sta idle_counter        ; also reset the idle counter
    sta idle_counter+1
    inx                     ; step to next consumer bank
    txa
    and #NUM_RECEIVE_BANKS-1
    sta consumer_bank
    jmp _plat_net_update    ; and see if there's something more to receive

idle_check:
    ldy receiver_size       ; receibed but not consumed?
    beq send                ; no
    inc idle_counter        ; tick the counter
    bne send
    inc idle_counter+1
    lda idle_counter+1
    cmp #32                 ; emphirically decided
    bcc send                ; not yet idle timeout
    lda #0                  ; clear the timer
    sta idle_counter+1
    beq line_end            ; send the line to consumer

send:
    ldy send_tail
    cpy send_head
    beq done                ; nothing to send
    lda status
    and #%00010000          ; Check if transmit is free
    beq done                ; Can't send so recv
    lda send_buffer,y       ; send the tail of the send queue
    sta data
    iny                     ; inc / wrap tail
    sty send_tail
done: 
    rts
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

; Reset the SwiftLink (closes open connections)
.proc _sw_shutdown
    sta status             ; Reset the chip (any write)
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

; The receiver uses this to place received bytes
receiver_bank:  .byte $00   ; bank the receiver will use in receive_buffer
receiver_size:  .byte $00   ; # bytes in bank, in the receive buffer

; Sender ring of 256 bytes
send_head:      .byte $00
send_tail:      .byte $00

; Used to see if a buffer was received that didn't end in a \n
idle_counter:   .byte $00, $00

; send buffer is a ring of 256 bytes (I won't send anything nearly as long)
send_buffer:    .res 256   ; Shared send buffer

; Shared receive buffer [in banks of 256 bytes]
receive_buffer: .res NUM_RECEIVE_BANKS * 256
data_end:
