; gapbench — the assembly side: the driver, and each routine of kernels.c
; written by hand on the same data, the way a 65816 programmer writes it.
; Entry and exit as the C ABI has them: A, X, Y 16 bits, D = 0, DBR = 0.
; tcc__lf (32 bytes of direct page) is scratch for the length of a call.
;
; What each twin allows itself that C does not say is listed in README.md
; (block move, unrolling, the hardware multiplier, arguments in registers,
; a value range it knows). Bench only: not the API of a module.

.DEFINE CALLS 16

.DEFINE T0 tcc__lf+0
.DEFINE T1 tcc__lf+2
.DEFINE T2 tcc__lf+4
.DEFINE T3 tcc__lf+6
.DEFINE T4 tcc__lf+8

;------------------------------------------------------------------------------
; The driver: for each row of the table, setup, CALLS calls of the routine
; between bench_mark_a and bench_mark_b, then the checksum. Interrupts off:
; a run is the same cycles every time. Written in assembly so that its cost
; does not move with the compiler; run.py subtracts the row of k_nop.
;------------------------------------------------------------------------------
.SECTION ".gap_driver" SUPERFREE
.ACCU 16
.INDEX 16
bench_table:
    .dl s_nop,   k_nop,       c_nop
    .dl s_copy,  k_copy_c,    c_copy
    .dl s_copy,  k_copy_asm,  c_copy
    .dl s_copy,  k_copy_lib,  c_copy
    .dl s_fill,  k_fill_c,    c_fill
    .dl s_fill,  k_fill_asm,  c_fill
    .dl s_oam,   k_oam_c,     c_oam
    .dl s_oam,   k_oam_asm,   c_oam
    .dl s_map,   k_col_c,     c_col
    .dl s_map,   k_col_asm,   c_col
    .dl s_map,   k_row_c,     c_row
    .dl s_map,   k_row_asm,   c_row
    .dl s_box,   k_box_c,     c_box
    .dl s_box,   k_box_asm,   c_box
    .dl s_fixed, k_fixed_c,   c_fixed
    .dl s_fixed, k_fixed_asm, c_fixed
    .dl s_mul,   k_mul_c,     c_mul
    .dl s_mul,   k_mul_asm,   c_mul
    .dl s_ents,  k_ents_c,    c_ents
    .dl s_ents,  k_ents_asm,  c_ents
    .dl s_call,  k_call_c,    c_call
    .dl s_call,  k_call_asm,  c_call
    .dl s_soa,   k_soa_c,     c_soa
    .dl s_soa,   k_soa_asm,   c_soa
    .dl s_glide, k_glide_c,   c_glide
    .dl s_glide, k_glide_asm, c_glide
bench_table_end:

bench_run:
    sei
    sep #$20
    .ACCU 8
    stz.w $4200
    rep #$30
    .ACCU 16
    .INDEX 16
    stz.w bench_i
    stz.w bench_k
gap_entry:
    ldx.w bench_i
    lda.l bench_table,x
    sta.w bench_ptr
    lda.l bench_table+2,x
    sta.w bench_ptr+2
    jsr gap_call
bench_mark_a:
    lda.w #CALLS
    sta.w bench_n
    ldx.w bench_i
    lda.l bench_table+3,x
    sta.w bench_ptr
    lda.l bench_table+5,x
    sta.w bench_ptr+2
gap_rep:
    jsr gap_call
    dec.w bench_n
    bne gap_rep
bench_mark_b:
    ldx.w bench_i
    lda.l bench_table+6,x
    sta.w bench_ptr
    lda.l bench_table+8,x
    sta.w bench_ptr+2
    jsr gap_call
    ldx.w bench_k
    sta.w bench_sum,x
    inx
    inx
    stx.w bench_k
    lda.w bench_i
    clc
    adc.w #9
    sta.w bench_i
    cmp.w #bench_table_end-bench_table
    bcc gap_entry
bench_done:
    jmp bench_done

gap_call:
    phk
    pea gap_ret-1
    jml [bench_ptr]
gap_ret:
    rep #$30
    rts
.ENDS

.SECTION ".gap_nop" SUPERFREE
k_nop:
    rtl
.ENDS

;------------------------------------------------------------------------------
; copy: the block move the language has no word for.
;------------------------------------------------------------------------------
.SECTION ".gap_copy" SUPERFREE
.ACCU 16
.INDEX 16
k_copy_asm:
    phb
    ldx.w #cp_src
    ldy.w #cp_dst
    lda.w #255
    mvn $00,$7E                 ; WLA-DX order: source bank, destination bank
    plb
    rtl
.ENDS

