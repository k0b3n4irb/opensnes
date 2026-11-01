;==============================================================================
; OpenSNES Dynamic Sprite Engine
;
; Provides runtime VRAM uploading for sprite graphics during VBlank.
; Based on PVSnesLib's dynamic sprite system by Alekmaul.
;
; Features:
;   - Per-sprite state tracking via oambuffer[]
;   - VRAM upload queue (max 7 sprites/frame)
;   - Support for 32x32, 16x16, and 8x8 sprites
;
; Author: OpenSNES Team
; License: MIT (original code zlib by Alekmaul)
;
;------------------------------------------------------------------------------
; C1 AUDIT VERDICT (2026-07-20): KEEP AS ASM — probe-backed.
;
; This module is already the hybrid the audit aims for: cold paths
; (dispatch, size helpers, metasprite walker) live in C next door
; (sprite_dynamic_dispatch.c / _helpers.c / _meta.c); THIS file keeps
; only the hot core. A C model of the steady oamDynamic16Draw path
; (real engine state via bank-0 externs, computed masks) measured
; ~+50 % on the draw+EndFrame pair (1486 vs 968 cyc) even after a
; codegen-friendly rewrite: the ASM pins X (OAM offset), Y (oambuffer
; offset) and DB ($7E = WRAM mirror) across the whole function while
; compiled C re-derives every array index per statement — a register-
; allocation gap, not a peephole one. Regressions here multiply by
; sprite count per frame AND eat VBlank budget in oamDynamicNmiFlush,
; the strictest zone in the lib. Numbers + method:
; .claude/notes/chantiers/c1_asm_audit.md.
;
; INTERNAL INVARIANTS (read before editing):
; - DB discipline: draw paths set DB=$7E and reach bank-0 WRAM through
;   the $7E:0000-$1FFF mirror (absolute,Y on oambuffer/oamMemory);
;   ROM LUTs are read with `.l` long addressing. Restore DB on exit.
; - Register pinning: X = OAM byte offset (oamnumberperframe), Y =
;   oambuffer byte offset (id*16) across each draw function; the
;   php/carry trick around the X-high-bit ror MUST keep the pair
;   (see _o16d_xhigh) — the carry crosses a mode switch.
; - oamDynamicNmiFlush runs INSIDE the NMI handler: VBlank budget
;   applies (idle floor ~283 cyc; each queued 16x16 refresh adds
;   ~1100 incl. DMA). Max 7 queued uploads/frame by design.
; - oambuffer's 16-byte layout is a PUBLIC C contract (t_sprites in
;   sprite.h, compile-time asserts) — fields move in lockstep.
; - After any rep/sep: explicit .ACCU/.INDEX markers (WLA-DX loses
;   tracking after branch merges — lint-enforced).
;==============================================================================

.ifdef SA1
.include "memmap_sa1.inc"
.else
.ifdef HIROM
.include "memmap_hirom.inc"
.else
.include "memmap.inc"
.endif
.endif

;------------------------------------------------------------------------------
; Constants
;------------------------------------------------------------------------------

.EQU OBJ_SPRITE32           1           ; 32x32 sprite identifier
.EQU OBJ_SPRITE16           2           ; 16x16 sprite identifier
.EQU OBJ_SPRITE8            4           ; 8x8 sprite identifier

.EQU OBJ_SIZE16_L32         3           ; Size index (matches C header), NOT register value
.EQU OBJ_QUEUELIST_SIZE     128         ; Max queue entries
.EQU MAXSPRTRF              42          ; 7 sprites * 6 bytes per entry

;------------------------------------------------------------------------------
; oambuffer structure offsets (16 bytes per entry)
;------------------------------------------------------------------------------
.EQU OAM_OAMX               0           ; s16 X position
.EQU OAM_OAMY               2           ; s16 Y position
.EQU OAM_FRAMEID            4           ; u16 frame index
.EQU OAM_ATTRIBUTE          6           ; u8 attributes
.EQU OAM_REFRESH            7           ; u8 refresh flag
.EQU OAM_GRAPHICS           8           ; u24 graphics pointer (3 bytes + pad)

;------------------------------------------------------------------------------
; Compile-time offset assertions (must match t_sprites in sprite.h)
;------------------------------------------------------------------------------
.ASSERT OAM_OAMX == 0
.ASSERT OAM_OAMY == 2
.ASSERT OAM_FRAMEID == 4
.ASSERT OAM_ATTRIBUTE == 6
.ASSERT OAM_REFRESH == 7
.ASSERT OAM_GRAPHICS == 8

