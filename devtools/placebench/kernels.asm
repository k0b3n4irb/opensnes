; placebench — the driver, and the call wrappers. Same driver as gapbench:
; for each row, setup, CALLS calls between bench_mark_a and bench_mark_b,
; interrupts off, then the checksum. A kernel pushes the arguments the C
; caller would (batch far pointer, cam_x, cam_y: left to right) and calls one
; of three routines: the library's oamPlaceWorld, place_old (the library's
; body before LIB place prod), or an empty one
; (k_empty: what run.py subtracts, so a figure is the routine's body alone).
; Each table line ends with "; ROW VARIANT" for run.py.

.DEFINE CALLS 16

.SECTION ".pb_driver" SUPERFREE
.ACCU 16
.INDEX 16
bench_table:
    .dl s_nop, k_empty, c_nop ; empty empty
    .dl s_mix16_plain, k_old, c_sum ; mix16_plain old
    .dl s_mix16_plain, k_new, c_sum ; mix16_plain new
    .dl s_mix16_order, k_old, c_sum ; mix16_order old
    .dl s_mix16_order, k_new, c_sum ; mix16_order new
    .dl s_mix16_vis, k_old, c_sum ; mix16_vis old
    .dl s_mix16_vis, k_new, c_sum ; mix16_vis new
    .dl s_mix16_ordvis, k_old, c_sum ; mix16_ordvis old
    .dl s_mix16_ordvis, k_new, c_sum ; mix16_ordvis new
    .dl s_mix8_plain, k_old, c_sum ; mix8_plain old
    .dl s_mix8_plain, k_new, c_sum ; mix8_plain new
    .dl s_mix8_order, k_old, c_sum ; mix8_order old
    .dl s_mix8_order, k_new, c_sum ; mix8_order new
    .dl s_mix8_vis, k_old, c_sum ; mix8_vis old
    .dl s_mix8_vis, k_new, c_sum ; mix8_vis new
    .dl s_mix8_ordvis, k_old, c_sum ; mix8_ordvis old
    .dl s_mix8_ordvis, k_new, c_sum ; mix8_ordvis new
    .dl s_mix32_plain, k_old, c_sum ; mix32_plain old
    .dl s_mix32_plain, k_new, c_sum ; mix32_plain new
    .dl s_mix32_order, k_old, c_sum ; mix32_order old
    .dl s_mix32_order, k_new, c_sum ; mix32_order new
    .dl s_mix32_vis, k_old, c_sum ; mix32_vis old
    .dl s_mix32_vis, k_new, c_sum ; mix32_vis new
    .dl s_mix32_ordvis, k_old, c_sum ; mix32_ordvis old
    .dl s_mix32_ordvis, k_new, c_sum ; mix32_ordvis new
    .dl s_vis32_plain, k_old, c_sum ; vis32_plain old
    .dl s_vis32_plain, k_new, c_sum ; vis32_plain new
    .dl s_vis32_order, k_old, c_sum ; vis32_order old
    .dl s_vis32_order, k_new, c_sum ; vis32_order new
    .dl s_vis32_vis, k_old, c_sum ; vis32_vis old
    .dl s_vis32_vis, k_new, c_sum ; vis32_vis new
    .dl s_vis32_ordvis, k_old, c_sum ; vis32_ordvis old
    .dl s_vis32_ordvis, k_new, c_sum ; vis32_ordvis new
    .dl s_hid32_plain, k_old, c_sum ; hid32_plain old
    .dl s_hid32_plain, k_new, c_sum ; hid32_plain new
    .dl s_hid32_order, k_old, c_sum ; hid32_order old
    .dl s_hid32_order, k_new, c_sum ; hid32_order new
    .dl s_hid32_vis, k_old, c_sum ; hid32_vis old
    .dl s_hid32_vis, k_new, c_sum ; hid32_vis new
    .dl s_hid32_ordvis, k_old, c_sum ; hid32_ordvis old
    .dl s_hid32_ordvis, k_new, c_sum ; hid32_ordvis new
    .dl s_vis1_plain, k_old, c_sum ; vis1_plain old
    .dl s_vis1_plain, k_new, c_sum ; vis1_plain new
    .dl s_vis1_order, k_old, c_sum ; vis1_order old
    .dl s_vis1_order, k_new, c_sum ; vis1_order new
    .dl s_vis1_vis, k_old, c_sum ; vis1_vis old
    .dl s_vis1_vis, k_new, c_sum ; vis1_vis new
    .dl s_vis1_ordvis, k_old, c_sum ; vis1_ordvis old
    .dl s_vis1_ordvis, k_new, c_sum ; vis1_ordvis new
    .dl s_hid1_plain, k_old, c_sum ; hid1_plain old
    .dl s_hid1_plain, k_new, c_sum ; hid1_plain new
    .dl s_hid1_order, k_old, c_sum ; hid1_order old
    .dl s_hid1_order, k_new, c_sum ; hid1_order new
    .dl s_hid1_vis, k_old, c_sum ; hid1_vis old
    .dl s_hid1_vis, k_new, c_sum ; hid1_vis new
    .dl s_hid1_ordvis, k_old, c_sum ; hid1_ordvis old
    .dl s_hid1_ordvis, k_new, c_sum ; hid1_ordvis new
    .dl s_odd_plain, k_old, c_sum ; odd_plain old
    .dl s_odd_plain, k_new, c_sum ; odd_plain new
    .dl s_odd_order, k_old, c_sum ; odd_order old
    .dl s_odd_order, k_new, c_sum ; odd_order new
    .dl s_odd_vis, k_old, c_sum ; odd_vis old
    .dl s_odd_vis, k_new, c_sum ; odd_vis new
    .dl s_odd_ordvis, k_old, c_sum ; odd_ordvis old
    .dl s_odd_ordvis, k_new, c_sum ; odd_ordvis new
    .dl s_p18, k_old, c_sum ; p18 old
    .dl s_p18, k_new, c_sum ; p18 new
    .dl s_p20, k_old, c_sum ; p20 old
    .dl s_p20, k_new, c_sum ; p20 new
    .dl s_pball_in, k_old, c_sum ; pball_in old
    .dl s_pball_in, k_new, c_sum ; pball_in new
    .dl s_pball_out, k_old, c_sum ; pball_out old
    .dl s_pball_out, k_new, c_sum ; pball_out new
    .dl s_p5, k_old, c_sum ; p5 old
    .dl s_p5, k_new, c_sum ; p5 new
    .dl s_p1, k_old, c_sum ; p1 old
    .dl s_p1, k_new, c_sum ; p1 new
    .dl s_p4a, k_old, c_sum ; p4a old
    .dl s_p4a, k_new, c_sum ; p4a new
    .dl s_p7, k_old, c_sum ; p7 old
    .dl s_p7, k_new, c_sum ; p7 new
    .dl s_p4b, k_old, c_sum ; p4b old
    .dl s_p4b, k_new, c_sum ; p4b new
    .dl s_size0_plain, k_old, c_sum ; size0_plain old
    .dl s_size0_plain, k_new, c_sum ; size0_plain new
    .dl s_size0_order, k_old, c_sum ; size0_order old
    .dl s_size0_order, k_new, c_sum ; size0_order new
    .dl s_size0_vis, k_old, c_sum ; size0_vis old
    .dl s_size0_vis, k_new, c_sum ; size0_vis new
    .dl s_size0_ordvis, k_old, c_sum ; size0_ordvis old
    .dl s_size0_ordvis, k_new, c_sum ; size0_ordvis new
    .dl s_size1_plain, k_old, c_sum ; size1_plain old
    .dl s_size1_plain, k_new, c_sum ; size1_plain new
    .dl s_size1_order, k_old, c_sum ; size1_order old
    .dl s_size1_order, k_new, c_sum ; size1_order new
    .dl s_size1_vis, k_old, c_sum ; size1_vis old
    .dl s_size1_vis, k_new, c_sum ; size1_vis new
    .dl s_size1_ordvis, k_old, c_sum ; size1_ordvis old
    .dl s_size1_ordvis, k_new, c_sum ; size1_ordvis new
    .dl s_size24_plain, k_old, c_sum ; size24_plain old
    .dl s_size24_plain, k_new, c_sum ; size24_plain new
    .dl s_size24_order, k_old, c_sum ; size24_order old
    .dl s_size24_order, k_new, c_sum ; size24_order new
    .dl s_size24_vis, k_old, c_sum ; size24_vis old
    .dl s_size24_vis, k_new, c_sum ; size24_vis new
    .dl s_size24_ordvis, k_old, c_sum ; size24_ordvis old
    .dl s_size24_ordvis, k_new, c_sum ; size24_ordvis new
    .dl s_two_plain, k_old, c_sum ; two_plain old
    .dl s_two_plain, k_new, c_sum ; two_plain new
    .dl s_two_order, k_old, c_sum ; two_order old
    .dl s_two_order, k_new, c_sum ; two_order new
    .dl s_two_vis, k_old, c_sum ; two_vis old
    .dl s_two_vis, k_new, c_sum ; two_vis new
    .dl s_two_ordvis, k_old, c_sum ; two_ordvis old
    .dl s_two_ordvis, k_new, c_sum ; two_ordvis new
    .dl s_one0_plain, k_old, c_sum ; one0_plain old
    .dl s_one0_plain, k_new, c_sum ; one0_plain new
    .dl s_one0_order, k_old, c_sum ; one0_order old
    .dl s_one0_order, k_new, c_sum ; one0_order new
    .dl s_one0_vis, k_old, c_sum ; one0_vis old
    .dl s_one0_vis, k_new, c_sum ; one0_vis new
    .dl s_one0_ordvis, k_old, c_sum ; one0_ordvis old
    .dl s_one0_ordvis, k_new, c_sum ; one0_ordvis new
    .dl s_mvis32s_plain, k_new, c_sum ; mvis32s_plain mix
    .dl s_mvis32s_order, k_new, c_sum ; mvis32s_order mix
    .dl s_mvis32s_vis, k_new, c_sum ; mvis32s_vis mix
    .dl s_mvis32s_ordvis, k_new, c_sum ; mvis32s_ordvis mix
    .dl s_mvis32b_plain, k_new, c_sum ; mvis32b_plain mix
    .dl s_mvis32b_order, k_new, c_sum ; mvis32b_order mix
    .dl s_mvis32b_vis, k_new, c_sum ; mvis32b_vis mix
    .dl s_mvis32b_ordvis, k_new, c_sum ; mvis32b_ordvis mix
    .dl s_mhid32_plain, k_new, c_sum ; mhid32_plain mix
    .dl s_mhid32_order, k_new, c_sum ; mhid32_order mix
    .dl s_mhid32_vis, k_new, c_sum ; mhid32_vis mix
    .dl s_mhid32_ordvis, k_new, c_sum ; mhid32_ordvis mix
    .dl s_mvis1s_plain, k_new, c_sum ; mvis1s_plain mix
    .dl s_mvis1s_order, k_new, c_sum ; mvis1s_order mix
    .dl s_mvis1s_vis, k_new, c_sum ; mvis1s_vis mix
    .dl s_mvis1s_ordvis, k_new, c_sum ; mvis1s_ordvis mix
    .dl s_mvis1b_plain, k_new, c_sum ; mvis1b_plain mix
    .dl s_mvis1b_order, k_new, c_sum ; mvis1b_order mix
    .dl s_mvis1b_vis, k_new, c_sum ; mvis1b_vis mix
    .dl s_mvis1b_ordvis, k_new, c_sum ; mvis1b_ordvis mix
    .dl s_mhid1_plain, k_new, c_sum ; mhid1_plain mix
    .dl s_mhid1_order, k_new, c_sum ; mhid1_order mix
    .dl s_mhid1_vis, k_new, c_sum ; mhid1_vis mix
    .dl s_mhid1_ordvis, k_new, c_sum ; mhid1_ordvis mix
    .dl s_mmix16_plain, k_new, c_sum ; mmix16_plain mix
    .dl s_mmix16_order, k_new, c_sum ; mmix16_order mix
    .dl s_mmix16_vis, k_new, c_sum ; mmix16_vis mix
    .dl s_mmix16_ordvis, k_new, c_sum ; mmix16_ordvis mix
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
pb_entry:
    ldx.w bench_i
    lda.l bench_table,x
    sta.w bench_ptr
    lda.l bench_table+2,x
    sta.w bench_ptr+2
    jsr pb_call
bench_mark_a:
    lda.w #CALLS
    sta.w bench_n
    ldx.w bench_i
    lda.l bench_table+3,x
    sta.w bench_ptr
    lda.l bench_table+5,x
    sta.w bench_ptr+2
pb_rep:
    jsr pb_call
    dec.w bench_n
    bne pb_rep
bench_mark_b:
    ldx.w bench_i
    lda.l bench_table+6,x
    sta.w bench_ptr
    lda.l bench_table+8,x
    sta.w bench_ptr+2
    jsr pb_call
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
    bcc pb_entry
bench_done:
    jmp bench_done

pb_call:
    phk
    pea pb_ret-1
    jml [bench_ptr]
pb_ret:
    rep #$30
    .ACCU 16
    .INDEX 16
    rts
.ENDS

; The three kernels: the same pushes, the same call shape, three callees.
.MACRO KERNEL ARGS TARGET
    rep #$30
    lda.w cur_batch+2
    pha
    lda.w cur_batch
    pha
    lda.w cur_camx
    pha
    lda.w cur_camy
    pha
    jsl TARGET
    rep #$30
    tsc
    clc
    adc.w #8
    tcs
    rtl
.ENDM

.SECTION ".pb_kernels" SUPERFREE
.ACCU 16
.INDEX 16
k_empty:
    KERNEL pb_nothing
k_new:
    KERNEL oamPlaceWorld
k_old:
    KERNEL place_old
pb_nothing:
    rtl
.ENDS
