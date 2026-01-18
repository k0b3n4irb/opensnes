;==============================================================================
; Sound Effects Demo - BRR Sample Data
;==============================================================================

.SECTION ".brr_data" SUPERFREE

;------------------------------------------------------------------------------
; Beep sample - A simple square wave tone (BRR format)
; 16 blocks = 144 bytes, produces a short beep sound
;------------------------------------------------------------------------------
beep_brr:
    ; Block 1 - (range=10, filter=0, no loop, not end)
    .db $A0
    .db $01, $23, $45, $67, $76, $54, $32, $10

    ; Block 2
    .db $A0
    .db $FE, $DC, $BA, $98, $89, $AB, $CD, $EF

    ; Block 3
    .db $A0
    .db $01, $23, $45, $67, $76, $54, $32, $10

    ; Block 4
    .db $A0
    .db $FE, $DC, $BA, $98, $89, $AB, $CD, $EF

    ; Block 5
    .db $A0
    .db $01, $23, $45, $67, $76, $54, $32, $10

    ; Block 6
    .db $A0
    .db $FE, $DC, $BA, $98, $89, $AB, $CD, $EF

    ; Block 7
    .db $A0
    .db $01, $23, $45, $67, $76, $54, $32, $10

    ; Block 8
    .db $A0
    .db $FE, $DC, $BA, $98, $89, $AB, $CD, $EF

    ; Block 9
    .db $A0
    .db $01, $23, $45, $67, $76, $54, $32, $10

    ; Block 10
    .db $A0
    .db $FE, $DC, $BA, $98, $89, $AB, $CD, $EF

    ; Block 11
    .db $A0
    .db $01, $23, $45, $67, $76, $54, $32, $10

    ; Block 12
    .db $A0
    .db $FE, $DC, $BA, $98, $89, $AB, $CD, $EF

    ; Block 13
    .db $A0
    .db $01, $23, $45, $67, $76, $54, $32, $10

    ; Block 14
    .db $A0
    .db $FE, $DC, $BA, $98, $89, $AB, $CD, $EF

    ; Block 15 - Decay
    .db $80
    .db $00, $11, $22, $33, $22, $11, $00, $00

    ; Block 16 - End (range=8, filter=0, no loop, END flag)
    .db $81
    .db $00, $00, $00, $00, $00, $00, $00, $00

beep_brr_end:

.ENDS
