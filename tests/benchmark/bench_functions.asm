
; Function: empty_func (framesize=0, slots=0, alloc_slots=0, fn_leaf=1, leaf_opt=1)
.SECTION ".text.empty_func" SUPERFREE
.ACCU 16
.INDEX 16
empty_func:
	rep #$20
@start.1:
@body.2:
	lda.w #0
	rtl
.ENDS
/* end function empty_func */


; Function: add_u16 (framesize=0, slots=7, alloc_slots=2, fn_leaf=1, leaf_opt=1)
; temp 64: slot=2, alloc=-1
; temp 65: slot=3, alloc=-1
; temp 66: slot=-1, alloc=0
; temp 67: slot=-1, alloc=1
; temp 68: slot=-1, alloc=-1
; temp 69: slot=4, alloc=-1
; temp 70: slot=-1, alloc=-1
; temp 71: slot=5, alloc=-1
; temp 72: slot=6, alloc=-1
.SECTION ".text.add_u16" SUPERFREE
.ACCU 16
.INDEX 16
add_u16:
	rep #$20
@start.3:
@body.4:
	lda 6,s
	clc
	adc 4,s
	rtl
.ENDS
/* end function add_u16 */


; Function: sub_u16 (framesize=0, slots=7, alloc_slots=2, fn_leaf=1, leaf_opt=1)
; temp 64: slot=2, alloc=-1
; temp 65: slot=3, alloc=-1
; temp 66: slot=-1, alloc=0
; temp 67: slot=-1, alloc=1
; temp 68: slot=-1, alloc=-1
; temp 69: slot=4, alloc=-1
; temp 70: slot=-1, alloc=-1
; temp 71: slot=5, alloc=-1
; temp 72: slot=6, alloc=-1
.SECTION ".text.sub_u16" SUPERFREE
.ACCU 16
.INDEX 16
sub_u16:
	rep #$20
@start.5:
@body.6:
	lda 6,s
	sec
	sbc 4,s
	rtl
.ENDS
/* end function sub_u16 */


