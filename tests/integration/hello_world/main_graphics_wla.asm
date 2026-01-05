.SECTION ".rodata.1" SUPERFREE

font_D:
	.db 124
	.db 66
	.db 66
	.db 66
	.db 66
	.db 66
	.db 124
	.db 0
.ENDS

.SECTION ".rodata.2" SUPERFREE

font_E:
	.db 126
	.db 64
	.db 64
	.db 124
	.db 64
	.db 64
	.db 126
	.db 0
.ENDS

.SECTION ".rodata.3" SUPERFREE

font_H:
	.db 66
	.db 66
	.db 66
	.db 126
	.db 66
	.db 66
	.db 66
	.db 0
.ENDS

.SECTION ".rodata.4" SUPERFREE

font_L:
	.db 64
	.db 64
	.db 64
	.db 64
	.db 64
	.db 64
	.db 126
	.db 0
.ENDS

.SECTION ".rodata.5" SUPERFREE

font_O:
	.db 60
	.db 66
	.db 66
	.db 66
	.db 66
	.db 66
	.db 60
	.db 0
.ENDS

.SECTION ".rodata.6" SUPERFREE

font_R:
	.db 124
	.db 66
	.db 66
	.db 124
	.db 72
	.db 68
	.db 66
	.db 0
.ENDS

.SECTION ".rodata.7" SUPERFREE

font_W:
	.db 66
	.db 66
	.db 66
	.db 66
	.db 90
	.db 102
	.db 66
	.db 0
.ENDS

.SECTION ".rodata.8" SUPERFREE

font_X:
	.db 24
	.db 24
	.db 24
	.db 24
	.db 0
	.db 0
	.db 24
	.db 0
.ENDS

.SECTION ".rodata.9" SUPERFREE

font_SP:
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
	.db 0
.ENDS


.SECTION ".text.10" SUPERFREE
write_tile:
	php
	rep #$30
	tsa
	sec
	sbc.w #20
	tas
@start.1:
	lda 25,s
	sta 8,s
	jmp @body.2
@body.2:
	lda.w #0
	sta 6,s
	jmp @for_cond.3
@for_cond.3:
	lda 6,s
	cmp.w #8
	bmi +
	lda.w #0
	bra ++
+	lda.w #1
++
	sta 12,s
	lda 12,s
	bne +
	jmp @for_join.6
+
	jmp @for_body.4
@for_body.4:
	lda 6,s
	sta 14,s
	lda 8,s
	clc
	adc 14,s
	sta 16,s
	lda 16,s
	tax
	sep #$20
	lda.l $0000,x
	rep #$20
	and.w #$00FF
	sta 18,s
	lda 18,s
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	jmp @for_cont.5
@for_cont.5:
	lda 6,s
	clc
	adc.w #1
	sta 10,s
	lda 10,s
	sta 6,s
	jmp @for_cond.3
@for_join.6:
	tsa
	clc
	adc.w #20
	tas
	plp
	rtl
.ENDS


.SECTION ".text.11" SUPERFREE
main:
	php
	rep #$30
	tsa
	sec
	sbc.w #10
	tas
@start.7:
	jmp @body.8
@body.8:
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
	lda.w #font_D
	pha
	jsl write_tile
	tsa
	clc
	adc.w #2
	tas
	lda.w #font_E
	pha
	jsl write_tile
	tsa
	clc
	adc.w #2
	tas
	lda.w #font_H
	pha
	jsl write_tile
	tsa
	clc
	adc.w #2
	tas
	lda.w #font_L
	pha
	jsl write_tile
	tsa
	clc
	adc.w #2
	tas
	lda.w #font_O
	pha
	jsl write_tile
	tsa
	clc
	adc.w #2
	tas
	lda.w #font_R
	pha
	jsl write_tile
	tsa
	clc
	adc.w #2
	tas
	lda.w #font_W
	pha
	jsl write_tile
	tsa
	clc
	adc.w #2
	tas
	lda.w #font_X
	pha
	jsl write_tile
	tsa
	clc
	adc.w #2
	tas
	lda.w #font_SP
	pha
	jsl write_tile
	tsa
	clc
	adc.w #2
	tas
	lda.w #0
	sep #$20
	sta.l $002116
	rep #$20
	lda.w #8
	sep #$20
	sta.l $002117
	rep #$20
	lda.w #0
	sta 4,s
	jmp @for_cond.9
@for_cond.9:
	lda 4,s
	cmp.w #1024
	bmi +
	lda.w #0
	bra ++
+	lda.w #1
++
	sta 8,s
	lda 8,s
	bne +
	jmp @for_join.12
+
	jmp @for_body.10
@for_body.10:
	lda.w #8
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	jmp @for_cont.11
@for_cont.11:
	lda 4,s
	clc
	adc.w #1
	sta 6,s
	lda 6,s
	sta 4,s
	jmp @for_cond.9
@for_join.12:
	lda.w #202
	sep #$20
	sta.l $002116
	rep #$20
	lda.w #9
	sep #$20
	sta.l $002117
	rep #$20
	lda.w #2
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	lda.w #1
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	lda.w #3
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	lda.w #3
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	lda.w #4
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	lda.w #8
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	lda.w #6
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	lda.w #4
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	lda.w #5
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	lda.w #3
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	lda.w #7
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
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
	lda.w #1
	sep #$20
	sta.l $00212C
	rep #$20
	lda.w #15
	sep #$20
	sta.l $002100
	rep #$20
	jmp @while_cond.13
@while_cond.13:
	jmp @while_body.14
@while_body.14:
	jmp @while_cond.13
.ENDS