;==============================================================================
; Dynamic Sprite RAM Buffer
;==============================================================================
; oambuffer MUST be in Bank $00 SLOT 1 ($0000-$1FFF) because the C compiler
; generates `sta.l $0000,x` which always writes to bank $00.
; Assembly code with DB=$7E reads the mirror ($7E:0000-$1FFF = $00:0000-$1FFF).
;==============================================================================

.RAMSECTION ".dynamic_sprite_buffer" BANK 0 SLOT 1
    oambuffer       dsb 2048    ; 128 × 16 bytes
.ENDS

; VRAM upload queue + engine state. Moved here from crt0.asm (2026-08) so they
; are allocated ONLY when a project links this module — crt0 reserved them
; unconditionally, costing ~794 bytes of WRAM in every ROM (a stack-overflow
; hazard for RAM-tight games). Same BANK 0 SLOT 1 requirement as oambuffer.
.RAMSECTION ".dynamic_sprite_queue" BANK 0 SLOT 1
    oamQueueEntry   dsb 768     ; 128 × 6 bytes (VRAM upload queue)
.ENDS

.RAMSECTION ".dynamic_sprite_state" BANK 0 SLOT 1
    ; Temporary values for sprite calculations
    sprit_val0      dsb 1       ; Temporary value #0
    sprit_val1      dsb 1       ; Temporary value #1
    sprit_val2      dsb 2       ; Temporary value #2 (16-bit)

    ; Queue state
    oamqueuenumber          dsb 2   ; Current position in VRAM upload queue

    ; Per-frame sprite tracking
    oamnumberperframe       dsb 2   ; Number of sprites drawn this frame (×4)
    oamnumberperframeold    dsb 2   ; Number of sprites drawn last frame (×4)

    ; Sprite slot counters (for each size category)
    oamnumberspr0           dsb 2   ; Current large sprite slot
    oamnumberspr0Init       dsb 2   ; Initial large sprite slot
    oamnumberspr1           dsb 2   ; Current small sprite slot
    oamnumberspr1Init       dsb 2   ; Initial small sprite slot

    ; VRAM base addresses
    spr0addrgfx             dsb 2   ; VRAM address for large sprites ($0000)
    spr1addrgfx             dsb 2   ; VRAM address for small sprites ($1000)
    spr16addrgfx            dsb 2   ; VRAM address for 16x16 sprites (context-dependent)

    ; Metasprite temporary storage
    sprit_mxsvg             dsb 2   ; Saved metasprite X origin
    sprit_mysvg             dsb 2   ; Saved metasprite Y origin
.ENDS

;==============================================================================
; Lookup Tables for OAM High Table Manipulation
;==============================================================================

.SECTION ".lut_oam_masks" SUPERFREE

