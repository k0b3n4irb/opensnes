; A6 far-pointer fixture data — placed in BANK 2 so its address has a non-zero
; bank byte. The C side derefs a pointer to it; with the bank-$00-hardcoded
; deref codegen (`lda.l $0000,x`) this reads bank $00 garbage instead.
.BANK 2 SLOT 0
.SECTION "a6_sentinel" SEMIFREE
a6_sentinel:
    .db $11, $22, $33, $44, $55, $66, $77, $88
.ENDS
