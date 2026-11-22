; G1 R3-0 paper text of k_soa_c, written under rules.md only (2026-11-15).
; sframe fn
; (leaf, no arguments, no locals: no tcc__lf slot, no tcc__r* word, no helper call used)
;
; Assumptions read from the rules, not from any generated text:
;  - ac_*/sp_* arrays and the two scalars are plain C RAM, reached by 16-bit absolute (sym,x / sym,y).
;  - X and Y are 16 bits on entry (the public ABI keeps them so; only `rep #$20` is written, as asked).
;  - D is never moved, DBR is never set (see the refusals at the end).
;
; Loop values and their single location at a time:
;   i            (%.1.2, also %.6)  -> X   index of the u8 arrays (ac_dirty, ac_flags), counter, compared to 18
;   2*i          (%.7)              -> Y   index of the s16/u16 arrays (abs,y), stepped by iny iny
;   2*f (frame_off index, %.88)     -> X   TEMPORARILY (the only indexed access whose index is another value:
;                                          writes X, a cost not a refusal), i is rebuilt from Y afterwards
;   every s16/u16 value (dx, dy, x, y, xc, yc, f, sp_x, ...) -> A, one at a time, never held in a slot
;   (the local schedule moves the whole y chain after the whole x chain so no clamp result is held)

k_soa_c:
	rep #$20
	.ACCU 16
	.INDEX 16
	ldx #0                      ; X = i = 0 (the entry test 0 < 18 is constant true: loop rotated, test at the bottom)
	ldy #0                      ; Y = 2*i
_top:
	lda ac_dx,y                 ; A = dx  (%.9)
	ora ac_dy,y                 ; A = dx | dy (%.15), dy loaded as the load-op operand, never held
	bne _move
	; --- dx|dy == 0 : look at ac_dirty[i]
	lda ac_dirty,x              ; byte read as word (X = i), masked
	and #$00FF
	bne _join                   ; dirty != 0 -> join
	brl _cont                   ; dirty == 0 -> continue
_move:
	; --- x chain: x = ac_x[i]; clamp to [16,623] (signed); ac_x[i] = ac_dx[i] + clamped
	lda ac_x,y                  ; A = x (%.21)
	eor #$8000                  ; A = x biased: signed order becomes unsigned order, so cmp serves (A keeps the value)
	cmp #$8270                  ; x > 623  <=>  (x^$8000) >= (624^$8000)
	bcs _xhi
	cmp #$8010                  ; x < 16   <=>  (x^$8000) <  (16^$8000)
	bcc _xlo
	eor #$8000                  ; in range: unbias, A = x
	bra _xdone
_xhi:
	lda #623                    ; A = 623
	bra _xdone
_xlo:
	lda #16                     ; A = 16
_xdone:                         ; A = xc (%.16.9)
	clc
	adc ac_dx,y                 ; A = xc + dx (%.42), dx reloaded as the IR says (%.41), operand not held
	sta ac_x,y
	; --- y chain: clamp to [24,455]
	lda ac_y,y                  ; A = y (%.27)
	eor #$8000
	cmp #$81C8                  ; y > 455  <=>  (y^$8000) >= (456^$8000)
	bcs _yhi
	cmp #$8018                  ; y < 24   <=>  (y^$8000) <  (24^$8000)
	bcc _ylo
	eor #$8000
	bra _ydone
_yhi:
	lda #455
	bra _ydone
_ylo:
	lda #24
_ydone:                         ; A = yc (%.22.10)
	clc
	adc ac_dy,y                 ; A = yc + dy (%.53)
	sta ac_y,y
_join:
	; --- ac_dirty[i] = 0 : byte store keeps sep/rep, stz needs no register (constant 0)
	sep #$20
	.ACCU 8
	stz ac_dirty,x
	rep #$20
	.ACCU 16
	lda ac_frame,y              ; A = f (%.73)
	cmp #24                     ; unsigned f < 24 ; cmp keeps A = f
	bcs _nof
	sta sp_frame,y              ; sp_frame[i] = f