;------------------------------------------------------------------------------
; oammask - 4 bytes per sprite entry (indexed by id*4)
;   Byte 0: X high bit set mask
;   Byte 1: mask that clears the X high bit AND the size bit of the sprite
;           (until 2026-11-01 it cleared the X bit alone)
;   Byte 2: High table byte index (id/4), to add to oamMemory+512
;   Byte 3: size bit = large (the X bit shifted left once)
;------------------------------------------------------------------------------
oammask:
    .byte $01,$fc,$00,$02, $04,$f3,$00,$08, $10,$cf,$00,$20, $40,$3f,$00,$80
    .byte $01,$fc,$01,$02, $04,$f3,$01,$08, $10,$cf,$01,$20, $40,$3f,$01,$80
    .byte $01,$fc,$02,$02, $04,$f3,$02,$08, $10,$cf,$02,$20, $40,$3f,$02,$80
    .byte $01,$fc,$03,$02, $04,$f3,$03,$08, $10,$cf,$03,$20, $40,$3f,$03,$80
    .byte $01,$fc,$04,$02, $04,$f3,$04,$08, $10,$cf,$04,$20, $40,$3f,$04,$80
    .byte $01,$fc,$05,$02, $04,$f3,$05,$08, $10,$cf,$05,$20, $40,$3f,$05,$80
    .byte $01,$fc,$06,$02, $04,$f3,$06,$08, $10,$cf,$06,$20, $40,$3f,$06,$80
    .byte $01,$fc,$07,$02, $04,$f3,$07,$08, $10,$cf,$07,$20, $40,$3f,$07,$80
    .byte $01,$fc,$08,$02, $04,$f3,$08,$08, $10,$cf,$08,$20, $40,$3f,$08,$80
    .byte $01,$fc,$09,$02, $04,$f3,$09,$08, $10,$cf,$09,$20, $40,$3f,$09,$80
    .byte $01,$fc,$0a,$02, $04,$f3,$0a,$08, $10,$cf,$0a,$20, $40,$3f,$0a,$80
    .byte $01,$fc,$0b,$02, $04,$f3,$0b,$08, $10,$cf,$0b,$20, $40,$3f,$0b,$80
    .byte $01,$fc,$0c,$02, $04,$f3,$0c,$08, $10,$cf,$0c,$20, $40,$3f,$0c,$80
    .byte $01,$fc,$0d,$02, $04,$f3,$0d,$08, $10,$cf,$0d,$20, $40,$3f,$0d,$80
    .byte $01,$fc,$0e,$02, $04,$f3,$0e,$08, $10,$cf,$0e,$20, $40,$3f,$0e,$80
    .byte $01,$fc,$0f,$02, $04,$f3,$0f,$08, $10,$cf,$0f,$20, $40,$3f,$0f,$80
    .byte $01,$fc,$10,$02, $04,$f3,$10,$08, $10,$cf,$10,$20, $40,$3f,$10,$80
    .byte $01,$fc,$11,$02, $04,$f3,$11,$08, $10,$cf,$11,$20, $40,$3f,$11,$80
    .byte $01,$fc,$12,$02, $04,$f3,$12,$08, $10,$cf,$12,$20, $40,$3f,$12,$80
    .byte $01,$fc,$13,$02, $04,$f3,$13,$08, $10,$cf,$13,$20, $40,$3f,$13,$80
    .byte $01,$fc,$14,$02, $04,$f3,$14,$08, $10,$cf,$14,$20, $40,$3f,$14,$80
    .byte $01,$fc,$15,$02, $04,$f3,$15,$08, $10,$cf,$15,$20, $40,$3f,$15,$80
    .byte $01,$fc,$16,$02, $04,$f3,$16,$08, $10,$cf,$16,$20, $40,$3f,$16,$80
    .byte $01,$fc,$17,$02, $04,$f3,$17,$08, $10,$cf,$17,$20, $40,$3f,$17,$80
    .byte $01,$fc,$18,$02, $04,$f3,$18,$08, $10,$cf,$18,$20, $40,$3f,$18,$80
    .byte $01,$fc,$19,$02, $04,$f3,$19,$08, $10,$cf,$19,$20, $40,$3f,$19,$80
    .byte $01,$fc,$1a,$02, $04,$f3,$1a,$08, $10,$cf,$1a,$20, $40,$3f,$1a,$80
    .byte $01,$fc,$1b,$02, $04,$f3,$1b,$08, $10,$cf,$1b,$20, $40,$3f,$1b,$80
    .byte $01,$fc,$1c,$02, $04,$f3,$1c,$08, $10,$cf,$1c,$20, $40,$3f,$1c,$80
    .byte $01,$fc,$1d,$02, $04,$f3,$1d,$08, $10,$cf,$1d,$20, $40,$3f,$1d,$80
    .byte $01,$fc,$1e,$02, $04,$f3,$1e,$08, $10,$cf,$1e,$20, $40,$3f,$1e,$80
    .byte $01,$fc,$1f,$02, $04,$f3,$1f,$08, $10,$cf,$1f,$20, $40,$3f,$1f,$80

;------------------------------------------------------------------------------
; Size/hide bit manipulation tables (indexed by (id >> 2) & 3)
;------------------------------------------------------------------------------
oamSetExand:
    .byte $fd, $f7, $df, $7f    ; AND mask to clear size bit

oamSizeshift:
    .byte $02, $08, $20, $80    ; OR mask to set size bit (large)

oamHideand:
    .byte $fe, $fb, $ef, $bf    ; AND mask to clear X high bit (show)

oamHideshift:
    .byte $01, $04, $10, $40    ; OR mask to set X high bit (hide)

.ENDS

;==============================================================================
; oamInitDynamicSprite
;==============================================================================
; void oamInitDynamicSprite(u16 gfxsp0adr, u16 gfxsp1adr,
;                           u16 oamsp0init, u16 oamsp1init, u8 oamsize)
;
; Stack layout (after PHP/PHB/PHX/PHY = 6 bytes + 3 byte return = 9):
; C calling convention pushes arg1 first (deepest), arg5 last (shallowest)
;   10,s = oamsize (arg5, last pushed)
;   12,s = oamsp1init (arg4)
;   14,s = oamsp0init (arg3)
;   16,s = gfxsp1adr (arg2)
;   18,s = gfxsp0adr (arg1, first pushed, deepest)
;==============================================================================

.SECTION ".sprite_dynamic_init" SUPERFREE

oamInitDynamicSprite:
    php
    phb
    phx
    phy

    ; Set data bank to $7E for RAM access
    sep #$20
    .ACCU 8
    lda #$7e
    pha
    plb

    ; Clear VRAM upload queue (768 bytes)
    rep #$10                        ; 16-bit X/Y for larger loop count
    .INDEX 16
    ldx #$0000
    sep #$20                        ; 8-bit A for byte stores
    .ACCU 8
    lda #$00
