.data
.balign 1
font_tiles:
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 102
	.db 0
	.db 102
	.db 0
	.db 102
	.db 0
	.db 126
	.db 0
	.db 102
	.db 0
	.db 102
	.db 0
	.db 102
	.db 0
	.db 0
	.db 0
	.db 126
	.db 0
	.db 96
	.db 0
	.db 96
	.db 0
	.db 124
	.db 0
	.db 96
	.db 0
	.db 96
	.db 0
	.db 126
	.db 0
	.db 0
	.db 0
	.db 96
	.db 0
	.db 96
	.db 0
	.db 96
	.db 0
	.db 96
	.db 0
	.db 96
	.db 0
	.db 96
	.db 0
	.db 126
	.db 0
	.db 0
	.db 0
	.db 60
	.db 0
	.db 102
	.db 0
	.db 102
	.db 0
	.db 102
	.db 0
	.db 102
	.db 0
	.db 102
	.db 0
	.db 60
	.db 0
	.db 0
	.db 0
	.db 198
	.db 0
	.db 198
	.db 0
	.db 198
	.db 0
	.db 214
	.db 0
	.db 254
	.db 0
	.db 238
	.db 0
	.db 198
	.db 0
	.db 0
	.db 0
	.db 124
	.db 0
	.db 102
	.db 0
	.db 102
	.db 0
	.db 124
	.db 0
	.db 108
	.db 0
	.db 102
	.db 0
	.db 102
	.db 0
	.db 0
	.db 0
	.db 120
	.db 0
	.db 108
	.db 0
	.db 102
	.db 0
	.db 102
	.db 0
	.db 102
	.db 0
	.db 108
	.db 0
	.db 120
	.db 0
	.db 0
	.db 0
	.db 24
	.db 0
	.db 24
	.db 0
	.db 24
	.db 0
	.db 24
	.db 0
	.db 24
	.db 0
	.db 0
	.db 0
	.db 24
	.db 0
	.db 0
	.db 0
/* end data */

.data
.balign 1
message:
	.db 1
	.db 2
	.db 3
	.db 3
	.db 4
	.db 0
	.db 5
	.db 4
	.db 6
	.db 3
	.db 7
	.db 8
	.db 255
/* end data */


; Function: main (framesize=46, slots=22, alloc_slots=3)
; temp 64: slot=-1, alloc=0
; temp 65: slot=-1, alloc=1
; temp 66: slot=-1, alloc=2
; temp 67: slot=-1, alloc=-1
; temp 68: slot=-1, alloc=-1
; temp 69: slot=-1, alloc=-1
; temp 70: slot=-1, alloc=-1
; temp 71: slot=-1, alloc=-1
; temp 72: slot=-1, alloc=-1
; temp 73: slot=-1, alloc=-1
; temp 74: slot=7, alloc=-1
; temp 75: slot=-1, alloc=-1
; temp 76: slot=8, alloc=-1
; temp 77: slot=-1, alloc=-1
; temp 78: slot=9, alloc=-1
; temp 79: slot=10, alloc=-1
; temp 80: slot=-1, alloc=-1
; temp 81: slot=-1, alloc=-1
; temp 82: slot=11, alloc=-1
; temp 83: slot=12, alloc=-1
; temp 84: slot=-1, alloc=-1
; temp 85: slot=13, alloc=-1
; temp 86: slot=14, alloc=-1
; temp 87: slot=-1, alloc=-1
; temp 88: slot=-1, alloc=-1
; temp 89: slot=6, alloc=-1
; temp 90: slot=-1, alloc=-1
; temp 91: slot=-1, alloc=-1
; temp 92: slot=-1, alloc=-1
; temp 93: slot=-1, alloc=-1
; temp 94: slot=-1, alloc=-1
; temp 95: slot=-1, alloc=-1
; temp 96: slot=-1, alloc=-1
; temp 97: slot=-1, alloc=-1
; temp 98: slot=16, alloc=-1
; temp 99: slot=-1, alloc=-1
; temp 100: slot=-1, alloc=-1
; temp 101: slot=-1, alloc=-1
; temp 102: slot=15, alloc=-1
; temp 103: slot=-1, alloc=-1
; temp 104: slot=-1, alloc=-1
; temp 105: slot=-1, alloc=-1
; temp 106: slot=-1, alloc=-1
; temp 107: slot=-1, alloc=-1
; temp 108: slot=-1, alloc=-1
; temp 109: slot=-1, alloc=-1
; temp 110: slot=18, alloc=-1
; temp 111: slot=-1, alloc=-1
; temp 112: slot=19, alloc=-1
; temp 113: slot=20, alloc=-1
; temp 114: slot=-1, alloc=-1
; temp 115: slot=-1, alloc=-1
; temp 116: slot=21, alloc=-1
; temp 117: slot=-1, alloc=-1
; temp 118: slot=-1, alloc=-1
; temp 119: slot=-1, alloc=-1
; temp 120: slot=-1, alloc=-1
; temp 121: slot=17, alloc=-1
; temp 122: slot=-1, alloc=-1
; temp 123: slot=-1, alloc=-1
; temp 124: slot=-1, alloc=-1
; temp 125: slot=3, alloc=-1
; temp 126: slot=-1, alloc=-1
; temp 127: slot=4, alloc=-1
; temp 128: slot=-1, alloc=-1
; temp 129: slot=5, alloc=-1
; temp 130: slot=-1, alloc=-1
; temp 131: slot=-1, alloc=-1
; temp 132: slot=-1, alloc=-1
; temp 133: slot=-1, alloc=-1
.SECTION ".text.main" SUPERFREE
main:
	php
	rep #$30
	tsa
	sec
	sbc.w #46
	tas
