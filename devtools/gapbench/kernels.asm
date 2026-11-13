; gapbench — the assembly side: the driver, and each routine of kernels.c
; written by hand on the same data, the way a 65816 programmer writes it.
; Entry and exit as the C ABI has them: A, X, Y 16 bits, D = 0, DBR = 0.
; tcc__lf (40 bytes of direct page) is scratch for the length of a call.
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
.DEFINE T5 tcc__lf+10
.DEFINE T6 tcc__lf+12
.DEFINE T7 tcc__lf+14

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
    .dl s_fill,  k_fillu_c,   c_fill
    .dl s_fill,  k_fillu_asm, c_fill
    .dl s_oam,   k_oam_c,     c_oam
    .dl s_oam,   k_oam_asm,   c_oam
    .dl s_map,   k_col_c,     c_col
    .dl s_map,   k_col_asm,   c_col
    .dl s_map,   k_coli_c,    c_col
    .dl s_map,   k_coli_asm,  c_col
    .dl s_map,   k_row_c,     c_row
    .dl s_map,   k_row_asm,   c_row
    .dl s_map,   k_rowi_c,    c_row
    .dl s_map,   k_rowi_asm,  c_row
    .dl s_box,   k_box_c,     c_box
    .dl s_box,   k_box_asm,   c_box
    .dl s_fixed, k_fixed_c,   c_fixed
    .dl s_fixed, k_fixed_asm, c_fixed
    .dl s_mul,   k_mul_c,     c_mul
    .dl s_mul,   k_mul_asm,   c_mul
    .dl s_ents,  k_ents_c,    c_ents
    .dl s_ents,  k_ents_asm,  c_ents
    .dl s_ents,  k_entsi_c,   c_ents
    .dl s_ents,  k_entsi_asm, c_ents
    .dl s_call,  k_call_c,    c_call
    .dl s_call,  k_call_asm,  c_call
    .dl s_soa,   k_soa_c,     c_soa
    .dl s_soa,   k_soa_asm,   c_soa
    .dl s_glide, k_glide_c,   c_glide
    .dl s_glide, k_glide_asm, c_glide
    .dl s_slots, k_slots_c,   c_slots
    .dl s_slots, k_slots_asm, c_slots
    .dl s_state, k_state_c,   c_state
    .dl s_state, k_state_asm, c_state
    .dl s_fptr,  k_fptr_c,    c_fptr
    .dl s_fptra, k_fptr_asm,  c_fptr
    .dl s_blocks, k_blocks_c, c_blocks
    .dl s_blocks, k_blocks_asm, c_blocks
    .dl s_probe, k_probe_c,   c_probe
    .dl s_probe, k_probe_asm, c_probe
    .dl s_chain, k_chain_c,   c_chain
    .dl s_chain, k_chain_asm, c_chain
    .dl s_sprites, k_sprites_c, c_sprites
    .dl s_sprites, k_sprites_asm, c_sprites
    .dl s_unpack, k_unpack_c, c_unpack
    .dl s_unpack, k_unpack_asm, c_unpack
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
    .ACCU 16
    .INDEX 16
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
k_fillu_asm:                    ; the same twin: only the C differs
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
k_coli_asm:                    ; the same twin: only the C differs
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
k_rowi_asm:                    ; the same twin: only the C differs
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
k_entsi_asm:                    ; the same twin: only the C differs
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

;==============================================================================
; The rows below have the shapes shipped games have (README.md).
;==============================================================================

;------------------------------------------------------------------------------
; slots: the state byte read in 8 bits, four slots a turn at fixed offsets,
; the index stepped once for the four.
;------------------------------------------------------------------------------
.SECTION ".gap_slots" SUPERFREE
.ACCU 16
.INDEX 16
k_slots_asm:
    ldx.w #0
    ldy.w #0                    ; live slots
    sep #$20
    .ACCU 8
@loop:
    .REPT 4 INDEX N
    lda.w slots+1+N*16,x
    beq +
    rep #$20
    .ACCU 16
    inc.w slots+2+N*16,x
    sep #$20
    .ACCU 8
    iny
+
    .ENDR
    rep #$21                    ; 16 bits, and the carry clear
    .ACCU 16
    txa
    adc.w #64
    tax
    sep #$20
    .ACCU 8
    cpx.w #64*16
    bcc @loop
    rep #$20
    .ACCU 16
    tya
    clc
    adc.w sl_live
    sta.w sl_live
    rtl