-   sta.w oamQueueEntry,x
    inx
    cpx #OBJ_QUEUELIST_SIZE*6
    bne -

    rep #$20
    .ACCU 16
    stz.w oamqueuenumber            ; Reset queue position

    stz.w oamnumberperframeold      ; Reset per-frame counters
    stz.w oamnumberperframe

    ; Store VRAM base addresses (arg1=18,s, arg2=16,s)
    lda 18,s                        ; gfxsp0adr (arg1)
    sta.w spr0addrgfx
    lda 16,s                        ; gfxsp1adr (arg2)
    sta.w spr1addrgfx

    ; Store initial OAM slots (arg3=14,s, arg4=12,s)
    lda 14,s                        ; oamsp0init (arg3)
    sta.w oamnumberspr0
    sta.w oamnumberspr0Init
    lda 12,s                        ; oamsp1init (arg4)
    sta.w oamnumberspr1
    sta.w oamnumberspr1Init

    ; Initialize OAM (clear all sprites) - must do BEFORE setting OBJSEL
    ; because OpenSNES's oamInit resets OBJSEL
    jsl oamInit

    ; Set 16x16 sprite VRAM address based on size configuration
    ; MUST be AFTER oamInit — oamInit's C code may clobber spr16addrgfx
    rep #$20
    .ACCU 16
    lda.w spr1addrgfx                 ; Default: small sprites use spr1 address
    sta.w spr16addrgfx
    sep #$20
    .ACCU 8
    lda 10,s                        ; oamsize (arg5)
    cmp #OBJ_SIZE16_L32
    beq +
    rep #$20
    .ACCU 16
    lda.w spr0addrgfx                 ; If not 16/32 mode, use spr0 address
    sta.w spr16addrgfx
