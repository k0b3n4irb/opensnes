; G1 R3-0, second paper of k_path_c (rules: G1_r3_paper_rules.md + G1_r3_paper_rules_v2.md, IR: path_ir.ssa)
; Written by hand, not measured, not assembled. Label renamed k_pathp2_asm (IR function k_path_c).
;
; ASSIGNMENTS TRIED: 1024 raw (5 loop values idx, counter, s, m, step x {A,X,Y,slot}); the 8 accumulator words are
;   slots by rule 5 and by count (8 words > 3 registers). Pruned by legality: s, m, step2 are operands of adc/sbc
;   (memory operand) and are used by four ALU chains that each need A -> slot only (A is clobbered, X/Y are not
;   adc operands). idx must be X for long,x (no long,y): X or slot (then txa/tax round trips). counter: Y, slot or X
;   (if idx is in a slot). 5 legal combinations costed by hand: (idx X, cnt Y) (idx X, cnt slot) (idx slot, cnt Y)
;   (idx slot, cnt slot) (idx slot, cnt X). Kept: idx in X, counter in Y, everything else in slots (cheapest:
;   update = txa/clc/adc/and/tax, test = dey/bne, no slot round trip for either).
;
; IDIOMS USED (each exact on the whole domain of the IR):
;  I1  sign-extension chain: m = 0 or $FFFF built once per sample from s (cmp #$8000 / lda #0 / sbc #0 / eor #$FFFF)
;      and used as the high word of sext(s) in acc0 (+), acc1 (-), and as the high word of sar(s,1) in acc3.
;      Same build used at entry for hi|sext(lo) of the four initial pairs (the select of the first paper, "cond ? k : 0").
;  I2  scaled induction variable: idx is only used as 2*idx (table index) -> carried scaled in X; step carried
;      scaled (ST = 2*step); mask 63 scaled to $7E. (ST + X) & $7E has the low 7 bits of 2*(step+idx) in 16 bits: exact.
;      Final pa_idx = X >> 1 (X is even and <= 126 after >= 1 iteration, trip count 64: exact).
;      The entry value 2*extuh(idx) is 17 bits wide (idx arbitrary): the IR address is path_sin + 2*idx16 with a
;      carry into the bank. X holds the low 16 bits and the 17th bit is the carry out of the entry asl. The IR gives
;      no range for pa_idx at entry, so it is NOT assumed zero: the first load has a second form,
;      long path_sin+$10000,x, taken when that carry is set (a second entry of the loop right after the load;
;      zero cost per iteration; from iteration 2 on idx < 64 comes from the IR's own "and 63"). This is the only
;      construct that is not one of I1-I4: it is the exact handling of a condition the IR does not give.
;  I3  loop rotation: trip count 64 constant >= 1, test at the bottom (bne).
;  I4  induction reversal: i counted in Y from 64 down to 0, dey/bne (i is used for nothing else).
;  Already admitted: cond ? k : 0 (m above).
;
; REFUSALS: F (2*s high word = mask): refused, hi(2s) = rol of m with the carry of asl s, as the IR's 32-bit shift.
;           P (hi | sext(lo) as a plain copy): refused, the select stays (mask built from lo, ora with hi).
;           No D move, no DBR change (path_sin is ROM; pa_* are plain low RAM read/written with abs under the
;           caller's DBR, as the first paper does). No helper call, no stack argument (no arguments).
;
; Slots (tcc__lf, leaf): +0 lo0 +2 hi0 +4 lo1 +6 hi1 +8 lo2 +10 hi2 +12 lo3 +14 hi3 +16 ST(2*step) +18 S +20 M +22 T +24 U

; sframe k_pathp2_asm
k_pathp2_asm:
	rep #$20
	.ACCU 16
	.INDEX 16
	; ---- entry: four pairs v = (hi<<16) | sext(lo), lo word = lo, hi word = hi | mask(lo)
	lda.w pa_lo
	sta.b tcc__lf+0
	cmp #$8000
	lda #$0000
	sbc #$0000
	eor #$FFFF
	ora.w pa_hi
	sta.b tcc__lf+2
	lda.w pa_lo+2
	sta.b tcc__lf+4
	cmp #$8000
	lda #$0000
	sbc #$0000
	eor #$FFFF
	ora.w pa_hi+2
	sta.b tcc__lf+6
	lda.w pa_lo+4
	sta.b tcc__lf+8
	cmp #$8000
	lda #$0000
	sbc #$0000
	eor #$FFFF
	ora.w pa_hi+4
	sta.b tcc__lf+10
	lda.w pa_lo+6
	sta.b tcc__lf+12
	cmp #$8000
	lda #$0000
	sbc #$0000
	eor #$FFFF
	ora.w pa_hi+6
	sta.b tcc__lf+14
	; ---- step scaled, idx scaled (carry = bit 16 of 2*idx16), counter
	lda.w pa_step
	asl a
	sta.b tcc__lf+16
	lda.w pa_idx
	asl a                           ; C = 17th bit of 2*extuh(idx)
	tax                             ; tax, ldy do not touch C
	ldy #64
	bcc k_pathp2_loop
	lda.l path_sin+$10000,x         ; first load only, idx16 >= $8000
	bra k_pathp2_body
k_pathp2_loop:
	lda.l path_sin,x                ; s = sext(path_sin[idx]), volatile, once per iteration
k_pathp2_body:
	sta.b tcc__lf+18                ; S
	cmp #$8000                      ; C = (s < 0)
	lda #$0000
	sbc #$0000                      ; A = C ? 0 : $FFFF
	eor #$FFFF                      ; A = m
	sta.b tcc__lf+20                ; M
	; acc0 += sext(s)
	lda.b tcc__lf+0
	clc
	adc.b tcc__lf+18
	sta.b tcc__lf+0
	lda.b tcc__lf+2
	adc.b tcc__lf+20
	sta.b tcc__lf+2
	; acc1 -= sext(s)
	lda.b tcc__lf+4
	sec
	sbc.b tcc__lf+18
	sta.b tcc__lf+4
	lda.b tcc__lf+6
	sbc.b tcc__lf+20
	sta.b tcc__lf+6
	; acc3 += sar(sext(s),1): low word = s ror with sign, high word = m
	lda.b tcc__lf+18
	cmp #$8000
	ror a
	clc
	adc.b tcc__lf+12
	sta.b tcc__lf+12
	lda.b tcc__lf+14
	adc.b tcc__lf+20
	sta.b tcc__lf+14
	; acc2 += 2*sext(s): low = s<<1, high = (m<<1)|bit15(s)
	lda.b tcc__lf+18
	asl a
	sta.b tcc__lf+22                ; T
	lda.b tcc__lf+20
	rol a
	sta.b tcc__lf+24                ; U
	lda.b tcc__lf+8
	clc
	adc.b tcc__lf+22
	sta.b tcc__lf+8
	lda.b tcc__lf+10
	adc.b tcc__lf+24
	sta.b tcc__lf+10
	; idx2 = (2*step + idx2) & $7E
	txa
	clc
	adc.b tcc__lf+16
	and #$007E
	tax
	dey
	bne k_pathp2_loop
	; ---- exit: store the four pairs and idx
	lda.b tcc__lf+0
	sta.w pa_lo
	lda.b tcc__lf+2
	sta.w pa_hi
	lda.b tcc__lf+4
	sta.w pa_lo+2
	lda.b tcc__lf+6
	sta.w pa_hi+2
	lda.b tcc__lf+8
	sta.w pa_lo+4
	lda.b tcc__lf+10
	sta.w pa_hi+4
	lda.b tcc__lf+12
	sta.w pa_lo+6
	lda.b tcc__lf+14
	sta.w pa_hi+6
	txa
	lsr a
	sta.w pa_idx
	rtl