; Function: mul_const_13 (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.mul_const_13" SUPERFREE
.ACCU 16
.INDEX 16
mul_const_13:
	rep #$20
@start.7:
@body.8:
	lda 4,s
	sta.b tcc__r9
	asl a
	clc
	adc.b tcc__r9
	asl a
	asl a
	clc
	adc.b tcc__r9
	rtl
.ENDS
/* end function mul_const_13 */


; Function: mul_const_8 (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.mul_const_8" SUPERFREE
.ACCU 16
.INDEX 16
mul_const_8:
	rep #$20
@start.9:
@body.10:
	lda 4,s
	asl a
	asl a
	asl a
	rtl
.ENDS
/* end function mul_const_8 */


; Function: div_const_10 (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.div_const_10" SUPERFREE
.ACCU 16
.INDEX 16
div_const_10:
	rep #$20
@start.11:
@body.12:
	lda 4,s
	sta.b tcc__r0
	lda.w #10
	sta.b tcc__r1
	jsl __div16
	rtl
.ENDS
/* end function div_const_10 */


; Function: mod_const_10 (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.mod_const_10" SUPERFREE
.ACCU 16
.INDEX 16
mod_const_10:
	rep #$20
@start.13:
@body.14:
	lda 4,s
	sta.b tcc__r0
	lda.w #10
	sta.b tcc__r1
	jsl __mod16
	rtl
.ENDS
/* end function mod_const_10 */


; Function: shift_left_3 (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.shift_left_3" SUPERFREE
.ACCU 16
.INDEX 16
shift_left_3:
	rep #$20
@start.15:
@body.16:
	lda 4,s
	asl a
	asl a
	asl a
	rtl
.ENDS
/* end function shift_left_3 */


; Function: shift_right_4 (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.shift_right_4" SUPERFREE
.ACCU 16
.INDEX 16
shift_right_4:
	rep #$20
@start.17:
@body.18:
	lda 4,s
	lsr a
	lsr a
	lsr a
	lsr a
	rtl
.ENDS
/* end function shift_right_4 */


; Function: bitwise_and (framesize=0, slots=7, alloc_slots=2, fn_leaf=1, leaf_opt=1)
; temp 64: slot=2, alloc=-1
; temp 65: slot=3, alloc=-1
; temp 66: slot=-1, alloc=0
; temp 67: slot=-1, alloc=1
; temp 68: slot=-1, alloc=-1
; temp 69: slot=4, alloc=-1
; temp 70: slot=-1, alloc=-1
; temp 71: slot=5, alloc=-1
; temp 72: slot=6, alloc=-1
.SECTION ".text.bitwise_and" SUPERFREE
.ACCU 16
.INDEX 16
bitwise_and:
	rep #$20
@start.19:
@body.20:
	lda 6,s
	and 4,s
	rtl
.ENDS
/* end function bitwise_and */


; Function: bitwise_or (framesize=0, slots=7, alloc_slots=2, fn_leaf=1, leaf_opt=1)
; temp 64: slot=2, alloc=-1
; temp 65: slot=3, alloc=-1
; temp 66: slot=-1, alloc=0
; temp 67: slot=-1, alloc=1
; temp 68: slot=-1, alloc=-1
; temp 69: slot=4, alloc=-1
; temp 70: slot=-1, alloc=-1
; temp 71: slot=5, alloc=-1
; temp 72: slot=6, alloc=-1
.SECTION ".text.bitwise_or" SUPERFREE
.ACCU 16
.INDEX 16
bitwise_or:
	rep #$20
@start.21:
@body.22:
	lda 6,s
	ora 4,s
	rtl
.ENDS
/* end function bitwise_or */


; Function: conditional (framesize=0, slots=7, alloc_slots=2, fn_leaf=1, leaf_opt=1)
; temp 64: slot=2, alloc=-1
; temp 65: slot=3, alloc=-1
; temp 66: slot=-1, alloc=0
; temp 67: slot=-1, alloc=1
; temp 68: slot=-1, alloc=-1
; temp 69: slot=4, alloc=-1
; temp 70: slot=-1, alloc=-1
; temp 71: slot=5, alloc=-1
; temp 72: slot=6, alloc=-1
; temp 73: slot=-1, alloc=-1
; temp 74: slot=-1, alloc=-1
.SECTION ".text.conditional" SUPERFREE
.ACCU 16
.INDEX 16
conditional:
	rep #$20
@start.23:
@body.24:
	lda 4,s
	cmp 6,s
	bcs +
	jmp @if_true.25
+
@if_false.26:
	lda 4,s
	rtl
@if_true.25:
	lda 6,s
	rtl
.ENDS
/* end function conditional */


; Function: loop_sum (framesize=26, slots=12, alloc_slots=3, fn_leaf=1, leaf_opt=1)
; temp 64: slot=6, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=1
; temp 67: slot=-1, alloc=2
; temp 68: slot=-1, alloc=-1
; temp 69: slot=9, alloc=-1
; temp 70: slot=-1, alloc=-1
; temp 71: slot=-1, alloc=-1
; temp 72: slot=10, alloc=-1
; temp 73: slot=-1, alloc=-1
; temp 74: slot=11, alloc=-1
; temp 75: slot=-1, alloc=-1
; temp 76: slot=-1, alloc=-1
; temp 77: slot=8, alloc=-1
; temp 78: slot=-1, alloc=-1
; temp 79: slot=7, alloc=-1
; temp 80: slot=-1, alloc=-1
; temp 81: slot=-1, alloc=-1
; temp 82: slot=-1, alloc=-1
; temp 83: slot=3, alloc=-1
; temp 84: slot=4, alloc=-1
; temp 85: slot=-1, alloc=-1
; temp 86: slot=-1, alloc=-1
; temp 87: slot=5, alloc=-1
.SECTION ".text.loop_sum" SUPERFREE
.ACCU 16
.INDEX 16
loop_sum:
	rep #$20
	tsa
	sec
	sbc.w #26
	tas
@start.27:
@body.28:
	lda.w #0
	sta 8,s
	lda.w #0
	sta 10,s
@for_cond.29:
	lda 8,s
	sta 20,s
	cmp 30,s
	bcc +
	jmp @for_join.32
+
@for_body.30:
@for_cont.31:
	lda 8,s
	inc a
	sta 16,s
	lda 10,s
	clc
	adc 20,s
	sta 18,s
	lda 16,s
	sta 8,s
	lda 18,s
	sta 10,s
	jmp @for_cond.29
@for_join.32:
	lda 10,s
	tax
	tsa
	clc
	adc.w #26
	tas
	txa
	rtl
.ENDS
/* end function loop_sum */


; Function: array_write (framesize=0, slots=13, alloc_slots=6, fn_leaf=1, leaf_opt=1)
; temp 64: slot=6, alloc=-1
; temp 65: slot=7, alloc=-1
; temp 66: slot=8, alloc=-1
; temp 67: slot=-1, alloc=0
; temp 68: slot=-1, alloc=4
; temp 69: slot=-1, alloc=5
; temp 70: slot=-1, alloc=-1
; temp 71: slot=-1, alloc=-1
; temp 72: slot=-1, alloc=-1
; temp 73: slot=9, alloc=-1
; temp 74: slot=10, alloc=-1
; temp 75: slot=11, alloc=-1
; temp 76: slot=12, alloc=-1
.SECTION ".text.array_write" SUPERFREE
.ACCU 16
.INDEX 16
array_write:
	rep #$20
@start.33:
@body.34:
	lda 6,s
	asl a
	clc
	adc 8,s
	tax
	lda 4,s
	sta.l $0000,x
	rtl
.ENDS
/* end function array_write */


; Function: array_read (framesize=0, slots=12, alloc_slots=5, fn_leaf=1, leaf_opt=1)
; temp 64: slot=5, alloc=-1
; temp 65: slot=6, alloc=-1
; temp 66: slot=-1, alloc=0
; temp 67: slot=-1, alloc=4
; temp 68: slot=-1, alloc=-1
; temp 69: slot=-1, alloc=-1
; temp 70: slot=7, alloc=-1
; temp 71: slot=8, alloc=-1
; temp 72: slot=9, alloc=-1
; temp 73: slot=10, alloc=-1
; temp 74: slot=11, alloc=-1
.SECTION ".text.array_read" SUPERFREE
.ACCU 16
.INDEX 16
array_read:
	rep #$20
@start.35:
@body.36:
	lda 4,s
	asl a
	clc
	adc 6,s
	tax
	lda.l $0000,x
	rtl
.ENDS
/* end function array_read */


; Function: struct_sum (framesize=26, slots=12, alloc_slots=4, fn_leaf=1, leaf_opt=1)
; temp 64: slot=4, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=5, alloc=-1
; temp 68: slot=-1, alloc=-1
; temp 69: slot=-1, alloc=-1
; temp 70: slot=6, alloc=-1
; temp 71: slot=7, alloc=-1
; temp 72: slot=-1, alloc=-1
; temp 73: slot=-1, alloc=-1
; temp 74: slot=-1, alloc=-1
; temp 75: slot=8, alloc=-1
; temp 76: slot=9, alloc=-1
; temp 77: slot=10, alloc=-1
; temp 78: slot=11, alloc=-1
.SECTION ".text.struct_sum" SUPERFREE
.ACCU 16
.INDEX 16
struct_sum:
	rep #$20
	tsa
	sec
	sbc.w #26
	tas
@start.37:
@body.38:
	lda 30,s
	tax
	lda.l $0000,x
	sta 16,s
	lda 30,s
	clc
	adc.w #2
	tax
	lda.l $0000,x
	clc
	adc 16,s
	tax
	tsa
	clc
	adc.w #26
	tas
	txa
	rtl
.ENDS
/* end function struct_sum */


; Function: swap (framesize=28, slots=13, alloc_slots=9, fn_leaf=1, leaf_opt=1)
; temp 64: slot=9, alloc=-1
; temp 65: slot=10, alloc=-1
; temp 66: slot=-1, alloc=0
; temp 67: slot=-1, alloc=4
; temp 68: slot=-1, alloc=8
; temp 69: slot=-1, alloc=-1
; temp 70: slot=11, alloc=-1
; temp 71: slot=-1, alloc=-1
; temp 72: slot=12, alloc=-1
; temp 73: slot=-1, alloc=-1
; temp 74: slot=-1, alloc=-1
; temp 75: slot=-1, alloc=-1
.SECTION ".text.swap" SUPERFREE
.ACCU 16
.INDEX 16
swap:
	rep #$20
	tsa
	sec
	sbc.w #28
	tas
@start.39:
@body.40:
	lda 34,s
	tax
	lda.l $0000,x
	sta 24,s
	lda 32,s
	tax
	lda.l $0000,x
	pha
	lda 36,s
	tax
	pla
	sta.l $0000,x
	lda 24,s
	pha
	lda 34,s
	tax
	pla
	sta.l $0000,x
	tsa
	clc
	adc.w #28
	tas
	rtl
.ENDS
/* end function swap */


; Function: call_add (framesize=0, slots=5, alloc_slots=2, fn_leaf=0, leaf_opt=1)
; temp 64: slot=2, alloc=-1
; temp 65: slot=3, alloc=-1
; temp 66: slot=-1, alloc=0
; temp 67: slot=-1, alloc=1
; temp 68: slot=-1, alloc=-1
; temp 69: slot=-1, alloc=-1
; temp 70: slot=4, alloc=-1
.SECTION ".text.call_add" SUPERFREE
.ACCU 16
.INDEX 16
call_add:
@start.41:
@body.42:
	jml add_u16
.ENDS
/* end function call_add */


; Function: mul_variable (framesize=0, slots=7, alloc_slots=2, fn_leaf=1, leaf_opt=1)
; temp 64: slot=2, alloc=-1
; temp 65: slot=3, alloc=-1
; temp 66: slot=-1, alloc=0
; temp 67: slot=-1, alloc=1
; temp 68: slot=-1, alloc=-1
; temp 69: slot=4, alloc=-1
; temp 70: slot=-1, alloc=-1
; temp 71: slot=5, alloc=-1
; temp 72: slot=6, alloc=-1
.SECTION ".text.mul_variable" SUPERFREE
.ACCU 16
.INDEX 16
mul_variable:
	rep #$20
@start.43:
@body.44:
	lda 4,s
	pha
	lda 8,s
	pha
	jsl __mul16
	plx
	plx
	rtl
.ENDS
/* end function mul_variable */


; Function: clamp (framesize=0, slots=11, alloc_slots=3, fn_leaf=1, leaf_opt=1)
; temp 64: slot=3, alloc=-1
; temp 65: slot=4, alloc=-1
; temp 66: slot=5, alloc=-1
; temp 67: slot=-1, alloc=0
; temp 68: slot=-1, alloc=1
; temp 69: slot=-1, alloc=2
; temp 70: slot=-1, alloc=-1
; temp 71: slot=6, alloc=-1
; temp 72: slot=-1, alloc=-1
; temp 73: slot=7, alloc=-1
; temp 74: slot=8, alloc=-1
; temp 75: slot=-1, alloc=-1
; temp 76: slot=-1, alloc=-1
; temp 77: slot=-1, alloc=-1
; temp 78: slot=-1, alloc=-1
; temp 79: slot=9, alloc=-1
; temp 80: slot=10, alloc=-1
; temp 81: slot=-1, alloc=-1
; temp 82: slot=-1, alloc=-1
.SECTION ".text.clamp" SUPERFREE
.ACCU 16
.INDEX 16
clamp:
	rep #$20
@start.45:
@body.46:
	lda 8,s
	cmp 6,s
	bcs +
	jmp @if_true.47
+
@if_false.48:
	lda 4,s
	cmp 8,s
	bcs +
	jmp @if_true.49
+
@if_false.50:
	lda 8,s
	rtl
@if_true.49:
	lda 4,s
	rtl
@if_true.47:
	lda 6,s
	rtl
.ENDS
/* end function clamp */


; Function: signed_shift_right_8 (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.signed_shift_right_8" SUPERFREE
.ACCU 16
.INDEX 16
signed_shift_right_8:
	rep #$20
@start.51:
@body.52:
	lda 4,s
	xba
	and.w #$00FF
	cmp.w #$0080
	bcc +
	ora.w #$FF00
+
	rtl
.ENDS
/* end function signed_shift_right_8 */


; Function: signed_shift_right_1 (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.signed_shift_right_1" SUPERFREE
.ACCU 16
.INDEX 16
signed_shift_right_1:
	rep #$20
@start.53:
@body.54:
	lda 4,s
	cmp.w #$8000
	ror a
	rtl
.ENDS
/* end function signed_shift_right_1 */


; Function: byte_store_loop (framesize=38, slots=18, alloc_slots=7, fn_leaf=1, leaf_opt=1)
; temp 64: slot=8, alloc=-1
; temp 65: slot=11, alloc=-1
; temp 66: slot=12, alloc=-1
; temp 67: slot=-1, alloc=0
; temp 68: slot=-1, alloc=4
; temp 69: slot=-1, alloc=5
; temp 70: slot=-1, alloc=6
; temp 71: slot=-1, alloc=-1
; temp 72: slot=14, alloc=-1
; temp 73: slot=-1, alloc=-1
; temp 74: slot=-1, alloc=-1
; temp 75: slot=15, alloc=-1
; temp 76: slot=-1, alloc=-1
; temp 77: slot=-1, alloc=-1
; temp 78: slot=-1, alloc=-1
; temp 79: slot=16, alloc=-1
; temp 80: slot=-1, alloc=-1
; temp 81: slot=-1, alloc=-1
; temp 82: slot=17, alloc=-1
; temp 83: slot=-1, alloc=-1
; temp 84: slot=13, alloc=-1
; temp 85: slot=-1, alloc=-1
; temp 86: slot=7, alloc=-1
; temp 87: slot=-1, alloc=-1
; temp 88: slot=9, alloc=-1
; temp 89: slot=10, alloc=-1
.SECTION ".text.byte_store_loop" SUPERFREE
.ACCU 16
.INDEX 16
byte_store_loop:
	rep #$20
	tsa
	sec
	sbc.w #38
	tas
@start.55:
@body.56:
	lda 44,s
	and.w #$00FF
	sta 24,s
	lda.w #0
	sta 16,s
@for_cond.57:
	lda 16,s
	cmp 42,s
	bcc +
	jmp @for_join.60
+
@for_body.58:
	lda 16,s
	clc
	adc 46,s
	sta 36,s
	lda 24,s
	sep #$20
	rep #$20
	pha
	lda 38,s
	tax
	pla
	sep #$20
	sta.l $0000,x
	rep #$20
@for_cont.59:
	lda 16,s
	inc a
	sta 28,s
	sta 16,s
	jmp @for_cond.57
@for_join.60:
	tsa
	clc
	adc.w #38
	tas
	rtl
.ENDS
/* end function byte_store_loop */


; Function: global_increment (framesize=0, slots=2, alloc_slots=0, fn_leaf=1, leaf_opt=1)
; temp 64: slot=0, alloc=-1
; temp 65: slot=1, alloc=-1
.SECTION ".text.global_increment" SUPERFREE
.ACCU 16
.INDEX 16
global_increment:
	rep #$20
@start.61:
@body.62:
	lda.w g_counter
	inc a
	sta.w g_counter
	rtl
.ENDS
/* end function global_increment */


; Function: zero_store_global (framesize=0, slots=0, alloc_slots=0, fn_leaf=1, leaf_opt=1)
.SECTION ".text.zero_store_global" SUPERFREE
.ACCU 16
.INDEX 16
zero_store_global:
	rep #$20
@start.63:
@body.64:
	stz.w g_value
	rtl
.ENDS
/* end function zero_store_global */


; Function: compare_and_branch (framesize=0, slots=8, alloc_slots=2, fn_leaf=1, leaf_opt=1)
; temp 64: slot=2, alloc=-1
; temp 65: slot=3, alloc=-1
; temp 66: slot=-1, alloc=0
; temp 67: slot=-1, alloc=1
; temp 68: slot=-1, alloc=-1
; temp 69: slot=4, alloc=-1
; temp 70: slot=-1, alloc=-1
; temp 71: slot=5, alloc=-1
; temp 72: slot=6, alloc=-1
; temp 73: slot=-1, alloc=-1
; temp 74: slot=-1, alloc=-1
; temp 75: slot=-1, alloc=-1
; temp 76: slot=-1, alloc=-1
; temp 77: slot=7, alloc=-1
.SECTION ".text.compare_and_branch" SUPERFREE
.ACCU 16
.INDEX 16
compare_and_branch:
	rep #$20
@start.65:
@body.66:
	lda 6,s
	cmp 4,s
	bne +
	jmp @if_true.67
+
@if_false.68:
	lda 6,s
	cmp 4,s
	bcs +
	jmp @if_true.69
+
@if_false.70:
	lda.w #3
	rtl
@if_true.69:
	lda.w #2
	rtl
@if_true.67:
	lda.w #1
	rtl
.ENDS
/* end function compare_and_branch */


; Function: helper (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.helper" SUPERFREE
.ACCU 16
.INDEX 16
helper:
	rep #$20
@start.71:
@body.72:
	lda 4,s
	inc a
	rtl
.ENDS
/* end function helper */


; Function: call_chain (framesize=0, slots=4, alloc_slots=1, fn_leaf=0, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.call_chain" SUPERFREE
.ACCU 16
.INDEX 16
call_chain:
	rep #$20
@start.73:
@body.74:
	lda 4,s
	pha
	jsl helper
	plx
	sta 4,s
	jml helper
.ENDS
/* end function call_chain */


; Function: pea_constant_args (framesize=0, slots=1, alloc_slots=0, fn_leaf=0, leaf_opt=1)
; temp 64: slot=0, alloc=-1
.SECTION ".text.pea_constant_args" SUPERFREE
.ACCU 16
.INDEX 16
pea_constant_args:
	rep #$20
@start.75:
@body.76:
	pea.w 42
	pea.w 100
	jsl add_u16
	plx
	plx
	rtl
.ENDS
/* end function pea_constant_args */


; Function: mul_const_24 (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.mul_const_24" SUPERFREE
.ACCU 16
.INDEX 16
mul_const_24:
	rep #$20
@start.77:
@body.78:
	lda 4,s
	sta.b tcc__r9
	asl a
	clc
	adc.b tcc__r9
	asl a
	asl a
	asl a
	rtl
.ENDS
/* end function mul_const_24 */


; Function: mul_const_48 (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.mul_const_48" SUPERFREE
.ACCU 16
.INDEX 16
mul_const_48:
	rep #$20
@start.79:
@body.80:
	lda 4,s
	sta.b tcc__r9
	asl a
	clc
	adc.b tcc__r9
	asl a
	asl a
	asl a
	asl a
	rtl
.ENDS
/* end function mul_const_48 */


; Function: mul_const_20 (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.mul_const_20" SUPERFREE
.ACCU 16
.INDEX 16
mul_const_20:
	rep #$20
@start.81:
@body.82:
	lda 4,s
	sta.b tcc__r9
	asl a
	asl a
	clc
	adc.b tcc__r9
	asl a
	asl a
	rtl
.ENDS
/* end function mul_const_20 */


; Function: mul_const_40 (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.mul_const_40" SUPERFREE
.ACCU 16
.INDEX 16
mul_const_40:
	rep #$20
@start.83:
@body.84:
	lda 4,s
	sta.b tcc__r9
	asl a
	asl a
	clc
	adc.b tcc__r9
	asl a
	asl a
	asl a
	rtl
.ENDS
/* end function mul_const_40 */


; Function: mul_const_96 (framesize=0, slots=4, alloc_slots=1, fn_leaf=1, leaf_opt=1)
; temp 64: slot=1, alloc=-1
; temp 65: slot=-1, alloc=0
; temp 66: slot=-1, alloc=-1
; temp 67: slot=2, alloc=-1
; temp 68: slot=3, alloc=-1
.SECTION ".text.mul_const_96" SUPERFREE
.ACCU 16
.INDEX 16
mul_const_96:
	rep #$20
@start.85:
@body.86:
	lda 4,s
	sta.b tcc__r9
	asl a
	clc
	adc.b tcc__r9
	asl a
	asl a
	asl a
	asl a
	asl a
	rtl
.ENDS
/* end function mul_const_96 */

.RAMSECTION ".bss.1" BANK 0 SLOT 1
g_counter:
	dsb 2
.ENDS
/* end data */

.RAMSECTION ".bss.2" BANK 0 SLOT 1
g_value:
	dsb 2
.ENDS
/* end data */


; End of generated code