.ENDS

;------------------------------------------------------------------------------
; state: Y is the object for the whole update; the state indexes a table of
; jumps, and each handler jumps back. x is known to stay above zero.
;------------------------------------------------------------------------------
.SECTION ".gap_state" SUPERFREE
.ACCU 16
.INDEX 16
k_state_asm:
    ldy.w #0
@loop:
    lda.w st_obj+0,y            ; state, timer
    and.w #$00FF
    asl a
    tax
    jmp (@table,x)
@s0:
    lda.w st_obj+2,y
    clc
    adc.w #2
    sta.w st_obj+2,y
    cmp.w #56
    bcc @next
    lda.w #1
    bra @state
@s1:
    lda.w st_obj+4,y
    inc a
    sta.w st_obj+4,y
    sep #$20
    .ACCU 8
    lda.w st_obj+1,y
    dec a
    sta.w st_obj+1,y
    bne @next8
    lda.b #5
    sta.w st_obj+1,y
    lda.b #2
    sta.w st_obj+0,y
    bra @next8
@s2:
    .ACCU 16
    lda.w st_obj+2,y
    sec
    sbc.w #3
    sta.w st_obj+2,y
    cmp.w #44
    bcs @next
    lda.w #3
    bra @state
@s3:
    lda.w st_obj+6,y
    clc
    adc.w st_obj+2,y
    sta.w st_obj+6,y
    lda.w #4
    bra @state
@state:                         ; reached in 16 bits too: the low byte is the state
    sep #$20
    .ACCU 8
    sta.w st_obj+0,y
@next8:
    rep #$20
    .ACCU 16
@next:
    tya
    clc
    adc.w #8
    tay
    cpy.w #32*8
    bcs @out
    jmp @loop
@out:
    rtl
@s4:
    sep #$20
    .ACCU 8
    lda.w st_obj+1,y
    beq +
    dec a
    sta.w st_obj+1,y
    bra @next8
+   lda.b #5
    sta.w st_obj+0,y
    bra @next8
@s5:
    .ACCU 16
    lda.w st_obj+4,y
    sec
    sbc.w #2
    sta.w st_obj+4,y
    bpl @next
    lda.w #0
    sta.w st_obj+4,y
    lda.w #6
    bra @state
@s6:
    lda.w st_obj+6,y
    eor.w #$5555
    sta.w st_obj+6,y
    lda.w #7
    bra @state
@s7:
    lda.w st_obj+6,y
    and.w #7
    sep #$20
    .ACCU 8
    sta.w st_obj+1,y
    lda.b #0
    bra @state
@table:
    .dw @s0, @s1, @s2, @s3, @s4, @s5, @s6, @s7
.ENDS

;------------------------------------------------------------------------------
; fptr: X is the object; the handler is a code address in the object, called
; through a one-instruction trampoline. The handlers share a bank, so the
; address is 16 bits and they return with rts.
;------------------------------------------------------------------------------
.SECTION ".gap_fptr" SUPERFREE
.ACCU 16
.INDEX 16
fp_asm_tab:                     ; as C reads it: four bytes a pointer
    .dl gap_fh0
    .db 0
    .dl gap_fh1
    .db 0
    .dl gap_fh2
    .db 0
    .dl gap_fh3
    .db 0
    .dl gap_fh4
    .db 0
    .dl gap_fh5
    .db 0
    .dl gap_fh6
    .db 0
    .dl gap_fh7
    .db 0

k_fptr_asm:
    ldx.w #0
@loop:
    lda.w fp_obj+0,x
    sta.b T0
    jsr @call
    txa
    clc
    adc.w #12
    tax
    cpx.w #32*12
    bcc @loop
    rtl
@call:
    jmp (T0)

; X = the object, kept. fn +0, x +4, y +6, val +8, timer +10.
gap_fh0:
    lda.w fp_obj+4,x
    clc
    adc.w #2
    sta.w fp_obj+4,x
    cmp.w #56
    bcc +
    lda.w #gap_fh1
    sta.w fp_obj+0,x
+   rts

gap_fh1:
    inc.w fp_obj+6,x
    sep #$20
    .ACCU 8
    dec.w fp_obj+10,x
    bne +
    lda.b #5
    sta.w fp_obj+10,x
    rep #$20
    .ACCU 16
    lda.w #gap_fh2
    sta.w fp_obj+0,x
    rts