;------------------------------------------------------------------------------
; fill: eight stores a turn.
;------------------------------------------------------------------------------
.SECTION ".gap_fill" SUPERFREE
.ACCU 16
.INDEX 16
k_fill_asm:
    lda.w fl_val
    tay
    ldx.w #512-16
-   sta.l fl_buf+0,x
    sta.l fl_buf+2,x
    sta.l fl_buf+4,x
    sta.l fl_buf+6,x
    sta.l fl_buf+8,x
    sta.l fl_buf+10,x
    sta.l fl_buf+12,x
    sta.l fl_buf+14,x
    txa
    sec
    sbc.w #16
    tax
    tya
    bcs -
    clc
    adc.w #$0101
    sta.w fl_val
    rtl
.ENDS

;------------------------------------------------------------------------------
; oam: X walks the objects, Y the list; both range tests are one unsigned
; compare; an entry is two word stores.
;------------------------------------------------------------------------------
.SECTION ".gap_oam" SUPERFREE
.ACCU 16
.INDEX 16
gap_hibit:
    .db 1, 4, 16, 64, 0
k_oam_asm:
    stz.w oam_hi
    stz.w oam_hi+2
    stz.w oam_hi+4
    stz.w oam_hi+6
    ldx.w #0
    ldy.w #0
@loop:
    lda.w sprs+5,x              ; attr, on
    cmp.w #$0100
    bcc @next
    lda.w sprs+0,x
    sec
    sbc.w cam_x
    sta.b T0                    ; sx
    clc
    adc.w #15
    cmp.w #271                  ; -15..255
    bcs @next
    lda.w sprs+2,x
    sec
    sbc.w cam_y
    clc
    adc.w #15
    cmp.w #239                  ; -15..223
    bcs @next
    sec
    sbc.w #15
    xba                         ; sy in the high byte
    sep #$20
    .ACCU 8
    lda.b T0
    rep #$20
    .ACCU 16
    sta.w oam_lo,y
    lda.w sprs+4,x              ; tile, attr
    sta.w oam_lo+2,y
    lda.b T0
    bpl @nohi
    phx
    tya
    lsr a
    lsr a
    and.w #3
    tax
    lda.l gap_hibit,x
    sta.b T1
    tya
    lsr a
    lsr a
    lsr a
    lsr a
    tax
    sep #$20
    .ACCU 8
    lda.w oam_hi,x
    ora.b T1
    sta.w oam_hi,x
    rep #$20
    .ACCU 16
    plx
@nohi:
    iny
    iny
    iny
    iny
@next:
    txa
    clc
    adc.w #8
    tax
    cmp.w #256
    bcc @loop
    cpy.w #128
    bcs @done
    sep #$20
    .ACCU 8
    lda.b #240
-   sta.w oam_lo+1,y
    iny
    iny
    iny
    iny
    cpy.w #128
    bcc -
    rep #$20
    .ACCU 16
@done:
    lda.w cam_x
    clc
    adc.w #3
    sta.w cam_x
    inc.w cam_y
    rtl
.ENDS

;------------------------------------------------------------------------------
; col / row: a fixed count, so no loop: 32 times the same four instructions.
;------------------------------------------------------------------------------
.SECTION ".gap_col" SUPERFREE
.ACCU 16
.INDEX 16
k_col_asm:
    lda.w map_y
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc.w map_x
    tax
    .REPT 32 INDEX N
    lda.l map+N*64,x
    and.w #$00FF
    asl a
    tay
    lda.w tile_tab,y
    sta.w colbuf+N*2
    .ENDR
    lda.w map_x
    inc a
    and.w #31
    sta.w map_x
    rtl
.ENDS

.SECTION ".gap_row" SUPERFREE
.ACCU 16
.INDEX 16
k_row_asm:
    lda.w map_y
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc.w map_x
    tax
    .REPT 32 INDEX N
    lda.l map+N,x
    and.w #$00FF
    asl a
    tay
    lda.w tile_tab,y
    sta.w rowbuf+N*2
    .ENDR
    lda.w map_y
    inc a
    and.w #31
    sta.w map_y
    rtl
.ENDS

;------------------------------------------------------------------------------
; box: the four bounds in the direct page; a signed compare is the N flag of
; a subtraction, which holds because the coordinates are small.
;------------------------------------------------------------------------------
.SECTION ".gap_box" SUPERFREE
.ACCU 16
.INDEX 16
k_box_asm:
    lda.w pbox+0
    sta.b T0                    ; px
    clc
    adc.w pbox+4
    sta.b T2                    ; px2
    lda.w pbox+2
    sta.b T1                    ; py
    clc
    adc.w pbox+6
    sta.b T3                    ; py2
    stz.b T4                    ; n
    ldx.w #0
    ldy.w #0