@start.1:
	jmp @body.2
@body.2:
	lda.w #0
	sep #$20
	sta.l $002105
	rep #$20
	lda.w #4
	sep #$20
	sta.l $002107
	rep #$20
	lda.w #0
	sep #$20
	sta.l $00210B
	rep #$20
	lda.w #128
	sep #$20
	sta.l $002115
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002116
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002117
	rep #$20
	lda.w #0
	sta 8,s
	jmp @for_cond.3
@for_cond.3:
	lda 8,s
	cmp.w #144
	bcc +
	lda.w #0
	bra ++
+	lda.w #1
++
	sta 16,s
	lda 16,s
	bne +
	jmp @for_join.6
+
	jmp @for_body.4
@for_body.4:
	lda 8,s
	sta 18,s
	lda 18,s
	clc
	adc.w #font_tiles
	sta 20,s
	lda 20,s
	tax
	sep #$20
	lda.l $0000,x
	rep #$20
	and.w #$00FF
	sta 22,s
	lda 22,s
	sep #$20
	sta.l $002118
	rep #$20
	lda 8,s
	clc
	adc.w #1
	sta 24,s
	lda 24,s
	sta 26,s
	lda 26,s
	clc
	adc.w #font_tiles
	sta 28,s
	lda 28,s
	tax
	sep #$20
	lda.l $0000,x
	rep #$20
	and.w #$00FF
	sta 30,s
	lda 30,s
	sep #$20
	sta.l $002119
	rep #$20
	jmp @for_cont.5
@for_cont.5:
	lda 8,s
	clc
	adc.w #2
	sta 14,s
	lda 14,s
	sta 8,s
	jmp @for_cond.3
@for_join.6:
	lda.w #0
	sep #$20
	sta.l $002121
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002122
	rep #$20
	lda.w #40
	sep #$20
	sta.l $002122
	rep #$20
	lda.w #255
	sep #$20
	sta.l $002122
	rep #$20
	lda.w #127
	sep #$20
	sta.l $002122
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002116
	rep #$20
	lda.w #4
	sep #$20
	sta.l $002117
	rep #$20
	lda.w #0
	sta 10,s
	jmp @for_cond.7
@for_cond.7:
	lda 10,s
	cmp.w #1024
	bcc +
	lda.w #0
	bra ++
+	lda.w #1
++
	sta 34,s
	lda 34,s
	bne +
	jmp @for_join.10
+
	jmp @for_body.8
@for_body.8:
	lda.w #0
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	jmp @for_cont.9
@for_cont.9:
	lda 10,s
	clc
	adc.w #1
	sta 32,s
	lda 32,s
	sta 10,s
	jmp @for_cond.7
@for_join.10:
	lda.w #202
	sep #$20
	sta.l $002116
	rep #$20
	lda.w #5
	sep #$20
	sta.l $002117
	rep #$20
	lda.w #0
	sta 12,s
	jmp @while_cond.11
@while_cond.11:
	jmp @while_body.12
@while_body.12:
	lda 12,s
	sta 38,s
	lda 38,s
	clc
	adc.w #message
	sta 40,s
	lda 40,s
	tax
	sep #$20
	lda.l $0000,x
	rep #$20
	and.w #$00FF
	sta 42,s
	lda 42,s
	cmp.w #255
	beq +
	lda.w #0
	bra ++
+	lda.w #1
++
	sta 44,s
	lda 44,s
	bne +
	jmp @if_false.15
+
	jmp @if_true.14
@if_false.15:
	lda 42,s
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	lda 12,s
	clc
	adc.w #1
	sta 36,s
	lda 36,s
	sta 12,s
	jmp @while_cond.11
@if_true.14:
	jmp @while_join.13
@while_join.13:
	lda.w #1
	sep #$20
	sta.l $00212C
	rep #$20
	lda.w #15
	sep #$20
	sta.l $002100
	rep #$20
	jmp @while_cond.16
@while_cond.16:
	jmp @while_body.17
@while_body.17:
	jmp @while_cond.16
.ENDS
/* end function main */


; End of generated code