+   rep #$20
    .ACCU 16
    rts

gap_fh2:
    lda.w fp_obj+4,x
    sec
    sbc.w #3
    sta.w fp_obj+4,x
    cmp.w #44
    bcs +
    lda.w #gap_fh3
    sta.w fp_obj+0,x
+   rts

gap_fh3:
    lda.w fp_obj+8,x
    clc
    adc.w fp_obj+4,x
    sta.w fp_obj+8,x
    lda.w #gap_fh4
    sta.w fp_obj+0,x
    rts

gap_fh4:
    sep #$20
    .ACCU 8
    lda.w fp_obj+10,x
    beq +
    dec.w fp_obj+10,x
    rep #$20
    .ACCU 16
    rts
+   rep #$20
    .ACCU 16
    lda.w #gap_fh5
    sta.w fp_obj+0,x
    rts

gap_fh5:
    lda.w fp_obj+6,x
    sec
    sbc.w #2
    sta.w fp_obj+6,x
    bpl +
    stz.w fp_obj+6,x
    lda.w #gap_fh6
    sta.w fp_obj+0,x
+   rts

gap_fh6:
    lda.w fp_obj+8,x
    eor.w #$5555
    sta.w fp_obj+8,x
    lda.w #gap_fh7
    sta.w fp_obj+0,x
    rts

gap_fh7:
    lda.w fp_obj+8,x
    and.w #7
    sep #$20
    .ACCU 8
    sta.w fp_obj+10,x
    rep #$20
    .ACCU 16
    lda.w #gap_fh0
    sta.w fp_obj+0,x
    rts
.ENDS

;------------------------------------------------------------------------------
; blocks: a fixed count, so no loop: the map row at fixed offsets from its
; start, X the block's four words, Y the place in the ring.
;------------------------------------------------------------------------------
.SECTION ".gap_blocks" SUPERFREE
.ACCU 16
.INDEX 16
k_blocks_asm:
    lda.w bk_y
    xba
    lsr a                       ; row * 64 words
    sta.b T0
    lda.w bk_x
    asl a
    adc.b T0                    ; the carry is clear: small values
    sta.b T0                    ; the first map word of the row
    lda.w bk_x
    asl a
    asl a
    and.w #$003F
    tay                         ; the first place in the ring, in bytes
    .REPT 16 INDEX N
    ldx.b T0
    lda.l bk_map+N*2,x
    and.w #$03FF
    asl a
    asl a
    asl a
    tax
    lda.l bk_def+0,x
    sta.w bk_top+0,y
    lda.l bk_def+2,x
    sta.w bk_top+2,y
    lda.l bk_def+4,x
    sta.w bk_bot+0,y
    lda.l bk_def+6,x
    sta.w bk_bot+2,y
    tya
    adc.w #4                    ; the carry is clear: the block number was shifted out of 10 bits
    and.w #$003F
    tay
    .ENDR
    lda.w bk_x
    inc a
    and.w #31
    sta.w bk_x
    lda.w bk_y
    inc a
    and.w #15
    sta.w bk_y
    rtl
.ENDS

;------------------------------------------------------------------------------
; probe: the cell from shifts and masks kept in place (no multiply, the two
; fields do not overlap), the two flags read from N after a shift.
;------------------------------------------------------------------------------
.SECTION ".gap_probe" SUPERFREE
.ACCU 16
.INDEX 16
k_probe_asm:
    stz.b T1                    ; the probe, as a byte index
    stz.b T2                    ; hits
    ldy.w #0
@loop:
    lda.w pr_y,y
    and.w #$00F0
    asl a
    asl a
    asl a                       ; row * 64 words
    sta.b T0
    lda.w pr_x,y
    lsr a
    lsr a
    lsr a
    and.w #$007E                ; column, in words
    ora.b T0
    tax
    lda.l bk_map,x
    bmi @solid
    asl a
    bmi @hazard
    lda.w #0
    bra @store
@solid:
    lda.w #1
    bra +
@hazard:
    lda.w #2
+   inc.b T2
@store:
    ldx.b T1
    sep #$20
    .ACCU 8
    sta.w pr_hit,x
    rep #$20
    .ACCU 16
    inx
    stx.b T1
    lda.w pr_x,y
    clc
    adc.w #5
    and.w #$03FF
    sta.w pr_x,y
    iny
    iny
    cpy.w #48
    bcc @loop
    lda.b T2
    clc
    adc.w pr_hits
    sta.w pr_hits
    rtl