@loop:
    lda.w boxes+0,x
    cmp.b T2
    bpl @miss                   ; b.x >= px2
    clc
    adc.w boxes+4,x
    dec a
    cmp.b T0
    bmi @miss                   ; b.x + b.w <= px
    lda.w boxes+2,x
    cmp.b T3
    bpl @miss
    clc
    adc.w boxes+6,x
    dec a
    cmp.b T1
    bmi @miss
    inc.b T4
    sep #$20
    .ACCU 8
    lda.b #1
    bra @store
@miss:
    sep #$20
    .ACCU 8
    lda.b #0
@store:
    sta.w hit,y
    rep #$20
    .ACCU 16
    iny
    txa
    clc
    adc.w #8
    tax
    cpy.w #32
    bcc @loop
    lda.b T4
    clc
    adc.w hits
    sta.w hits
    lda.b T0
    clc
    adc.w #5
    cmp.w #251
    bmi +
    lda.w #-30
+   sta.w pbox+0
    lda.b T1
    clc
    adc.w #3
    sta.w pbox+2
    rtl
.ENDS

;------------------------------------------------------------------------------
; fixed: the hardware multiplier, twice an element: low byte times scale,
; high byte times scale, and the sign put back.
; (v * s) >> 8 = ((vl * s) >> 8) + vh * s, vh signed.
;------------------------------------------------------------------------------
.SECTION ".gap_fixed" SUPERFREE
.ACCU 16
.INDEX 16
k_fixed_asm:
    lda.w fx_scale
    and.w #$00FF
    xba
    sta.b T0                    ; scale << 8
    sep #$20
    .ACCU 8
    lda.w fx_scale
    sta.w $4202
    ldx.w #30
@loop:
    lda.w fx_vel,x
    sta.w $4203                 ; vl * s
    lda.w fx_vel+1,x
    rep #$20
    .ACCU 16
    ldy.w $4216                 ; read 10 cycles after the write
    sep #$20
    .ACCU 8
    sta.w $4203                 ; vh (unsigned) * s
    rep #$20
    .ACCU 16
    tya
    xba
    and.w #$00FF                ; (vl * s) >> 8
    clc
    adc.w $4216
    ldy.w fx_vel,x
    bpl +
    sec
    sbc.b T0                    ; vh was negative: minus s << 8
+   clc
    adc.w fx_pos,x
    sta.w fx_pos,x
    sep #$20
    .ACCU 8
    dex
    dex
    bpl @loop
    lda.w fx_scale
    clc
    adc.b #7
    sta.w fx_scale
    rep #$20
    .ACCU 16
    rtl
.ENDS

;------------------------------------------------------------------------------
; mul: the hardware multiplier, the whole loop in 8 bits.
;------------------------------------------------------------------------------
.SECTION ".gap_mul" SUPERFREE
.ACCU 16
.INDEX 16
k_mul_asm:
    sep #$20
    .ACCU 8
    ldx.w #0
    ldy.w #0
@loop:
    lda.w mu_a,x
    sta.w $4202
    lda.w mu_b,x
    sta.w $4203
    lda.w mu_a,x
    clc
    adc.b #3
    sta.w mu_a,x
    lda.w $4216
    sta.w mu_p,y
    lda.w $4217
    sta.w mu_p+1,y
    iny
    iny
    inx
    cpx.w #16
    bne @loop
    rep #$20
    .ACCU 16
    rtl
.ENDS

;------------------------------------------------------------------------------
; ents: X is the structure; a field is an offset from it.
;------------------------------------------------------------------------------
.SECTION ".gap_ents" SUPERFREE
.ACCU 16
.INDEX 16
k_ents_asm:
    ldx.w #0
@loop:
    lda.w ents+8,x              ; timer, state
    cmp.w #$0100
    bcc @next
    lda.w ents+0,x
    clc
    adc.w ents+4,x
    sta.w ents+0,x
    lda.w ents+2,x
    clc
    adc.w ents+6,x
    sta.w ents+2,x
    sep #$20
    .ACCU 8
    dec.w ents+8,x
    bne @next8
    lda.b #6
    sta.w ents+8,x
    rep #$20
    .ACCU 16
    lda.w #0
    sec
    sbc.w ents+4,x
    sta.w ents+4,x
    lda.w ents+10,x
    beq @dead
    dec a
    sta.w ents+10,x
    bra @next
@dead:
    sep #$20
    .ACCU 8
    stz.w ents+9,x
@next8:
    rep #$20
    .ACCU 16
@next:
    txa
    clc
    adc.w #12
    tax
    cpx.w #32*12
    bcc @loop
    rtl
.ENDS

