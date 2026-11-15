; =====================================================================================================
; k_path_c, paper text under the frozen rules of G1_r3_paper_rules.md (inputs: those rules and path_ir.ssa only)
;
; ASSIGNMENTS TRIED, loop @for_cond/@for_body/@for_cont (the only loop): 28
;   - 20 = place of the carried scalars: Y in {none, i, idx, step, M} x s kept in A for its first consumer {yes, no}
;          x M {computed once into a slot, recomputed at each of its 5 uses}.
;          8 of the 20 illegal (Y=step, Y=M: adc/sbc take their operand in memory or immediate, never a register).
;          Of the 12 legal, per-iteration cost of the counter+index part, CPU cycles by hand:
;          Y=i 30, Y=idx 33, none 39 -> Y=i kept (counter: iny / cpy #64).
;   - 4  = which chain starts on A right after M is stored (acc0, acc1, T=s*2, U=s sar 1): the U chain, because A=M
;          already holds the operand of its high word (saves one lda); acc0 first (A=s commuting) was dropped,
;          M's cmp/sbc destroys the carry of a low-word add started before it.
;   - 4  = each 32-bit accumulator held as a register pair (X:Y) instead of two slots: refused, rule 5 (a pair is a
;          pair of slots) and X is the index of the path_sin read at every iteration.
;   Kept: i in Y; idx, step, s, M, the four accumulators (pairs), T/U scratch pair in tcc__lf slots; A = ALU; X = the
;   index 2*idx rewritten each iteration (cost, not refusal, rule 2).
;
; IR IDIOMS applied once before allocation:
;   (a) or (hi<<16), loadsw(lo)  ->  low word = lo ; high word = hi | (lo<0 ? $FFFF : 0)   (cond ? k : 0, a bmi select);
;       the IR sign-extends lo (loadsw), so the or is NOT a plain concatenation: kept as the IR says.
;   (b) loadsw of path_sin: s is a 32-bit value = pair (S, M), M = (S<0 ? $FFFF : 0)   (sign extension, cmp/sbc/eor).
;   (c) mul s,2 -> asl/rol over the pair ; sar s,1 -> cmp #$8000 / ror, ror over the pair ; shr x,16 -> the high word.
;   (d) range test cultw i,64 -> cpy #64 / bcs (carry clear = in range). mul idx*2 -> asl a. extuh of a word: none.
;
; USE OF D MOVED: none. Reason: every access of the function besides the 9 stores of the epilogue and the 4 loads of
;   the prologue is a .b tcc__lf access, forbidden while D is moved (rule 4); and the epilogue stores read tcc__lf
;   slots too, so no stretch of the text satisfies the condition.
; USE OF DBR SET: none. Reason: path_sin is a ROM table, not an entity of bank 0 below $2000 (rule 4, first condition),
;   so its read stays `lda.l path_sin,x` (long,x, one byte more than abs,x). The plain-RAM globals are read with
;   DBR at its default value (never moved here).
;
; REFUSALS / LIMITS (written, not worked around):
;   - Exactness of the index. The IR computes extuh(idx)*2 + &path_sin on 32 bits; sym,x takes an unsigned 16-bit
;     index (rule 2), so the paper computes 2*idx mod $10000. It equals the IR for every pa_idx < $8000 at entry
;     (after the first iteration idx is <= 63 anyway). No guard added: a guard would be a trick no rule permits.
;   - Rule 1/store: after `sta` the value stays live in A for the next instruction (A stays live across stores); the
;     slot is its home, A is the stretch the local schedule lengthens.
;   - No helper call, no jump table, no address taken: nothing else refused.
;
; SLOTS (tcc__lf, direct page $80-$A7, 28 bytes used of 40): below.
; =====================================================================================================

.DEFINE ACC0L   tcc__lf+0       ; accumulators: %.1 (0), %.12 (1), %.23 (2), %.34 (3), low/high word
.DEFINE ACC0H   tcc__lf+2
.DEFINE ACC1L   tcc__lf+4
.DEFINE ACC1H   tcc__lf+6
.DEFINE ACC2L   tcc__lf+8
.DEFINE ACC2H   tcc__lf+10
.DEFINE ACC3L   tcc__lf+12
.DEFINE ACC3H   tcc__lf+14
.DEFINE STEP    tcc__lf+16      ; %.48
.DEFINE IDX     tcc__lf+18      ; %.45 (idx, carried)
.DEFINE S       tcc__lf+20      ; %.57 low word
.DEFINE M       tcc__lf+22      ; %.57 high word (sign mask)
.DEFINE TL      tcc__lf+24      ; scratch pair: %.70 (s*2) then %.74 (s sar 1), low
.DEFINE TH      tcc__lf+26      ; high

; sframe k_path_c
k_path_c:
    rep #$20
    .ACCU 16
    .INDEX 16

; ---- prologue: %.11 %.22 %.33 %.44 (hi | sext(lo)), %.46 %.48 ------------------------------------------------
    lda.w pa_lo
    sta.b ACC0L
    bmi @n0
    lda.w pa_hi
    bra @s0
@n0:
    lda #$FFFF
@s0:
    sta.b ACC0H

    lda.w pa_lo+2
    sta.b ACC1L
    bmi @n1
    lda.w pa_hi+2
    bra @s1
@n1:
    lda #$FFFF
@s1:
    sta.b ACC1H

    lda.w pa_lo+4
    sta.b ACC2L
    bmi @n2
    lda.w pa_hi+4
    bra @s2
@n2:
    lda #$FFFF
@s2:
    sta.b ACC2H

    lda.w pa_lo+6
    sta.b ACC3L
    bmi @n3
    lda.w pa_hi+6
    bra @s3
@n3:
    lda #$FFFF
@s3:
    sta.b ACC3H

    lda.w pa_step
    sta.b STEP
    lda.w pa_idx
    sta.b IDX
    ldy #$0000                  ; i = 0, in Y

; ---- loop ------------------------------------------------------------------------------------------------------
@for_cond:
    cpy #$0040                  ; i <u 64
    bcs @for_join
@for_body:
    lda.b IDX                   ; x = 2*idx
    asl a
    tax
    lda.l path_sin,x            ; s (volatile read, once)
    sta.b S
    cmp #$8000                  ; C = s<0
    lda #$0000
    sbc #$0000                  ; C=1 -> 0, C=0 -> $FFFF
    eor #$FFFF                  ; M = s<0 ? $FFFF : 0
    sta.b M
    ; U = s sar 1 (32-bit pair TH:TL), A = M on entry
    cmp #$8000
    ror a
    sta.b TH
    lda.b S
    ror a
    sta.b TL
    clc                         ; acc3 += U
    lda.b ACC3L
    adc.b TL
    sta.b ACC3L
    lda.b ACC3H
    adc.b TH
    sta.b ACC3H
    ; T = s * 2 (32-bit pair TH:TL)
    lda.b S
    asl a
    sta.b TL
    lda.b M
    rol a
    sta.b TH
    clc                         ; acc2 += T
    lda.b ACC2L
    adc.b TL
    sta.b ACC2L
    lda.b ACC2H
    adc.b TH
    sta.b ACC2H
    clc                         ; acc0 += s
    lda.b ACC0L
    adc.b S
    sta.b ACC0L
    lda.b ACC0H
    adc.b M
    sta.b ACC0H
    sec                         ; acc1 -= s
    lda.b ACC1L
    sbc.b S
    sta.b ACC1L
    lda.b ACC1H
    sbc.b M
    sta.b ACC1H
    lda.b STEP                  ; idx = (step + idx) & 63
    clc
    adc.b IDX
    and #$003F
    sta.b IDX
@for_cont:
    iny                         ; i + 1
    bra @for_cond

; ---- epilogue --------------------------------------------------------------------------------------------------
@for_join:
    lda.b ACC0H
    sta.w pa_hi
    lda.b ACC0L
    sta.w pa_lo
    lda.b ACC1H
    sta.w pa_hi+2
    lda.b ACC1L
    sta.w pa_lo+2
    lda.b ACC2H
    sta.w pa_hi+4
    lda.b ACC2L
    sta.w pa_lo+4
    lda.b ACC3H
    sta.w pa_hi+6
    lda.b ACC3L
    sta.w pa_lo+6
    lda.b IDX
    sta.w pa_idx
    rtl