.ENDS

;------------------------------------------------------------------------------
; chain: near calls; X is the actor and every callee keeps it; arguments in
; A and Y, the result in A and in the flags; a range test as one unsigned
; compare.
;------------------------------------------------------------------------------
.SECTION ".gap_chain" SUPERFREE
.ACCU 16
.INDEX 16
k_chain_asm:
    ldx.w #0
@loop:
    jsr gap_ch_update
    txa
    clc
    adc.w #22
    tax
    cpx.w #16*22
    bcc @loop
    rtl

; X = the actor. x +0, v +2, flags +4, frame +6, life +8, y +10, vy +12,
; tick +14, rate +16, score +18, cell +20.
gap_ch_update:
    lda.w ch_act+4,x
    ldy.w #1
    jsr gap_ch_test
    beq +
    lda.w ch_act+0,x
    clc
    adc.w ch_act+2,x
    ldy.w #300
    jsr gap_ch_clamp
    sta.w ch_act+0,x
+   lda.w ch_act+6,x
    jsr gap_ch_anim
    sta.w ch_act+6,x
    tay
    bne +
    lda.w ch_act+4,x
    eor.w #4
    sta.w ch_act+4,x
+   lda.w ch_act+10,x
    clc
    adc.w ch_act+12,x
    sec
    sbc.w #20
    cmp.w #181                  ; 20..200, one unsigned compare
    bcc +
    lda.w #0
    sec
    sbc.w ch_act+12,x
    sta.w ch_act+12,x
    bra ++
+   adc.w #20                   ; the carry is clear
    sta.w ch_act+10,x
++
    lda.w ch_act+8,x
    jsr gap_ch_decay
    sta.w ch_act+8,x
    bne +
    lda.w ch_act+4,x
    ldy.w #2
    jsr gap_ch_test
    beq +
    lda.w #20
    sta.w ch_act+8,x
    stz.w ch_act+0,x
    inc.w ch_loops
+   lda.w ch_act+14,x
    beq @reload
    dec.w ch_act+14,x
    bra +
@reload:
    lda.w ch_act+16,x
    sta.w ch_act+14,x
    lda.w ch_act+6,x
    sec                         ; frame + 1
    adc.w ch_act+18,x
    sta.w ch_act+18,x
+   lda.w ch_act+0,x
    lsr a
    lsr a
    lsr a
    lsr a
    sta.b T0
    lda.w ch_act+4,x
    and.w #4
    xba
    lsr a
    lsr a                       ; the flag, six bits up
    adc.b T0                    ; the carry is clear
    sta.b T0
    lda.w ch_act+10,x
    and.w #$00F0
    adc.b T0
    sta.w ch_act+20,x
    rts

gap_ch_test:                    ; A = flags, Y = mask; A and Z
    sty.b T0
    and.b T0
    rts

gap_ch_clamp:                   ; A = value, Y = max
    sty.b T0
    cmp.b T0
    bcc +
    tya
+   rts

gap_ch_anim:                    ; A = frame
    inc a
    cmp.w #6
    bcc +
    lda.w #0
+   rts

gap_ch_decay:                   ; A = life; A and Z
    tay
    beq +
    dec a
+   rts
.ENDS

;------------------------------------------------------------------------------
; sprites: Y is the place in the list from the first tile to the last; X is
; the object, then its frame; both range tests are one unsigned compare, an
; entry is two word stores. y is known to stay above zero.
;------------------------------------------------------------------------------
.SECTION ".gap_sprites" SUPERFREE
.ACCU 16
.INDEX 16
k_sprites_asm:
    lda.w #1
    sta.b T0                    ; the insertion pass
@ins:
    ldx.b T0
    lda.w mo_order,x
    and.w #$00FF
    sta.b T1                    ; the object to place
    asl a
    asl a
    asl a
    tay
    lda.w mobs+2,y
    sta.b T2                    ; its y
@shift:
    lda.w mo_order-1,x
    and.w #$00FF
    asl a
    asl a
    asl a
    tay
    lda.w mobs+2,y
    cmp.b T2
    bcc @place
    beq @place
    sep #$20
    .ACCU 8
    lda.w mo_order-1,x
    sta.w mo_order,x
    rep #$20
    .ACCU 16
    dex
    bne @shift
