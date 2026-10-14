;************************************************
; snesmod soundbank data                        *
; total size:      16990 bytes                  *
;************************************************

.BANK 1
.ORG 0
.SECTION "SOUNDBANK" FORCE ; need dedicated bank(s)

soundbank:
.incbin "soundbank.bnk"
.ENDS