_nof:
	asl a                       ; A = 2*f (index of frame_off; f >= 32768 wraps, out of bounds anyway: UB for the C array)
	tax                         ; X = 2*f  (X = i is dead until rebuilt below; i is still held as Y/2)
	lda ac_x,y                  ; A = ac_x[i] (%.85); load order exchanged with the frame_off load, independent
	clc
	adc.l frame_off,x           ; A += frame_off[f]  (%.90, volatile, one read; long,x: ROM asset bank, DBR untouched)
	sec
	sbc #16                     ; A = %.92
	sta sp_x,y
	lda ac_y,y                  ; A = ac_y[i] (%.101)
	sec
	sbc #16                     ; A = %.102
	sta sp_y,y
	sep #$20
	.ACCU 8
	lda #1
	sta sp_changed              ; sp_changed = 1 (byte store)
	rep #$20
	.ACCU 16
	tya
	lsr a
	tax                         ; X = i rebuilt from Y (cheapest of the assignments tried, see end)
	lda ac_flags,x              ; byte read as word
	and #$0010                  ; one AND masks the byte and gives t = flags & 16 (the cond ? 16 : 0 idiom, one step)
	eor #$FFFF                  ; A = ~t
	sec
	adc ac_y,y                  ; A = ~t + 1 + y = y - t = d (%.121); y reloaded (%.112.5) as the load-op operand
	cmp sp_depth,y              ; d != sp_depth[i] ? (cnew); A keeps d
	beq _cont
	sta sp_depth,y              ; sp_depth[i] = d
	lda sp_depth_changed        ; A = counter (%.134)
	inc a                       ; A = counter + 1 (%.135)
	sta sp_depth_changed
_cont:
	inx                         ; i + 1 (X = i here on every path)
	iny
	iny                         ; 2*(i+1)
	cpx #18                     ; unsigned i < 18
	bcs _done
	brl _top                    ; the body is longer than a short branch reaches
_done:
	rtl

; ---------------------------------------------------------------------------------------------------------
; D / DBR : none used. No D moved, no DBR set, no tcc__lf access.
;
; Assignments tried for the loop (values i, 2*i, 2*f index; locations X, Y, slot, derived): 8
;   1. i=X, 2*i=Y, X clobbered by the frame_off index, rebuilt tya/lsr/tax once (chosen)
;   2. i=Y, 2*i=X (swap): the same single rebuild (tya/asl/tax), and the dirty store loses stz abs,x
;      (no stz abs,y: lda #0 + sta, 16 master cycles more) -> worse than 1
;   3. as 1, X saved by phx/plx around the lookup instead of the rebuild
;   4. as 1, X saved by stx/ldx on a direct-page slot
;   5. 2*i=Y only, X derived (tya/lsr/tax) before each byte access (3 per iteration on the dirty path)
;   6. i in a slot, 2*i=Y, X scratch (ldx slot before each byte access, counter stepped through A)
;   7. i in a slot, 2*i=X, Y scratch (same, plus the lookup rebuilds X)
;   8. the x chain and the y chain kept in the IR order (xc held across the y clamp: sta/lda on a slot)
; Rebuild costs, master cycles, one iteration on the join path (every non-skipped iteration), no block weights
; (none were given with the IR, so no weighted figure is claimed; this comparison does not need one):
;                       SlowROM   FastROM
;   tya;lsr a;tax          42        36      (3 implied instructions)
;   phx + plx              66        62
;   stx dp + ldx dp        64        60
; Choice 1 is the cheapest legal on the path every iteration but the skip takes; 5 pays that rebuild up to 3 times,
; 6 and 7 pay a slot access per byte access plus a counter step through A, 8 pays a slot store and load (64 / 60).
;
; Refusals, with their reason:
;   - frame_off via long,y: refused by rule 2 (no long,y); the index is moved to X instead.
;   - DBR set to the asset bank to use abs,x instead of long,x: the saving is one byte (8 master); setting and
;     restoring DBR costs several instructions (phb / pea / plb ... plb), and rule 4 forbids any plain-RAM absolute
;     access while it is moved: refused on cost, the condition could be met only around the one lookup.
;   - D moved onto the arrays (direct-page indexed): rule 4 needs the entity in bank 0 below $2000 and gives
;     no address; nine s16 arrays of 36 bytes span more than the 256-byte direct-page window, and the IR states no
;     layout: the condition cannot be shown, refused.
;   - inc abs (read-modify-write in memory) for sp_depth_changed: no rule gives a location to a value inside a
;     memory read-modify-write; lda / inc a / sta written instead.
;   - no refusal of the function itself: no jump table, no inline assembly, no address taken, public ABI kept.
; ---------------------------------------------------------------------------------------------------------
