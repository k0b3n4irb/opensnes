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
;==============================================================================

.ifdef HIROM
.include "lib_memmap_hirom.inc"
.else
.include "lib_memmap.inc"
.endif

;------------------------------------------------------------------------------
; Constants
;------------------------------------------------------------------------------

.EQU OBJ_SPRITE32           1           ; 32x32 sprite identifier
.EQU OBJ_SPRITE16           2           ; 16x16 sprite identifier
.EQU OBJ_SPRITE8            4           ; 8x8 sprite identifier

.EQU OBJ_SIZE16_L32         $60         ; (3<<5) 16x16 small, 32x32 large
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

;==============================================================================
; Lookup Tables for OAM High Table Manipulation
;==============================================================================

.SECTION ".lut_oam_masks" SUPERFREE

;------------------------------------------------------------------------------
; oammask - 4 bytes per sprite entry (indexed by id*4)
;   Byte 0: X high bit set mask
;   Byte 1: X high bit clear mask
;   Byte 2: High table offset (512 + id/4)
;   Byte 3: Padding
;------------------------------------------------------------------------------
oammask:
    .byte $01,$fe,$00,$00, $04,$fb,$00,$00, $10,$ef,$00,$00, $40,$bf,$00,$00
    .byte $01,$fe,$01,$00, $04,$fb,$01,$00, $10,$ef,$01,$00, $40,$bf,$01,$00
    .byte $01,$fe,$02,$00, $04,$fb,$02,$00, $10,$ef,$02,$00, $40,$bf,$02,$00
    .byte $01,$fe,$03,$00, $04,$fb,$03,$00, $10,$ef,$03,$00, $40,$bf,$03,$00
    .byte $01,$fe,$04,$00, $04,$fb,$04,$00, $10,$ef,$04,$00, $40,$bf,$04,$00
    .byte $01,$fe,$05,$00, $04,$fb,$05,$00, $10,$ef,$05,$00, $40,$bf,$05,$00
    .byte $01,$fe,$06,$00, $04,$fb,$06,$00, $10,$ef,$06,$00, $40,$bf,$06,$00
    .byte $01,$fe,$07,$00, $04,$fb,$07,$00, $10,$ef,$07,$00, $40,$bf,$07,$00
    .byte $01,$fe,$08,$00, $04,$fb,$08,$00, $10,$ef,$08,$00, $40,$bf,$08,$00
    .byte $01,$fe,$09,$00, $04,$fb,$09,$00, $10,$ef,$09,$00, $40,$bf,$09,$00
    .byte $01,$fe,$0a,$00, $04,$fb,$0a,$00, $10,$ef,$0a,$00, $40,$bf,$0a,$00
    .byte $01,$fe,$0b,$00, $04,$fb,$0b,$00, $10,$ef,$0b,$00, $40,$bf,$0b,$00
    .byte $01,$fe,$0c,$00, $04,$fb,$0c,$00, $10,$ef,$0c,$00, $40,$bf,$0c,$00
    .byte $01,$fe,$0d,$00, $04,$fb,$0d,$00, $10,$ef,$0d,$00, $40,$bf,$0d,$00
    .byte $01,$fe,$0e,$00, $04,$fb,$0e,$00, $10,$ef,$0e,$00, $40,$bf,$0e,$00
    .byte $01,$fe,$0f,$00, $04,$fb,$0f,$00, $10,$ef,$0f,$00, $40,$bf,$0f,$00
    .byte $01,$fe,$10,$00, $04,$fb,$10,$00, $10,$ef,$10,$00, $40,$bf,$10,$00
    .byte $01,$fe,$11,$00, $04,$fb,$11,$00, $10,$ef,$11,$00, $40,$bf,$11,$00
    .byte $01,$fe,$12,$00, $04,$fb,$12,$00, $10,$ef,$12,$00, $40,$bf,$12,$00
    .byte $01,$fe,$13,$00, $04,$fb,$13,$00, $10,$ef,$13,$00, $40,$bf,$13,$00
    .byte $01,$fe,$14,$00, $04,$fb,$14,$00, $10,$ef,$14,$00, $40,$bf,$14,$00
    .byte $01,$fe,$15,$00, $04,$fb,$15,$00, $10,$ef,$15,$00, $40,$bf,$15,$00
    .byte $01,$fe,$16,$00, $04,$fb,$16,$00, $10,$ef,$16,$00, $40,$bf,$16,$00
    .byte $01,$fe,$17,$00, $04,$fb,$17,$00, $10,$ef,$17,$00, $40,$bf,$17,$00
    .byte $01,$fe,$18,$00, $04,$fb,$18,$00, $10,$ef,$18,$00, $40,$bf,$18,$00
    .byte $01,$fe,$19,$00, $04,$fb,$19,$00, $10,$ef,$19,$00, $40,$bf,$19,$00
    .byte $01,$fe,$1a,$00, $04,$fb,$1a,$00, $10,$ef,$1a,$00, $40,$bf,$1a,$00
    .byte $01,$fe,$1b,$00, $04,$fb,$1b,$00, $10,$ef,$1b,$00, $40,$bf,$1b,$00
    .byte $01,$fe,$1c,$00, $04,$fb,$1c,$00, $10,$ef,$1c,$00, $40,$bf,$1c,$00
    .byte $01,$fe,$1d,$00, $04,$fb,$1d,$00, $10,$ef,$1d,$00, $40,$bf,$1d,$00
    .byte $01,$fe,$1e,$00, $04,$fb,$1e,$00, $10,$ef,$1e,$00, $40,$bf,$1e,$00
    .byte $01,$fe,$1f,$00, $04,$fb,$1f,$00, $10,$ef,$1f,$00, $40,$bf,$1f,$00

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
    lda #$7e
    pha
    plb

    ; Clear VRAM upload queue (768 bytes)
    rep #$10                        ; 16-bit X/Y for larger loop count
    ldx #$0000
    sep #$20                        ; 8-bit A for byte stores
    lda #$00
