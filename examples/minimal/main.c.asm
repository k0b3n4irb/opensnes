
; Function: add (framesize=12, slots=5, alloc_slots=2)
; temp 64: slot=2, alloc=-1
; temp 65: slot=3, alloc=-1
; temp 66: slot=-1, alloc=0
; temp 67: slot=-1, alloc=1
; temp 68: slot=-1, alloc=-1
; temp 69: slot=-1, alloc=-1
; temp 70: slot=4, alloc=-1
.SECTION ".text.add" SUPERFREE
add:
	php
	rep #$30
	tsa
	sec
	sbc.w #12
	tas
@start.1:
	lda 19,s
	sta 6,s
	lda 17,s
	sta 8,s
	jmp @body.2
@body.2:
	lda 6,s
	clc
	adc 8,s
	sta 10,s
	lda 10,s
	tsa
	clc
	adc.w #12
	tas
	plp
	rtl
.ENDS
/* end function add */


; Function: sum_to_n (framesize=20, slots=9, alloc_slots=3)
; temp 64: slot=5, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=1
; temp 67: slot=-1, alloc=2
; temp 68: slot=-1, alloc=-1
; temp 69: slot=-1, alloc=-1
; temp 70: slot=8, alloc=-1
; temp 71: slot=-1, alloc=-1
; temp 72: slot=-1, alloc=-1
; temp 73: slot=6, alloc=-1
; temp 74: slot=-1, alloc=-1
; temp 75: slot=7, alloc=-1
; temp 76: slot=-1, alloc=-1
; temp 77: slot=-1, alloc=-1
; temp 78: slot=-1, alloc=-1
; temp 79: slot=3, alloc=-1
; temp 80: slot=4, alloc=-1
; temp 81: slot=-1, alloc=-1
; temp 82: slot=-1, alloc=-1
.SECTION ".text.sum_to_n" SUPERFREE
sum_to_n:
	php
	rep #$30
	tsa
	sec
	sbc.w #20
	tas
@start.3:
	lda 25,s
	sta 12,s
	jmp @body.4
@body.4:
	lda.w #0
	sta 8,s
	lda.w #1
	sta 10,s
	jmp @for_cond.5
@for_cond.5:
	lda 12,s
	cmp 10,s
	bmi +
	lda.w #1
	bra ++
+	lda.w #0
++
	sta 18,s
	lda 18,s
	bne +
	jmp @for_join.8
+
	jmp @for_body.6
@for_body.6:
	jmp @for_cont.7
@for_cont.7:
	lda 10,s
	clc
	adc.w #1
	sta 16,s
	lda 8,s
	clc
	adc 10,s
	sta 14,s
	lda 14,s
	sta 8,s
	lda 16,s
	sta 10,s
	jmp @for_cond.5
@for_join.8:
	lda 8,s
	tsa
	clc
	adc.w #20
	tas
	plp
	rtl
.ENDS
/* end function sum_to_n */


; Function: main (framesize=16, slots=7, alloc_slots=4)
; temp 64: slot=-1, alloc=0
; temp 65: slot=-1, alloc=1
; temp 66: slot=-1, alloc=2
; temp 67: slot=-1, alloc=3
; temp 68: slot=-1, alloc=-1
; temp 69: slot=-1, alloc=-1
; temp 70: slot=4, alloc=-1
; temp 71: slot=5, alloc=-1
; temp 72: slot=-1, alloc=-1
; temp 73: slot=-1, alloc=-1
; temp 74: slot=-1, alloc=-1
; temp 75: slot=6, alloc=-1
.SECTION ".text.main" SUPERFREE
main:
	php
	rep #$30
	tsa
	sec
	sbc.w #16
	tas
@start.9:
	jmp @body.10
@body.10:
	lda.w #10
	pha
	lda.w #5
	pha
	jsl add
	tax
	tsa
	clc
	adc.w #4
	tas
	txa
	sta 10,s
	lda.w #5
	pha
	jsl sum_to_n
	tax
	tsa
	clc
	adc.w #2
	tas
	txa
	sta 12,s
	lda.w #15
	sep #$20
	sta.l $002100
	rep #$20
	lda 10,s
	clc
	adc 12,s
	sta 14,s
	lda 14,s
	tsa
	clc
	adc.w #16
	tas
	plp
	rtl
.ENDS
/* end function main */


; End of generated code