@place:
    sep #$20
    .ACCU 8
    lda.b T1
    sta.w mo_order,x
    rep #$20
    .ACCU 16
    lda.b T0
    inc a
    sta.b T0
    cmp.w #12
    bcc @ins

    stz.w mo_hi
    stz.w mo_hi+2
    stz.w mo_hi+4
    stz.w mo_hi+6
    stz.w mo_hi+8
    stz.w mo_hi+10
    stz.b T0                    ; the place in the order
    ldy.w #0                    ; the place in the list
@obj:
    ldx.b T0
    lda.w mo_order,x
    and.w #$00FF
    asl a
    asl a
    asl a
    tax
    lda.w mobs+0,x
    sec
    sbc.w mo_cam_x
    sta.b T1                    ; sx
    lda.w mobs+2,x
    sec
    sbc.w mo_cam_y
    sta.b T2                    ; sy
    lda.w mobs+2,x
    clc
    adc.w mobs+4,x
    sta.w mobs+2,x
    lda.w mobs+6,x              ; frame, attr
    pha
    and.w #$FF00
    sta.b T3                    ; attr, in the high byte
    pla
    and.w #$00FF
    asl a
    asl a
    asl a
    asl a
    tax                         ; the frame
    lda.l mo_frames,x
    and.w #$00FF
    sta.b T4                    ; tiles
@tile:
    lda.l mo_frames+4,x
    and.w #$00FF
    clc
    adc.b T1
    sta.b T5                    ; x of the tile
    clc
    adc.w #15
    cmp.w #271                  ; -15..255
    bcs @skip
    lda.l mo_frames+5,x
    and.w #$00FF
    clc
    adc.b T2
    clc
    adc.w #15
    cmp.w #239                  ; -15..223
    bcs @skip
    sbc.w #14                   ; the carry is clear: minus 15
    xba                         ; y in the high byte
    sep #$20
    .ACCU 8
    lda.b T5
    rep #$20
    .ACCU 16
    sta.w mo_oam,y
    lda.l mo_frames+6,x
    and.w #$00FF
    ora.b T3
    sta.w mo_oam+2,y
    lda.b T5
    bpl @nohi
    phx
    tya
    lsr a
    lsr a
    and.w #3
    tax
    lda.l gap_hibit,x
    sta.b T6
    tya
    lsr a
    lsr a
    lsr a
    lsr a
    tax
    sep #$20
    .ACCU 8
    lda.w mo_hi,x
    ora.b T6
    sta.w mo_hi,x
    rep #$20
    .ACCU 16
    plx
@nohi:
    iny
    iny
    iny
    iny
@skip:
    inx
    inx
    inx
    dec.b T4
    bne @tile
    lda.b T0
    inc a
    sta.b T0
    cmp.w #12
    bcs @hide
    jmp @obj
@hide:
    cpy.w #12*16
    bcs @done
    sep #$20
    .ACCU 8
    lda.b #240
-   sta.w mo_oam+1,y
    iny
    iny
    iny
    iny
    cpy.w #12*16
    bcc -
    rep #$20
    .ACCU 16
@done:
    lda.w mo_cam_x
    inc a
    inc a
    sta.w mo_cam_x
    rtl
.ENDS

;------------------------------------------------------------------------------
; unpack: the block move for the literals and for the copies (it moves upward
; a byte at a time, which is what an overlapping copy needs); the data bank
; set once; the token and the byte after it read as one word.
;------------------------------------------------------------------------------
.SECTION ".gap_unpack" SUPERFREE
.ACCU 16
.INDEX 16
k_unpack_asm:
    phb
    sep #$20
    .ACCU 8
    lda.b #$7E
    pha
    plb
    rep #$20
    .ACCU 16
    ldx.w #un_src
    ldy.w #un_dst
@loop:
    lda.w 0,x                   ; the token, and the byte after it
    inx
    bit.w #$0080
    bne @copy
    and.w #$007F                ; literals - 1
    mvn $7E,$7E
    bra @next
@copy:
    inx
    stx.b T0
    sta.b T1
    xba
    and.w #$00FF
    sta.b T2                    ; distance - 1
    tya
    clc
    sbc.b T2
    tax                         ; from: the output, distance back
    lda.b T1
    and.w #$007F
    inc a
    inc a                       ; length - 1
    mvn $7E,$7E
    ldx.b T0
@next:
    cpy.w #un_dst+512
    bcc @loop
    plb
    rtl
.ENDS