-   sta.w oamQueueEntry,x
    inx
    cpx #OBJ_QUEUELIST_SIZE*6
    bne -

    rep #$20
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

    ; Set 16x16 sprite VRAM address based on size configuration
    lda.w spr1addrgfx                 ; Default: small sprites use spr1 address
    sta.w spr16addrgfx
    sep #$20
    lda 10,s                        ; oamsize (arg5)
    cmp #OBJ_SIZE16_L32
    beq +
    rep #$20
    lda.w spr0addrgfx                 ; If not 16/32 mode, use spr0 address
    sta.w spr16addrgfx

+:  ; Initialize OAM (clear all sprites) - must do BEFORE setting OBJSEL
    ; because OpenSNES's oamInit resets OBJSEL
    rep #$20
    jsl oamInit

    ; Configure OBJSEL register AFTER oamInit (OpenSNES's oamInit overwrites it)
    sep #$20
    lda 10,s                        ; oamsize (arg5)
    sta.l $2101                     ; REG_OBJSEL

    ply
    plx
    plb
    plp
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
    lda #$7e
    pha
    plb

    rep #$20
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
    lda.l oamHideshift,x
    ora.w oamMemory,y
    sta.w oamMemory,y

    ; Set X position to 1 (with high bit = off-screen)
    rep #$20
    plx
    txa
    tay
    sep #$20
    lda #$01
    sta.w oamMemory,y
    iny
    lda #$F0                        ; Y = 240 (hidden)
    sta.w oamMemory,y

    rep #$20
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
    lda #$01
    sta.l oam_update_flag
    rep #$20

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
    stz.w oamqueuenumber            ; Assume we'll process everything
    txa                             ; A = bytes queued
    cmp #MAXSPRTRF
    bcc _vqu_process                ; Less than max, process all
    ldx #MAXSPRTRF                  ; Limit to max
    sec
    sbc #MAXSPRTRF
    sta.l oamqueuenumber            ; Save remainder for next frame

_vqu_process:
    ; Set up DMA channels for word transfer to VRAM
    lda #$1801
    sta.l $4310                     ; Channel 1: word increment to $2118
    sta.l $4320                     ; Channel 2
    sta.l $4330                     ; Channel 3
    sta.l $4340                     ; Channel 4

    dex                             ; Start at last entry (will decrement more in loop)

_vqu_loop:
    dex                             ; Move to next entry (6 bytes back)
    dex
    dex
    dex
    dex

    ; Check sprite size type
    sep #$20
    lda.l oamQueueEntry+5,x         ; Get sprite type
    cmp #OBJ_SPRITE8
    bne +
    jmp _vqu_8x8
+:  cmp #OBJ_SPRITE16
    bne _vqu_32x32
    jmp _vqu_16x16

;------------------------------------------------------------------------------
; 32x32 sprite transfer (4 DMA transfers of 128 bytes each)
;------------------------------------------------------------------------------
_vqu_32x32:
    rep #$20
    lda.l oamQueueEntry+3,x         ; VRAM destination address
    sta.l $2116

    ; Set up 4 source addresses (128 bytes apart in VRAM layout)
    lda.l oamQueueEntry,x           ; Source address (low 16 bits)
    sta.l $4312                     ; Channel 1 source
    clc
    adc #$200
    sta.l $4322                     ; Channel 2 source
    clc
    adc #$200
    sta.l $4332                     ; Channel 3 source
    clc
    adc #$200
    sta.l $4342                     ; Channel 4 source

    ; Set transfer size (128 bytes per row)
    lda #$0080
    sta.l $4315
    sta.l $4325
    sta.l $4335
    sta.l $4345

    ; Set source bank for all channels
    sep #$20
    lda.l oamQueueEntry+2,x         ; Source bank
    sta.l $4314
    sta.l $4324
    sta.l $4334
    sta.l $4344

    ; Transfer row 1
    lda #$02
    sta.l $420b

    ; Transfer row 2 (VRAM offset += $100)
    rep #$20
    lda #$100
    clc
    adc.l oamQueueEntry+3,x
    sta.l $2116
    sep #$20
    lda #$04
    sta.l $420b

    ; Transfer row 3 (VRAM offset += $200)
    rep #$20
    lda #$200
    clc
    adc.l oamQueueEntry+3,x
    sta.l $2116
    sep #$20
    lda #$08
    sta.l $420b

    ; Transfer row 4 (VRAM offset += $300)
    rep #$20
    lda #$300
    clc
    adc.l oamQueueEntry+3,x
    sta.l $2116
    sep #$20
    lda #$10
    sta.l $420b

    jmp _vqu_next

;------------------------------------------------------------------------------
; 16x16 sprite transfer (2 DMA transfers of 64 bytes each)
;------------------------------------------------------------------------------
_vqu_16x16:
    rep #$20
    lda.l oamQueueEntry+3,x         ; VRAM destination address
    sta.l $2116

    ; Set up 2 source addresses
    lda.l oamQueueEntry,x           ; Source address
    sta.l $4322                     ; Channel 2 source
    clc
    adc #$200
    sta.l $4332                     ; Channel 3 source

    ; Set transfer size (64 bytes per row)
    lda #$0040
    sta.l $4325
    sta.l $4335

    ; Set source bank
    sep #$20
    lda.l oamQueueEntry+2,x
    sta.l $4324
    sta.l $4334

    ; Transfer row 1
    lda #$04
    sta.l $420b

    ; Transfer row 2 (VRAM offset += $100)
    rep #$20
    lda.l oamQueueEntry+3,x
    ora #$100
    sta.l $2116
    sep #$20
    lda #$08
    sta.l $420b

    jmp _vqu_next

;------------------------------------------------------------------------------
; 8x8 sprite transfer (1 DMA transfer of 32 bytes)
;------------------------------------------------------------------------------
_vqu_8x8:
    rep #$20
    lda.l oamQueueEntry+3,x         ; VRAM destination
    sta.l $2116

    lda.l oamQueueEntry,x           ; Source address
    sta.l $4322

    lda #$0020                      ; 32 bytes
    sta.l $4325

    sep #$20
    lda.l oamQueueEntry+2,x
    sta.l $4324

    lda #$04
    sta.l $420b

_vqu_next:
    dex
    bmi _vqu_check_overflow
    beq _vqu_check_overflow
    jmp _vqu_loop

_vqu_check_overflow:
    ; If we hit the max, move remaining entries to front of queue
    rep #$20
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
    lda 10,s                        ; id
    asl a                           ; id * 16 (oambuffer entry size)
    asl a
    asl a
    asl a
    tay                             ; Y = oambuffer offset

    sep #$20
    lda #$7e
    pha
    plb

    ; Check if graphics need refresh
    sep #$20
    lda.w oambuffer+OAM_REFRESH,y
    beq _o32d_no_refresh
    lda #$00
    sta.w oambuffer+OAM_REFRESH,y     ; Clear refresh flag

    ; Queue graphics for VRAM upload
    rep #$20
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
    lda.w oambuffer+OAM_GRAPHICS+2,y  ; Source bank
    sta.l oamQueueEntry+2,x
    lda #OBJ_SPRITE32
    sta.l oamQueueEntry+5,x         ; Sprite type marker

_o32d_no_refresh:
    ; Update OAM buffer with sprite position/attributes
    rep #$20
    ldx.w oamnumberperframe           ; Current OAM slot (×4)

    ; Get tile ID from lookup table
    phx
    lda.w oamnumberspr0
    asl a
    tax
    lda.l lkup32idT,x               ; Tile number for 32x32 sprite
    plx
    sta.w oamMemory+2,x             ; Store tile in OAM

    ; Pack X and Y coordinates
    lda.w oambuffer+OAM_OAMX,y        ; X position
    xba                             ; Save X high byte
    sep #$20
    ror a                           ; X bit 8 into carry

    lda.w oambuffer+OAM_OAMY,y        ; Y position
    xba                             ; Swap: now A = Y:X_high_bit_in_carry
    rep #$20
    sta.w oamMemory,x               ; Store X(lo) and Y

    ; Handle X high bit in high table
    ; NOTE: Carry flag contains X bit 8 from the ror above
    phy                             ; Save oambuffer offset
    php                             ; Save processor status including carry
    rep #$20
    txa                             ; A = oamnumberperframe (oam index × 4)
    lsr a                           ; /2
    lsr a                           ; /4
    lsr a                           ; /8
    lsr a                           ; /16 = high table byte index
    clc
    adc.w #512                      ; Add high table base address
    tay                             ; Y = high table address (512+)
    plp                             ; Restore carry flag from ror
    sep #$20                        ; Ensure 8-bit A for mask operations

    bcs _o32d_xhigh                 ; If carry set, X >= 256
    lda.l oammask+1,x               ; Clear X high bit
    and.w oamMemory,y
    sta.w oamMemory,y
    jmp _o32d_attr

_o32d_xhigh:
    lda.l oammask,x                 ; Set X high bit
    ora.w oamMemory,y
    sta.w oamMemory,y

_o32d_attr:
    ply
    lda.w oambuffer+OAM_ATTRIBUTE,y   ; Sprite attributes
    sta.w oamMemory+3,x

    ; Set sprite size to LARGE in high table
    rep #$20
    lda.w oamnumberperframe
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.w #512
    tay                             ; Y = high table offset

    lda.w oamnumberperframe
    lsr a
    lsr a
    and.w #$0003
    tax

    sep #$20
    lda.w oamMemory,y
    and.l oamSetExand,x             ; Clear size bit
    sta.w oamMemory,y
    lda.l oamSizeshift,x            ; Set size = large
    ora.w oamMemory,y
    sta.w oamMemory,y

    ; Update counters
    rep #$20
    inc.w oamnumberspr0             ; Next 32x32 slot
    lda.w oamnumberperframe
    clc
    adc #$0004                      ; Next OAM entry
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
    lda 10,s                        ; id
    asl a
    asl a
    asl a
    asl a
    tay                             ; Y = oambuffer offset

    sep #$20
    lda #$7e
    pha
    plb

    ; Check if graphics need refresh
    lda.w oambuffer+OAM_REFRESH,y
    beq _o16d_no_refresh
    lda #$00
    sta.w oambuffer+OAM_REFRESH,y

    ; Queue graphics for VRAM upload
    rep #$20
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
    lda.w oambuffer+OAM_GRAPHICS+2,y
    sta.l oamQueueEntry+2,x
    lda #OBJ_SPRITE16
    sta.l oamQueueEntry+5,x

_o16d_no_refresh:
    rep #$20
    ldx.w oamnumberperframe

    ; Get tile ID based on size configuration
    phx
    lda.w spr16addrgfx
    cmp.w spr0addrgfx
    beq _o16d_large_tile
    ; Small sprites use lkup16idT
    lda.w oamnumberspr1
    asl a
    tax
    lda.l lkup16idT,x
    jmp _o16d_store_tile
_o16d_large_tile:
    ; Large sprites use lkup16idT0
    lda.w oamnumberspr0
    asl a
    tax
    lda.l lkup16idT0,x
_o16d_store_tile:
    plx
    sta.w oamMemory+2,x

    ; Pack X and Y
    lda.w oambuffer+OAM_OAMX,y
    xba
    sep #$20
    ror a
    lda.w oambuffer+OAM_OAMY,y
    xba
    rep #$20
    sta.w oamMemory,x

    ; Handle X high bit in high table
    ; NOTE: Carry flag contains X bit 8 from the ror above
    phy                             ; Save oambuffer offset
    php                             ; Save processor status including carry
    rep #$20
    txa                             ; A = oamnumberperframe (oam index × 4)
    lsr a                           ; /2
    lsr a                           ; /4
    lsr a                           ; /8
    lsr a                           ; /16 = high table byte index
    clc
    adc.w #512                      ; Add high table base address
    tay                             ; Y = high table address (512+)
    plp                             ; Restore carry flag from ror
    sep #$20                        ; Ensure 8-bit A for mask operations

    bcs _o16d_xhigh
    lda.l oammask+1,x               ; Clear mask
    and.w oamMemory,y
    sta.w oamMemory,y
    jmp _o16d_attr

_o16d_xhigh:
    lda.l oammask,x                 ; Set mask
    ora.w oamMemory,y
    sta.w oamMemory,y

_o16d_attr:
    ply
    lda.w oambuffer+OAM_ATTRIBUTE,y
    sta.w oamMemory+3,x

    ; Set sprite size in high table
    rep #$20
    lda.w oamnumberperframe
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.w #512
    tay

    lda.w oamnumberperframe
    lsr a
    lsr a
    and.w #$0003
    tax

    sep #$20
    lda.w oamMemory,y
    and.l oamSetExand,x
    sta.w oamMemory,y

    ; Set size based on configuration (large if using spr0, small if spr1)
    rep #$20
    lda.w spr16addrgfx
    cmp.w spr1addrgfx
    beq _o16d_small_size
    ; Large size
    sep #$20
    lda.l oamSizeshift,x
    ora.w oamMemory,y
    sta.w oamMemory,y

_o16d_small_size:
    ; Update counters
    rep #$20
    lda.w spr16addrgfx
    cmp.w spr0addrgfx
    beq _o16d_inc_large
    inc.w oamnumberspr1
    bra _o16d_inc_frame
_o16d_inc_large:
    inc.w oamnumberspr0

_o16d_inc_frame:
    lda.w oamnumberperframe
    clc
    adc #$0004
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
    lda 10,s
    asl a
    asl a
    asl a
    asl a
    tay

    sep #$20
    lda #$7e
    pha
    plb

    ; Check refresh
    lda.w oambuffer+OAM_REFRESH,y
    beq _o8d_no_refresh
    lda #$00
    sta.w oambuffer+OAM_REFRESH,y

    rep #$20
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
    lda.w oambuffer+OAM_GRAPHICS+2,y
    sta.l oamQueueEntry+2,x
    lda #OBJ_SPRITE8
    sta.l oamQueueEntry+5,x

_o8d_no_refresh:
    rep #$20
    ldx.w oamnumberperframe

    ; Get tile ID
    phx
    lda.w oamnumberspr1
    asl a
    tax
    lda.l lkup8idT,x
    plx
    sta.w oamMemory+2,x

    ; Pack X and Y
    lda.w oambuffer+OAM_OAMX,y
    xba
    sep #$20
    ror a
    lda.w oambuffer+OAM_OAMY,y
    xba
    rep #$20
    sta.w oamMemory,x

    ; Handle X high bit in high table
    ; NOTE: Carry flag contains X bit 8 from the ror above
    phy                             ; Save oambuffer offset
    php                             ; Save processor status including carry
    rep #$20
    txa                             ; A = oamnumberperframe (oam index × 4)
    lsr a                           ; /2
    lsr a                           ; /4
    lsr a                           ; /8
    lsr a                           ; /16 = high table byte index
    clc
    adc.w #512                      ; Add high table base address
    tay                             ; Y = high table address (512+)
    plp                             ; Restore carry flag from ror
    sep #$20                        ; Ensure 8-bit A for mask operations

    bcs _o8d_xhigh
    lda.l oammask+1,x               ; Clear X high bit
    and.w oamMemory,y
    sta.w oamMemory,y
    jmp _o8d_attr

_o8d_xhigh:
    lda.l oammask,x                 ; Set X high bit
    ora.w oamMemory,y
    sta.w oamMemory,y

_o8d_attr:
    ply
    lda.w oambuffer+OAM_ATTRIBUTE,y
    sta.w oamMemory+3,x

    ; Set sprite size = small in high table
    rep #$20
    lda.w oamnumberperframe
    lsr a
    lsr a
    lsr a
    lsr a
    clc
    adc.w #512
    tay

    lda.w oamnumberperframe
    lsr a
    lsr a
    and.w #$0003
    tax

    sep #$20
    lda.w oamMemory,y
    and.l oamSetExand,x             ; Clear size bit (small)
    sta.w oamMemory,y

    ; Update counters
    rep #$20
    inc.w oamnumberspr1
    lda.w oamnumberperframe
    clc
    adc #$0004
    sta.w oamnumberperframe

    ply
    plx
    plb
    plp
    rtl

.ENDS
