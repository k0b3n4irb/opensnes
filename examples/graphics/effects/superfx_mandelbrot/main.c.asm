.SECTION ".rodata.1" SUPERFREE
fractal_pal:
	.dw 0
	.dw 10240
	.dw 20480
	.dw 31744
	.dw 32256
	.dw 32736
	.dw 17376
	.dw 992
	.dw 1008
	.dw 1023
	.dw 543
	.dw 31
	.dw 16415
	.dw 31775
	.dw 21140
	.dw 32767
.ENDS
/* end data */


; Function: main (framesize=8, slots=3, alloc_slots=0, fn_leaf=0, leaf_opt=0)
; temp 64: slot=0, alloc=-1
; temp 65: slot=1, alloc=-1
; temp 66: slot=2, alloc=-1
; temp 67: slot=-1, alloc=-1
.SECTION ".text.main" SUPERFREE
.ACCU 16
.INDEX 16
main:
	rep #$20
	tsa
	sec
	sbc.w #8
	tas
@start.1:
@body.2:
	jsl consoleInit
	pea.w 1
	pea.w 0
	jsl setMode
	tsa
	clc
	adc.w #4
	tas
	pea.w fractal_pal
	pea.w 0
	pea.w 32
	jsl dmaCopyCGram
	tsa
	clc
	adc.w #6
	tas
	pea.w 0
	pea.w 0
	jsl bgSetGfxPtr
	tsa
	clc
	adc.w #4
	tas
	pea.w 0
	pea.w 16384
	pea.w 0
	jsl bgSetMapPtr
	tsa
	clc
	adc.w #6
	tas
	jsl setupBitmapTilemap
	jsl gsuInit
	and.w #$00FF
	cmp.w #0
	bne +
	jmp @if_true.3
+
@if_false.4:
	jsl launchGSU
	jsl WaitForVBlank
	jsl setScreenOff
	jsl dmaBitmapToVRAM
	sep #$20
	lda #1
	sta.l $00212C
	rep #$20
	jsl setScreenOn
@while_cond.8:
@while_body.9:
	jsl WaitForVBlank
	jmp @while_cond.8
@if_true.3:
	jsl setScreenOn
@while_cond.5:
@while_body.6:
	jsl WaitForVBlank
	jmp @while_cond.5
.ENDS
/* end function main */


; End of generated code