+:

    ; Configure OBJSEL register AFTER oamInit (OpenSNES's oamInit overwrites it)
    ; Callers pass size index (0-5), convert to register bits 7-5
    sep #$20
    .ACCU 8
    lda 10,s                        ; oamsize index (0-5)
    sta.l oam_dynamic_size_mode     ; remember mode for oamDynamicDraw dispatch
    asl a                           ; index << 5 → OBJSEL size bits (= OBJ_SIZE_TO_REG)
    asl a
    asl a
    asl a
    asl a
    sta.l $2101                     ; REG_OBJSEL

    ; Register oamDynamicNmiFlush with the NMI handler. From this point on,
    ; the VBlank ISR will call our flush automatically and the application
    ; never needs to invoke oamInitDynamicSpriteEndFrame / oamVramQueueUpdate
    ; by hand. Idempotent: re-init just rewrites the same address.
    rep #$20
    .ACCU 16
    lda #oamDynamicNmiFlush
    sta.l dynamic_flush_hook
    sep #$20
    .ACCU 8
    lda #:oamDynamicNmiFlush
    sta.l dynamic_flush_hook+2

    ply
    plx
    plb
    plp
    rtl

.ENDS

;==============================================================================
; oamDynamicNmiFlush
;==============================================================================
; void oamDynamicNmiFlush(void)
;
; Indirect-long-callable target for the NMI handler's dynamic_flush_hook.
; Called once per VBlank (after the engine is initialized) to:
;   1. End-of-frame housekeeping: hide sprites from last frame that were
;      not drawn this frame, reset slot counters, set oam_update_flag.
;   2. Flush any queued sprite tile DMAs to VRAM.
;
; The two underlying routines are idempotent for runtime use, but the
; end-frame step compares this frame's sprite count to last frame's and
; hides the difference. While the application is just waiting for the
; queue to drain (oamDynamicDrainQueue), the count drops to 0 and the
; hide path would erase the very sprites we are trying to upload — so
; we skip it whenever oam_dynamic_draining is non-zero.
;==============================================================================

.SECTION ".sprite_dynamic_nmi_flush" SUPERFREE

oamDynamicNmiFlush:
    php                                 ; preserve caller's P (NMI in rep #$20)
    sep #$20
    .ACCU 8
    lda.l oam_dynamic_draining
    bne _drain_only                     ; flag != 0 → skip end-frame
    plp
    .ACCU 16                            ; restore tracking for WLA-DX
    jsl oamInitDynamicSpriteEndFrame
    jsl oamVramQueueUpdate
    rtl

_drain_only:
    plp
    .ACCU 16
    jsl oamVramQueueUpdate
    rtl

.ENDS

;==============================================================================
; oamInitDynamicSpriteEndFrame
;==============================================================================
; void oamInitDynamicSpriteEndFrame(void)
;
; Called after drawing all sprites each frame.
; Hides sprites from previous frame that weren't drawn this frame.
;==============================================================================

.SECTION ".sprite_dynamic_endframe" SUPERFREE

oamInitDynamicSpriteEndFrame:
    php
    phb
    phx

    sep #$20
    .ACCU 8
    lda #$7e
    pha
    plb

    rep #$20
    .ACCU 16
    ldx.w oamnumberperframe           ; Current frame sprite count (×4)
    txa
    cmp.w oamnumberperframeold        ; Compare to last frame
    bcs _endframe_done              ; If current >= old, nothing to hide

    ; Hide sprites that were visible last frame but not this frame
    phy
_endframe_loop:
    ; Calculate high table offset: id >> 4 + 512
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.w #512
    tay                             ; Y = high table offset

    ; Get slot within high table byte: (id >> 2) & 3
    txa
    phx
    lsr a
    lsr a
    and.w #$03
    tax

    ; Set X high bit to push sprite off-screen
    sep #$20
    .ACCU 8
    lda.l oamHideshift,x
    ora.w oamMemory,y
    sta.w oamMemory,y

    ; Set X position to 1 (with high bit = off-screen)
    rep #$20
    .ACCU 16
    plx
    txa
    tay
    sep #$20
    .ACCU 8
    lda #$01
    sta.w oamMemory,y
    iny
    lda #$F0                        ; Y = 240 (hidden)
    sta.w oamMemory,y

    rep #$20
    .ACCU 16
    ; Next sprite (id += 4)
    inx
    inx
    inx
    inx
    txa
    cmp.w oamnumberperframeold
    bne _endframe_loop
    ply

_endframe_done:
    ; Save current count for next frame
    lda.w oamnumberperframe
    sta.w oamnumberperframeold
    lda #$0000
    sta.w oamnumberperframe

    ; Reset sprite slot counters
    lda.w oamnumberspr0Init
    sta.w oamnumberspr0
    lda.w oamnumberspr1Init
    sta.w oamnumberspr1

    ; Signal NMI handler to DMA OAM buffer to hardware
    sep #$20
    .ACCU 8
    lda #$01
    sta.l oam_update_flag
    rep #$20
    .ACCU 16

    plx
    plb
    plp
    rtl

.ENDS

;==============================================================================
; oamVramQueueUpdate
;==============================================================================
; void oamVramQueueUpdate(void)
;
; Process VRAM upload queue during VBlank.
; Transfers up to 7 sprites per frame to VRAM.
;==============================================================================

.SECTION ".sprite_dynamic_vram" SUPERFREE

oamVramQueueUpdate:
    php
    phb
    phx
    phy

    sep #$20
    .ACCU 8
    lda #$7e
    pha
    plb

    ldx.w oamqueuenumber              ; Anything to transfer?
    bne _vqu_start
    jmp _vqu_done                   ; No, exit

_vqu_start:
    lda #$80
    sta.l $2115                     ; VRAM increment mode

    rep #$20
    .ACCU 16
    stz.w oamqueuenumber            ; Assume we'll process everything
    txa                             ; A = bytes queued
    cmp #MAXSPRTRF
    bcc _vqu_process                ; Less than max, process all
    ldx #MAXSPRTRF                  ; Limit to max
    sec
    sbc #MAXSPRTRF
    sta.l oamqueuenumber            ; Save remainder for next frame

_vqu_process:
    ; Every row goes through DMA channel 7, one after the other: the NMI's
    ; OAM transfer, which runs after this hook, reprograms it anyway, and it
    ; is the one channel no game gives to HDMA (hdma.h). Until 2026-10-17
    ; the rows were preloaded on channels 1-4, which a Super FX letterbox
    ; (channel 1) and the game's own HDMA (2-4) were using (audit 1.0, 01-1).
    lda #$1801
    sta.l $4370                     ; word increment to $2118

    dex                             ; Start at last entry (will decrement more in loop)

_vqu_loop:
    dex                             ; Move to next entry (6 bytes back)
    dex
    dex
    dex
    dex

    ; Source bank, the same for every row of the entry
    sep #$20
    .ACCU 8
    lda.l oamQueueEntry+2,x
    sta.l $4374

    ; Check sprite size type
    lda.l oamQueueEntry+5,x         ; Get sprite type
    cmp #OBJ_SPRITE8
    bne +
    jmp _vqu_8x8
+:  cmp #OBJ_SPRITE16
    bne _vqu_32x32
    jmp _vqu_16x16

;------------------------------------------------------------------------------
; 32x32 sprite transfer (4 DMA transfers of 128 bytes each)
; Row n: source + n * $200, VRAM + n * $100
;------------------------------------------------------------------------------
_vqu_32x32:
    rep #$20
    .ACCU 16
    ldy #$0000                      ; row offset in the source: 0, $200, $400, $600
_vqu_32_row:
    tya
    lsr a                           ; VRAM offset = source offset / 2
    clc
    adc.l oamQueueEntry+3,x
    sta.l $2116
    tya
    clc
    adc.l oamQueueEntry,x
    sta.l $4372                     ; source of this row
    lda #$0080
    sta.l $4375                     ; 128 bytes
    sep #$20
    .ACCU 8
    lda #$80
    sta.l $420b                     ; channel 7
    rep #$20
    .ACCU 16
    tya
    clc
    adc #$0200
    tay
    cpy #$0800
    bne _vqu_32_row
    jmp _vqu_next

;------------------------------------------------------------------------------
; 16x16 sprite transfer (2 DMA transfers of 64 bytes each)
;------------------------------------------------------------------------------
_vqu_16x16:
    rep #$20
    .ACCU 16
    lda.l oamQueueEntry+3,x         ; VRAM destination address
    sta.l $2116
    lda.l oamQueueEntry,x           ; Source address
    sta.l $4372
    lda #$0040                      ; 64 bytes
    sta.l $4375
    sep #$20
    .ACCU 8
    lda #$80
    sta.l $420b                     ; row 1

    ; Row 2: VRAM offset |= $100, source + $200
    rep #$20
    .ACCU 16
    lda.l oamQueueEntry+3,x
    ora #$100
    sta.l $2116
    lda.l oamQueueEntry,x
    clc
    adc #$200
    sta.l $4372
    lda #$0040
    sta.l $4375
    sep #$20
    .ACCU 8
    lda #$80
    sta.l $420b

    jmp _vqu_next

;------------------------------------------------------------------------------
; 8x8 sprite transfer (1 DMA transfer of 32 bytes)
;------------------------------------------------------------------------------
_vqu_8x8:
    rep #$20
    .ACCU 16
    lda.l oamQueueEntry+3,x         ; VRAM destination
    sta.l $2116
    lda.l oamQueueEntry,x           ; Source address
    sta.l $4372
    lda #$0020                      ; 32 bytes
    sta.l $4375
    sep #$20
    .ACCU 8
    lda #$80
    sta.l $420b

_vqu_next:
    dex
    bmi _vqu_check_overflow
    beq _vqu_check_overflow
    jmp _vqu_loop

_vqu_check_overflow:
    ; If we hit the max, move remaining entries to front of queue
    rep #$20
    .ACCU 16
    lda.w oamqueuenumber
    beq _vqu_done
    ldy #$0000
    ldx #MAXSPRTRF
_vqu_shift:
    pha
    lda.l oamQueueEntry,x
    sta.w oamQueueEntry,y
    iny
    inx
    pla
    dea
    bmi _vqu_done
    jmp _vqu_shift

_vqu_done:
    ply
    plx
    plb
    plp
    rtl

.ENDS

;==============================================================================
; DYN_PACK_TAIL large - the part of oamDynamic{8,16}Draw after the tile number
;==============================================================================
; In: A 16-bit, X = oamnumberperframe, Y = oambuffer offset, DB = $7E, the
; tile number already stored. Packs X (low byte) and Y into the OAM entry,
; copies the attribute, and writes the entry's two bits of the high table
; (X bit 8, size) in ONE read-modify-write: oammask clears both, then sets
; the X bit if the carry says so, and the size bit if `large` is 1.
; Until 2026-11-01 this was a php/plp round trip around an index computation,
; and the size bit a second read-modify-write through other tables.
; The carry (X bit 8, from the ror) survives every instruction in between.
;==============================================================================
.MACRO DYN_PACK_TAIL ARGS large
    lda.w oambuffer+OAM_OAMX,y
    xba
    sep #$20
    .ACCU 8
    ror a                           ; carry = X bit 8
    lda.w oambuffer+OAM_OAMY,y
    xba
    rep #$20
    .ACCU 16
    sta.w oamMemory,x
    sep #$20
    .ACCU 8
    lda.w oambuffer+OAM_ATTRIBUTE,y
    sta.w oamMemory+3,x
    rep #$20
    .ACCU 16
    lda.l oammask+2,x               ; high table byte index (byte 3 is masked off)
    and.w #$00FF
    tay
    sep #$20
    .ACCU 8
    lda.w oamMemory+512,y
    and.l oammask+1,x               ; clear this sprite's X bit and size bit
    bcc @nox\@
    ora.l oammask,x                 ; X bit 8 set
@nox\@:
.IF \1 == 1
    ora.l oammask+3,x               ; size = large
.ENDIF
    sta.w oamMemory+512,y
    rep #$20
    .ACCU 16
.ENDM


;==============================================================================
; oamDynamic32Draw
;==============================================================================
; void oamDynamic32Draw(u16 id)
;
; Draw a 32x32 sprite using the dynamic engine.
; Queues graphics for VRAM upload if oamrefresh is set.
;
; Stack (after PHP/PHB/PHX/PHY = 8 bytes):
;   10,s = id (index into oambuffer)
;==============================================================================

.SECTION ".sprite_dynamic_32" SUPERFREE

oamDynamic32Draw:
    php
    phb
    phx
    phy

    rep #$20
    .ACCU 16
    lda 10,s                        ; id
    asl a                           ; id * 16 (oambuffer entry size)
    asl a
    asl a
    asl a
    tay                             ; Y = oambuffer offset

    sep #$20
    .ACCU 8
    lda #$7e
    pha
    plb

    ; Check if graphics need refresh
    sep #$20
    .ACCU 8
    lda.w oambuffer+OAM_REFRESH,y
    beq _o32d_no_refresh
    ; Queue full (128 entries, 768 bytes)? Leave the refresh flag set and try
    ; again next frame. Until 2026-10-03 the index advanced without a bound
    ; and the 129th entry overwrote .dynamic_sprite_state, index included.
    rep #$20
    .ACCU 16
    lda.l oamqueuenumber
    cmp #768
    sep #$20
    .ACCU 8
    bcs _o32d_no_refresh
    lda #$00
    sta.w oambuffer+OAM_REFRESH,y     ; Clear refresh flag

    ; Queue graphics for VRAM upload
    rep #$20
    .ACCU 16
    lda.w oambuffer+OAM_FRAMEID,y     ; Frame index
    asl a                           ; * 2 for word lookup
    tax
    lda.l lkup32oamS,x              ; Get source offset in sprite sheet
    clc
    adc.w oambuffer+OAM_GRAPHICS,y  ; Add graphics base address
    sta.w sprit_val2                ; Save computed source address

    ; Add to queue
    lda.l oamqueuenumber
    tax                             ; X = queue position
    clc
    adc #$0006
    sta.l oamqueuenumber            ; Advance queue

    ; Get VRAM destination for this sprite slot
    phx
    lda.w oamnumberspr0
    asl a
    tax
    lda.l lkup32idB,x               ; VRAM block address
    plx
    clc
    adc.w spr0addrgfx                 ; Add base VRAM address
    sta.l oamQueueEntry+3,x         ; Store VRAM dest in queue

    lda.w sprit_val2                  ; Source address
    sta.l oamQueueEntry,x

    sep #$20
    .ACCU 8
    lda.w oambuffer+OAM_GRAPHICS+2,y  ; Source bank
    sta.l oamQueueEntry+2,x
    lda #OBJ_SPRITE32
    sta.l oamQueueEntry+5,x         ; Sprite type marker

_o32d_no_refresh:
    ; Update OAM buffer with sprite position/attributes
    rep #$20
    .ACCU 16
    lda.w oamnumberspr0
    asl a
    tax
    lda.l lkup32idT,x               ; Tile number for 32x32 sprite
    ldx.w oamnumberperframe         ; Current OAM slot (x4)
    sta.w oamMemory+2,x             ; Store tile in OAM
    ; Position, attribute and high table bits: the same tail as the 16 and 8
    ; (the Y is stored as given, like oamSet: OAM y = 0 is the first visible
    ; line); the size is large.
    DYN_PACK_TAIL 1
    inc.w oamnumberspr0             ; Next 32x32 slot
    txa
    clc
    adc.w #$0004                    ; Next OAM entry
    sta.w oamnumberperframe

    ply
    plx
    plb
    plp
    rtl

.ENDS

;==============================================================================
; oamDynamic16Draw
;==============================================================================
; void oamDynamic16Draw(u16 id)
;
; Draw a 16x16 sprite using the dynamic engine.
;==============================================================================

.SECTION ".sprite_dynamic_16" SUPERFREE

oamDynamic16Draw:
    php
    phb
    phx
    phy

    rep #$20
    .ACCU 16
    lda 10,s                        ; id
    asl a
    asl a
    asl a
    asl a
    tay                             ; Y = oambuffer offset

    sep #$20
    .ACCU 8
    lda #$7e
    pha
    plb

    ; Check if graphics need refresh
    lda.w oambuffer+OAM_REFRESH,y
    beq _o16d_no_refresh
    ; Queue full (128 entries, 768 bytes)? Leave the refresh flag set and try
    ; again next frame. Until 2026-10-03 the index advanced without a bound
    ; and the 129th entry overwrote .dynamic_sprite_state, index included.
    rep #$20
    .ACCU 16
    lda.l oamqueuenumber
    cmp #768
    sep #$20
    .ACCU 8
    bcs _o16d_no_refresh
    lda #$00
    sta.w oambuffer+OAM_REFRESH,y

    ; Queue graphics for VRAM upload
    rep #$20
    .ACCU 16
    lda.w oambuffer+OAM_FRAMEID,y
    asl a
    tax
    lda.l lkup16oamS,x              ; Source offset
    clc
    adc.w oambuffer+OAM_GRAPHICS,y
    sta.w sprit_val2

    lda.l oamqueuenumber
    tax
    clc
    adc #$0006
    sta.l oamqueuenumber

    ; Determine if using large or small sprite VRAM area
    phx
    lda.w spr16addrgfx
    cmp.w spr0addrgfx
    beq _o16d_large_slot
    ; Small sprites
    lda.w oamnumberspr1
    jmp _o16d_get_block
_o16d_large_slot:
    lda.w oamnumberspr0
_o16d_get_block:
    asl a
    tax
    lda.l lkup16idB,x
    plx
    clc
    adc.w spr16addrgfx
    sta.l oamQueueEntry+3,x

    lda.w sprit_val2
    sta.l oamQueueEntry,x
    sep #$20
    .ACCU 8
    lda.w oambuffer+OAM_GRAPHICS+2,y
    sta.l oamQueueEntry+2,x
    lda #OBJ_SPRITE16
    sta.l oamQueueEntry+5,x

_o16d_no_refresh:
    rep #$20
    .ACCU 16
    lda.w spr16addrgfx
    cmp.w spr0addrgfx
    beq _o16d_large
    ; Small sprites: tile from lkup16idT
    lda.w oamnumberspr1
    asl a
    tax
    lda.l lkup16idT,x
    ldx.w oamnumberperframe
    sta.w oamMemory+2,x
    DYN_PACK_TAIL 0
    inc.w oamnumberspr1
    bra _o16d_inc_frame
_o16d_large:
    ; Large sprites: tile from lkup16idT0
    lda.w oamnumberspr0
    asl a
    tax
    lda.l lkup16idT0,x
    ldx.w oamnumberperframe
    sta.w oamMemory+2,x
    DYN_PACK_TAIL 1
    inc.w oamnumberspr0
_o16d_inc_frame:
    txa
    clc
    adc.w #$0004
    sta.w oamnumberperframe

    ply
    plx
    plb
    plp
    rtl

.ENDS

;==============================================================================
; oamDynamic8Draw
;==============================================================================
; void oamDynamic8Draw(u16 id)
;
; Draw an 8x8 sprite using the dynamic engine.
;==============================================================================

.SECTION ".sprite_dynamic_8" SUPERFREE

oamDynamic8Draw:
    php
    phb
    phx
    phy

    rep #$20
    .ACCU 16
    lda 10,s
    asl a
    asl a
    asl a
    asl a
    tay

    sep #$20
    .ACCU 8
    lda #$7e
    pha
    plb

    ; Check refresh
    lda.w oambuffer+OAM_REFRESH,y
    beq _o8d_no_refresh
    ; Queue full (128 entries, 768 bytes)? Leave the refresh flag set and try
    ; again next frame. Until 2026-10-03 the index advanced without a bound
    ; and the 129th entry overwrote .dynamic_sprite_state, index included.
    rep #$20
    .ACCU 16
    lda.l oamqueuenumber
    cmp #768
    sep #$20
    .ACCU 8
    bcs _o8d_no_refresh
    lda #$00
    sta.w oambuffer+OAM_REFRESH,y

    rep #$20
    .ACCU 16
    lda.w oambuffer+OAM_FRAMEID,y
    asl a
    tax
    lda.l lkup8oamS,x
    clc
    adc.w oambuffer+OAM_GRAPHICS,y
    sta.w sprit_val2

    lda.l oamqueuenumber
    tax
    clc
    adc #$0006
    sta.l oamqueuenumber

    phx
    lda.w oamnumberspr1
    asl a
    tax
    lda.l lkup8idB,x
    plx
    clc
    adc.w spr1addrgfx
    sta.l oamQueueEntry+3,x

    lda.w sprit_val2
    sta.l oamQueueEntry,x
    sep #$20
    .ACCU 8
    lda.w oambuffer+OAM_GRAPHICS+2,y
    sta.l oamQueueEntry+2,x
    lda #OBJ_SPRITE8
    sta.l oamQueueEntry+5,x

_o8d_no_refresh:
    rep #$20
    .ACCU 16
    lda.w oamnumberspr1
    asl a
    tax
    lda.l lkup8idT,x
    ldx.w oamnumberperframe
    sta.w oamMemory+2,x
    DYN_PACK_TAIL 0
    inc.w oamnumberspr1
    txa
    clc
    adc.w #$0004
    sta.w oamnumberperframe

    ply
    plx
    plb
    plp
    rtl

.ENDS

