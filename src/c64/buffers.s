;
;  buffers.s
;  RetroMate
;
;  By Oliver Schmidt, 2025.
;  Adapted by S. Wessels, June 2025
;

.segment "DATA"

.export eth_inp
eth_inp:             .res 1518

.export eth_outp
eth_outp:            .res 1518

.export output_buffer
output_buffer:       .res 520

.export _terminal_log_buffer
_terminal_log_buffer: .res (80 * 23)

.export _status_log_buffer
_status_log_buffer:   .res (13 * 24)
