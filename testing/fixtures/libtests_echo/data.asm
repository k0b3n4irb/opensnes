; 16 KB with no zero byte: what the echo ring writes when it runs over a
; sample is zeros (nothing plays), so the fixture can tell a sample that was
; written over from one that was not.
ASSET_SECTION "echo_fill"
echo_fill:
    .DSB $4000, $5A
.ENDS
