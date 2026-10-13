; 16 KB with no zero byte: what the echo ring writes when it runs over a
; sample is zeros (nothing plays), so the fixture can tell a sample that was
; written over from one that was not.
ASSET_SECTION "echo_fill"
echo_fill:
    .DSB $4000, $5A
.ENDS

; 1000 bytes that differ from one to the next: a piece sent to the wrong
; place, or twice, shows.
ASSET_SECTION "step_pat"
step_pat:
    .REPT 1000 INDEX N
    .DB ((N * 7 + 3) & 255) | 1
    .ENDR
.ENDS