;------------------------------------------------------------------------------
; call: the arguments in A, Y and the direct page; a near call.
;------------------------------------------------------------------------------
.SECTION ".gap_call" SUPERFREE
.ACCU 16
.INDEX 16
k_call_asm:
    ldy.w #0
@loop:
    tya
    and.w #7
    asl a
    tax
    lda.w #1000
    sta.b T0
    lda.w ca_acc,x
    jsr gap_call_f
    sta.w ca_acc,x
    iny
    cpy.w #64
    bcc @loop
    rtl

; A = a, Y = b, T0 = lim; returns in A; X and Y kept.
gap_call_f:
    sty.b T1
    clc
    adc.b T1
    cmp.b T0
    bcc +
    sbc.b T0
+   rts
.ENDS

;------------------------------------------------------------------------------
; soa: Y indexes the word arrays, X the byte arrays.
;------------------------------------------------------------------------------
.SECTION ".gap_soa" SUPERFREE
.ACCU 16
.INDEX 16
k_soa_asm:
    ldx.w #0
    ldy.w #0
@loop:
    lda.w ac_dx,y
    ora.w ac_dy,y
    beq @still
    lda.w ac_x,y
    cmp.w #623+1
    bmi +
    lda.w #623
    bra ++
+   cmp.w #16
    bpl ++
    lda.w #16
++  clc
    adc.w ac_dx,y
    sta.w ac_x,y
    lda.w ac_y,y
    cmp.w #455+1
    bmi +
    lda.w #455
    bra ++
+   cmp.w #24
    bpl ++
    lda.w #24
++  clc
    adc.w ac_dy,y
    sta.w ac_y,y
    bra @record
@still:
    lda.w ac_dirty,x
    and.w #$00FF
    beq @next
@record:
    sep #$20
    .ACCU 8
    stz.w ac_dirty,x
    lda.b #1
    sta.w sp_changed
    rep #$20
    .ACCU 16
    lda.w ac_frame,y
    cmp.w #24
    bcs +
    sta.w sp_frame,y
+   asl a
    stx.b T0
    tax
    lda.l frame_off,x
    ldx.b T0
    clc
    adc.w ac_x,y
    sec
    sbc.w #16
    sta.w sp_x,y
    lda.w ac_y,y
    sec
    sbc.w #16
    sta.w sp_y,y
    lda.w ac_flags,x
    and.w #$0010                ; 16 when the flag is set: the amount itself
    sta.b T1
    lda.w ac_y,y
    sec
    sbc.b T1
    cmp.w sp_depth,y
    beq @next
    sta.w sp_depth,y
    inc.w sp_depth_changed
@next:
    iny
    iny
    inx
    cpx.w #18
    bcs @out
    jmp @loop
@out:
    rtl
.ENDS

;------------------------------------------------------------------------------
; glide: Y indexes the lists; the table byte is sign-extended by hand.
;------------------------------------------------------------------------------
.SECTION ".gap_glide" SUPERFREE
.ACCU 16
.INDEX 16
k_glide_asm:
    ldy.w #0
@loop:
    lda.w dl_x,y
    sec
    sbc.w dl_px,y
    clc
    adc.w #48
    cmp.w #97
    bcs @farx
    clc
    adc.w gl_row
    tax
    lda.l glide_tab,x
    and.w #$00FF
    cmp.w #$0080
    bcc +
    ora.w #$FF00
+   clc
    adc.w dl_px,y
    bra @gotx
@farx:
    lda.w dl_x,y
@gotx:
    sec
    sbc.w gl_cam
    sta.b T0                    ; sx
    lda.w dl_y,y
    sec
    sbc.w dl_py,y
    clc
    adc.w #48
    cmp.w #97
    bcs @fary
    clc
    adc.w gl_row
    tax
    lda.l glide_tab,x
    and.w #$00FF
    cmp.w #$0080
    bcc +
    ora.w #$FF00
+   clc
    adc.w dl_py,y
    bra @goty
@fary:
    lda.w dl_y,y
@goty:
    xba                         ; sy in the high byte
    sep #$20
    .ACCU 8
    lda.b T0
    rep #$20
    .ACCU 16
    sta.b T1
    tya
    asl a
    tax
    lda.b T1
    sta.w gl_oam,x
    lda.b T0
    and.w #$0100
    cmp.w gl_x9,y
    beq +
    sta.w gl_x9,y
    inc.w gl_changed
+   iny
    iny
    cpy.w #96
    bcs @out
    jmp @loop
@out:
    lda.w #97
    sec
    sbc.w gl_row
    sta.w gl_row
    lda.w gl_cam
    clc
    adc.w #9
    sta.w gl_cam
    rtl
.ENDS
