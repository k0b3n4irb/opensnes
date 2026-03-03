;==============================================================================
; OpenSNES Object Engine
;==============================================================================
;
; Game object engine with physics, collision detection, and type-based dispatch.
; Uses a workspace pattern for C callback access (objWorkspace in Bank $00).
;
; Based on: PVSnesLib object engine by Alekmaul
; Slope management: Nub1604
; License: zlib (compatible with MIT)
;
;==============================================================================

.ifdef HIROM
.include "lib_memmap_hirom.inc"
.else
.include "lib_memmap.inc"
.endif

;------------------------------------------------------------------------------
; Constants
;------------------------------------------------------------------------------

.DEFINE OB_NULL             $FFFF
.DEFINE OB_ID_NULL          $0000

.DEFINE OB_MAX              80
.DEFINE OB_TYPE_MAX         64
.DEFINE OB_SIZE             64

.DEFINE OB_SCR_XLR_CHK      -64
.DEFINE OB_SCR_XRR_CHK      320
.DEFINE OB_SCR_YLR_CHK      -64
.DEFINE OB_SCR_YRR_CHK      288

.DEFINE OB_SCR_XLE_CHK      -32
.DEFINE OB_SCR_XRI_CHK      256
.DEFINE OB_SCR_YLE_CHK      -32
.DEFINE OB_SCR_YRI_CHK      224

.DEFINE T_SOLID             $FF00
.DEFINE T_LADDER            $0001
.DEFINE T_FIRES             $0002
.DEFINE T_SPIKE             $0004
.DEFINE T_PLATE             $0008
.DEFINE T_SLOPES            $0020

.DEFINE ACT_CLIMB           $2000
.DEFINE ACT_DIE             $4000
.DEFINE ACT_BURN            $8000

.DEFINE GRAVITY             41
.DEFINE MAX_Y_VELOCITY      (10*256)
.DEFINE FRICTION            $10
.DEFINE FRICTION1D          $0100

;------------------------------------------------------------------------------
; Object structure (64 bytes)
;------------------------------------------------------------------------------

.STRUCT t_objs
prev            DW
next            DW
type            DB
nID             DB
sprnum          DW
sprid3216       DW
sprblk3216      DW
sprflip         DB
sprpal          DB
sprframe        DW
xpos            DSB 3
ypos            DSB 3
xofs            DW
yofs            DW
width           DW
height          DW
xmin            DW
xmax            DW
xvel            DW
yvel            DW
tilestand       DW
tileabove       DW
tilesprop       DW
tilebprop       DW
action          DW
status          DB
tempo           DB
count           DB
dir             DB
parentID        DW
hitpoints       DB
sprrefresh      DB
onscreen        DB
objnotused      DSB 7
.ENDST

;------------------------------------------------------------------------------
; Compile-time struct layout assertions
; If these fail, the C header (object.h) and ASM struct are out of sync.
;------------------------------------------------------------------------------
.ASSERT _sizeof_t_objs == 64
.ASSERT t_objs.xpos == 16
.ASSERT t_objs.ypos == 19
.ASSERT t_objs.xvel == 34
.ASSERT t_objs.yvel == 36
.ASSERT t_objs.action == 46
.ASSERT t_objs.type == 4
.ASSERT t_objs.sprnum == 6
.ASSERT t_objs.width == 26
.ASSERT t_objs.height == 28
.ASSERT t_objs.status == 48
.ASSERT t_objs.onscreen == 56

;------------------------------------------------------------------------------
; oambuffer structure offsets (must match sprite_dynamic.asm)
;------------------------------------------------------------------------------
.EQU OAM_OAMREFRESH        7

;------------------------------------------------------------------------------
; RAM - Bank $00 (workspace + small vars, C-accessible)
;------------------------------------------------------------------------------

.RAMSECTION ".obj_bank00" BANK 0 SLOT 1

objWorkspace    INSTANCEOF t_objs           ; 64-byte workspace for C callbacks
objgetid        DW                          ; return value of objNew
objptr          DW                          ; current object offset
objtokill       DB                          ; set to 1 to kill current object

.ENDS

;------------------------------------------------------------------------------
; RAM - Bank $7E (bulk data, NOT C-accessible)
;------------------------------------------------------------------------------

.RAMSECTION ".obj_bank7e" BANK $7E SLOT 2

objbuffers      INSTANCEOF t_objs OB_MAX    ; object struct array (80 * 64 = 5120 bytes)
objactives      DSW OB_TYPE_MAX             ; active object list heads

objfctinit      DSB 4*OB_TYPE_MAX           ; init function pointers
objfctupd       DSB 4*OB_TYPE_MAX           ; update function pointers
objfctref       DSB 4*OB_TYPE_MAX           ; refresh function pointers

objtmpbuf       DSW (OB_MAX*5)+1            ; temporary buffer for object loading

objunused       DW                          ; head of unused object list
objnewid        DW                          ; new id for last object
objnextid       DB                          ; each new object gets unique nID

objfctcall      DSB 2                       ; low address for C function call
objfctcallh     DSB 2                       ; high address for C function call

objcidx         DW                          ; index of current object in loop

objgravity      DW                          ; gravity value
objfriction     DW                          ; friction value

objneedrefresh  DB                          ; 1 if global sprite refresh needed

objtmp1         DW                          ; temporary vars
objtmp2         DW
objtmp3         DW
objtmp4         DW

.ENDS

;==============================================================================
; Block Copy Safety Macros
;==============================================================================
.include "blockcopy.inc"

;==============================================================================
; Workspace Sync Macros
;==============================================================================
; These copy 64 bytes between objWorkspace (Bank $00) and objbuffers (Bank $7E).
; X must contain the byte offset into objbuffers on entry.
; Implemented as macros because SUPERFREE sections may be placed in different
; banks, and jsr can't cross bank boundaries. phb/plb preserves DBR since
; MVN changes it to the destination bank.
;==============================================================================

; SYNC_TO_WORKSPACE: objbuffers[X] → objWorkspace
; X = byte offset into objbuffers (preserved)
.MACRO SYNC_TO_WORKSPACE
    phb                         ; save DBR (MVN will change it)
    phx
    phy

    rep #$30
    txa
    clc
    adc #objbuffers.w
    tax                         ; X = source addr in Bank $7E

    ldy #objWorkspace.w         ; Y = dest addr in Bank $00

    lda #OB_SIZE - 1            ; A = byte count - 1
    mvn $7E, $00                ; BLOCK_COPY_7E_TO_00 (inline for computed X)

    ply
    plx
    plb                         ; restore DBR
.ENDM

; SYNC_FROM_WORKSPACE: objWorkspace → objbuffers[X]
; X = byte offset into objbuffers (preserved)
.MACRO SYNC_FROM_WORKSPACE
    phb                         ; save DBR (MVN will change it)
    phx
    phy

    rep #$30
    txa
    clc
    adc #objbuffers.w
    tay                         ; Y = dest addr in Bank $7E

    ldx #objWorkspace.w         ; X = source addr in Bank $00

    lda #OB_SIZE - 1            ; A = byte count - 1
    mvn $00, $7E                ; BLOCK_COPY_00_TO_7E (inline for computed Y)

    ply
    plx
    plb                         ; restore DBR
.ENDM

;==============================================================================
; CODE SECTION 0 - Core object functions
;==============================================================================

.SECTION ".objects0_text" SUPERFREE

.accu 16
.index 16
.16bit

;------------------------------------------------------------------------------
; void objInitEngine(void)
;------------------------------------------------------------------------------
objInitEngine:
    php
    phb
    phx
    phy

    sep #$20
    lda #$7e
    pha
    plb

    rep #$20
    ldx #0

_oieR0:
    stz objbuffers,x
    inx
    inx
    txa
    cmp #(OB_SIZE*(OB_MAX))
    bne _oieR0

    lda #$0000
    ldy #$0001

_oieR1:
    tax
    stz objbuffers.1.nID,x
    tya
    sta objbuffers.1.next,x
    txa
    cmp #(OB_SIZE*(OB_MAX-2))
    beq _oieR2
    iny
    clc
    adc #OB_SIZE
    bra _oieR1

_oieR2:
    tya
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    lda #OB_NULL
    sta objbuffers.1.next,x
    stz objunused

    ldx #(2*(OB_TYPE_MAX-1))
_oieR3:
    sta objactives,x
    dex
    dex
    bne _oieR3
    sta objactives,x

    stz.w objnewid
    stz.w objgetid

    sep #$20
    lda #$1
    sta objnextid

    rep #$20
    lda #GRAVITY
    sta objgravity
    lda #FRICTION
    sta objfriction

    ply
    plx
    plb
    plp

    rtl

;------------------------------------------------------------------------------
; void objInitGravity(u16 objgravity, u16 objfriction)
;------------------------------------------------------------------------------
objInitGravity:
    php
    phb

    sep #$20
    lda #$7e
    pha
    plb

    ; cproc L-to-R: friction (param 2) at SP+6, gravity (param 1) at SP+8
    rep #$20
    lda 8,s                                 ; gravity (param 1)
    sta objgravity
    lda 6,s                                 ; friction (param 2)
    sta objfriction

    plb
    plp

    rtl

;------------------------------------------------------------------------------
; void objInitFunctions(u8 objtype, void *initfct, void *updfct, void *reffct)
; Stack: 5 6-9 10-13 14-16
;------------------------------------------------------------------------------
objInitFunctions:
    php
    phb

    phx

    sep #$20
    lda #$7e
    pha
    plb

    ; cproc L-to-R: ref(p4) SP+8, upd(p3) SP+10, init(p2) SP+12, type(p1) SP+14
    ; cproc passes 16-bit function addresses only (no bank byte)
    lda 14,s                                ; type (param 1)
    rep #$20
    and #$00ff
    asl a
    asl a
    tax

    lda 12,s                                ; init function (param 2) — 16-bit addr
    sta objfctinit,x
    lda 10,s                                ; update function (param 3)
    sta objfctupd,x
    lda 8,s                                 ; refresh function (param 4)
    sta objfctref,x

    ; Set bank bytes to $00 (cproc LoROM: code in bank $00)
    sep #$20
    lda #$00
    sta objfctinit+2,x
    sta objfctupd+2,x
    sta objfctref+2,x

    plx
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; u16 objNew(u8 objtype, u16 x, u16 y)
; cproc L-to-R: y(p3) SP+10, x(p2) SP+12, type(p1) SP+14
;------------------------------------------------------------------------------
objNew:
    php
    phb

    phx
    phy

    sep #$20
    lda #$7e
    pha
    plb

    lda 14,s                                ; type (param 1)

    rep #$20
    and #$00ff
    asl a
    tay

    lda objunused
    cmp #OB_NULL
    beq _oiN0
    bra _oiN1

_oiN0:
    lda #OB_ID_NULL
    sta tcc__r0
    jmp _oiNEnd

_oiN1:
    sta objnewid
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    lda objbuffers.1.next,x
    sta objunused

    lda #OB_NULL
    sta objbuffers.1.prev,x
    lda objactives,y
    sta objbuffers.1.next,x

    phx
    cmp #OB_NULL
    beq _oiN2
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    lda objnewid
    sta objbuffers.1.prev,x

_oiN2:
    lda objnewid
    sta objactives,y
    plx

    sep #$20
    lda 14,s                                ; type (param 1)
    sta objbuffers.1.type,x
    lda objnextid
    sta objbuffers.1.nID,x
    inc a
    bne _oiN3
    inc a
_oiN3:
    sta objnextid

    rep #$20
    lda 12,s                                ; x (param 2)
    sta objbuffers.1.xpos+1,x
    lda 10,s                                ; y (param 3)
    sta objbuffers.1.ypos+1,x

    sep #$20
    lda objbuffers.1.nID,x

    rep #$20
    and #$00ff
    xba
    clc
    adc objnewid
    sta tcc__r0
    sta.l objgetid

    ; Copy new object to workspace for init callback access
    SYNC_TO_WORKSPACE

_oiNEnd:
    ply
    plx
    plb
    plp

    rtl

;------------------------------------------------------------------------------
; Long call to C function via stored pointer
;------------------------------------------------------------------------------
jslcallfct:
    sep #$20
    lda.l objfctcall + 2
    pha
    rep #$20
    lda.l objfctcall
    dec a
    pha
    rtl

;------------------------------------------------------------------------------
; void objGetPointer(u16 objhandle)
;------------------------------------------------------------------------------
objGetPointer:
    php
    phb

    phx
    phy

    sep #$20
    lda #$7e
    pha
    plb

    rep #$20
    lda 10,s                                ; handle (5+1+2+2)
    xba
    and #$00ff

    sta.l objptr
    lda 10,s
    and #$00ff
    tax
    inx
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tay
    sep #$20
    lda objbuffers.1.nID,y
    rep #$20
    and #$00ff
    clc
    cmp.l objptr
    beq _oigp1

    ldx #0
_oigp1:
    txa
    sta.l objptr

    ; If valid, copy to workspace
    beq _oigp2
    tya                                     ; Y = byte offset
    tax
    SYNC_TO_WORKSPACE

_oigp2:
    ply
    plx
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; void objKill(u16 objhandle)
;------------------------------------------------------------------------------
objKill:
    php
    phb

    phx
    phy

    sep #$20
    lda #$7e
    pha
    plb

    rep #$20
    lda 10,s                                ; handle (5+1+2+2)
    pha
    jsl objGetPointer
    pla

    lda.l objptr
    bne _oik1
    brl _oikend

_oik1:
    dec a
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    sta.l objptr

    lda objbuffers.1.prev,x

    cmp #OB_NULL
    bne _oik2

    sep #$20
    lda objbuffers.1.type,x
    rep #$20
    and #$00ff
    asl a
    tay
    lda objbuffers.1.next,x
    sta objactives,y

    cmp #OB_NULL
    beq _oik3

    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    lda #OB_NULL
    sta objbuffers.1.prev,x
    bra _oik3

_oik2:
    lda objbuffers.1.next,x
    tay
    lda objbuffers.1.prev,x
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    tya
    sta objbuffers.1.next,x

    cmp #OB_NULL
    beq _oik3

    ldx.w objptr
    lda objbuffers.1.prev,x
    tay
    lda objbuffers.1.next,x
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    tya
    sta objbuffers.1.prev,x

_oik3:
    ldx.w objptr
    lda objunused
    sta objbuffers.1.next,x
    lda 10,s
    and #$00ff
    sta objunused

    ldy #4
_oik4:
    stz objbuffers.1.type,x
    inx
    inx
    iny
    iny
    cpy #OB_SIZE
    bne _oik4

_oikend:
    ply
    plx
    plb
    plp

    rtl

;------------------------------------------------------------------------------
; void objKillAll(void)
;------------------------------------------------------------------------------
objKillAll:
    php
    phb

    phx
    phy

    sep #$20
    lda #$7e
    pha
    plb

    rep #$20
    ldy #$0000

_oikal1:
    lda objactives,y
_oikal2:
    sta objcidx
    cmp #OB_NULL
    beq _oikal3
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    lda objbuffers.1.next,x
    pha

    sep #$20
    lda objbuffers.1.nID,x
    rep #$20
    and #$00ff
    xba
    clc
    adc objcidx
    pha
    jsl objKill
    pla

    pla
    bra _oikal2

_oikal3:
    iny
    iny
    cpy #OB_TYPE_MAX*2
    bne _oikal1

    stz.w objnewid
    stz.w objgetid
    stz.w objunused

    sep #$20
    lda #$1
    sta objnextid

    ply
    plx
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; objOamRefreshAll - internal: mark all sprites for refresh
;------------------------------------------------------------------------------
objOamRefreshAll:
    php
    phx

    ldx #$0000
_oora:
    sep #$20
    lda #$1
    sta oambuffer + OAM_OAMREFRESH,x
    rep #$20
    txa
    clc
    adc #16                                 ; 16 bytes per oambuffer entry
    tax
    cmp #128*16
    bne _oora

    plx
    plp
    rts

;------------------------------------------------------------------------------
; void objUpdateAll(void)
;------------------------------------------------------------------------------
objUpdateAll:
    php
    phb

    phx
    phy

    sep #$20
    lda #$7e
    pha
    plb

    stz objneedrefresh

    rep #$20
    ldx #$0000

_oiual1:
    lda objactives,x
    phx
_oiual10:
    sta objcidx
    cmp #OB_NULL
    bne _oiual3
    brl _oiual2
_oiual3:
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax

    lda objbuffers.1.next,x
    pha

    lda objbuffers.1.xpos+1,x
    sec
    sbc.l x_pos

    cmp.w #OB_SCR_XRR_CHK
    bcc _oiual3y
    cmp.w #OB_SCR_XLR_CHK
    bcc _oiual3y1

_oiual3y:
    lda objbuffers.1.ypos+1,x
    sec
    sbc.l y_pos

    cmp.w #OB_SCR_YRR_CHK
    bcc _oiual32
    cmp.w #OB_SCR_YLR_CHK
    bcs _oiual32

_oiual3y1:
    jmp _oial4

_oiual32:
    lda objbuffers.1.xpos+1,x
    sec
    sbc.l x_pos
    cmp.w #OB_SCR_XRI_CHK
    bcc _oiual3sy
    cmp.w #OB_SCR_XLE_CHK
    bcc _oiual3sy1

_oiual3sy:
    lda objbuffers.1.ypos+1,x
    sec
    sbc.l y_pos

    cmp.w #OB_SCR_YRI_CHK
    bcc _oiuals32
    cmp.w #OB_SCR_YLE_CHK
    bcs _oiuals32

_oiual3sy1:
    sep #$20
    lda objbuffers.1.onscreen,x
    beq _oiual3sy11
    stz objbuffers.1.onscreen,x
    lda objneedrefresh
    bne _oiual3sy11
    lda #1
    sta objneedrefresh
    jsr objOamRefreshAll

_oiual3sy11:
    bra _oiual321

_oiuals32:
    sep #$20
    lda objbuffers.1.onscreen,x
    bne _oiual321
    lda #$1
    sta objbuffers.1.onscreen,x
    lda objneedrefresh
    bne _oiual321
    lda #1
    sta objneedrefresh
    jsr objOamRefreshAll
_oiual321:
    ; --- WORKSPACE PATTERN: copy object to workspace before C callback ---
    SYNC_TO_WORKSPACE

    lda objbuffers.1.type,x
    rep #$20
    and #$00ff
    asl a
    asl a
    tay

    lda objfctupd,y
    sta objfctcall
    lda objfctupd+2,y
    sta objfctcallh

    lda objcidx
    pha

    sep #$20
    stz.w objtokill
    jsl jslcallfct
    rep #$20
    pla

    ; --- WORKSPACE PATTERN: copy workspace back to object after callback ---
    lda objcidx
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    SYNC_FROM_WORKSPACE

    sep #$20
    lda.l objtokill
    beq _oial4

_oial41:
    rep #$20
    lda objcidx
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    sep #$20
    stz objbuffers.1.onscreen,x
    lda objbuffers.1.nID,x
    rep #$20
    and #$00ff
    xba
    clc
    adc objcidx
    pha
    jsl objKill
    pla

    sep #$20
    lda objneedrefresh
    bne _oial4
    lda #1
    sta objneedrefresh
    jsr objOamRefreshAll

_oial4:
    rep #$20
    pla
    brl _oiual10

_oiual2:
    plx
    inx
    inx
    cpx #OB_TYPE_MAX*2
    beq _oiuaend
    brl _oiual1

_oiuaend:
    ply
    plx
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; void objRefreshAll(void)
;------------------------------------------------------------------------------
objRefreshAll:
    php
    phb

    phx
    phy

    sep #$20
    lda #$7e
    pha
    plb

    stz objneedrefresh

    rep #$20
    ldx #$0000

_oiral1:
    lda objactives,x
    phx
_oiral10:
    sta objcidx
    cmp #OB_NULL
    bne _oiral3
    brl _oiral2
_oiral3:
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax

    lda objbuffers.1.next,x
    pha

    sep #$20
    lda objbuffers.1.onscreen,x
    bne _oiral32

    rep #$20
    bra _oiral31

_oiral32:
    ; --- WORKSPACE PATTERN: copy to workspace before refresh callback ---
    SYNC_TO_WORKSPACE

    lda objbuffers.1.type,x
    rep #$20
    and #$00ff
    asl a
    asl a
    tay

    lda objfctref,y
    sta objfctcall
    lda objfctref+2,y
    sta objfctcallh

    lda objcidx
    pha

    sep #$20
    jsl jslcallfct
    rep #$20
    pla

    ; --- WORKSPACE PATTERN: copy back after refresh callback ---
    lda objcidx
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    SYNC_FROM_WORKSPACE

_oiral31:
    pla
    brl _oiral10

_oiral2:
    plx
    inx
    inx
    cpx #OB_TYPE_MAX*2
    beq _oiraend
    brl _oiral1

_oiraend:
    ply
    plx
    plb
    plp
    rtl

.ENDS

;==============================================================================
; CODE SECTION 1 - objCollidMap
;==============================================================================

.SECTION ".objects1_text" SUPERFREE

.accu 16
.index 16
.16bit

;------------------------------------------------------------------------------
; void objCollidMap(u16 objhandle)
;
; Workspace sync: copies workspace → buffer before, buffer → workspace after.
;------------------------------------------------------------------------------
objCollidMap:
    php
    phb

    phx
    phy

    sep #$20
    lda #$7e
    pha
    plb

    rep #$20
    lda 10,s                                ; get index (5+1+2+2)

    ; --- Sync workspace → objbuffers before collision ---
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    SYNC_FROM_WORKSPACE

    stx objtmp2                             ; X preserved by SYNC macro (MVN clobbers A)

    lda objbuffers.1.yvel,x
    bpl _oicm1
    jmp _oicmtstyn
_oicm1:
    xba
    and #$00FF
    clc
    adc objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    clc
    adc objbuffers.1.height,x
    bpl +
    lda #0000

+   lsr a
    lsr a
    and #$FFFE
    tay

    lda objbuffers.1.xpos+1,x
    clc
    adc objbuffers.1.xofs,x
    pha

    and #$0007
    clc
    adc objbuffers.1.width,x
    dec a
    lsr a
    lsr a
    lsr a
    inc a
    sta objtmp1

    pla
    lsr a
    lsr a
    and #$FFFE
    clc
    adc.w mapadrrowlut, y
    tay

    clc
    adc maptile_L1d
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb
    and #$03FF
    asl a
    tax
    lda metatilesprop, x
    sta objtmp3
    bra _oicm21

_oicm2:
    tya
    clc
    adc maptile_L1d
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb
    and #$03FF
    asl a
    tax
    lda metatilesprop, x
    sta objtmp3

_oicm21:
    ldx objtmp2
    sta objbuffers.1.tilesprop,x
    lda objbuffers.1.tilesprop,x
    cmp #T_LADDER
    beq _oicm3
    cmp #T_PLATE
    beq _oicm3

    cmp #T_FIRES
    bne _oicm22
    lda #ACT_BURN
    sta objbuffers.1.action
    brl _oicmtstx

_oicm22:
    cmp #T_SPIKE
    bne _oicm23
    lda #ACT_DIE
    sta objbuffers.1.action
    brl _oicmtstx

_oicm23:
    and #$ff00
    beq _oicm4

_oicm3:
    lda objtmp3
    sta objbuffers.1.tilestand,x

    lda objbuffers.1.yvel+1,x
    and #$00ff
    clc
    adc objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    clc
    adc objbuffers.1.height,x
    inc a

    and #$fff8
    sec
    sbc objbuffers.1.yofs,x
    sec
    sbc objbuffers.1.height,x
    sta objbuffers.1.ypos+1,x
    stz objbuffers.1.yvel,x
    jmp _oicmtstx

_oicm4:
    rep #$20
    iny
    iny
    dec objtmp1
    beq _oicm5
    brl _oicm2

_oicm5:
    ldx objtmp2
    stz objbuffers.1.tilestand,x

    lda objbuffers.1.tilesprop,x
    beq _oicm61
    lda objbuffers.1.action,x
    and #ACT_CLIMB
    beq _oicm61
    brl _oicmtstx

_oicm61:
    lda objbuffers.1.yvel,x
    clc
    adc #GRAVITY
    cmp #MAX_Y_VELOCITY+1
    bmi _oicm6
    lda #MAX_Y_VELOCITY

_oicm6:
    sta objbuffers.1.yvel,x
    brl _oicmtstx

_oicmtstyn:
    stz objbuffers.1.tilestand,x

    xba
    ora #$FF00
    clc
    adc objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    bpl +

    lda #0000
+   lsr a
    lsr a
    and #$FFFE
    tay

    lda objbuffers.1.xpos+1,x
    clc
    adc objbuffers.1.xofs,x
    pha

    and #$0007
    clc
    adc objbuffers.1.width,x
    dec a
    lsr a
    lsr a
    lsr a
    inc a
    sta objtmp1

    pla
    lsr a
    lsr a
    and #$FFFE
    clc
    adc.w mapadrrowlut, y
    tay

    clc
    adc maptile_L1d
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb
    and #$03FF
    asl a
    tax
    lda metatilesprop, x
    bra _oicmtstyn2

_oicmtstyn1:
    tya
    clc
    adc maptile_L1d
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb
    and #$03FF
    asl a
    tax
    lda metatilesprop, x

_oicmtstyn2:
    ldx objtmp2
    sta objbuffers.1.tilesprop,x
    lda objbuffers.1.tilesprop,x
    cmp #T_LADDER
    beq _oicmtstyn4

    and #$ff00
    beq _oicmtstyn3

    lda objbuffers.1.yvel+1,x
    ora #$ff00
    clc
    adc objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    clc
    adc #8
    and #$fff8
    sec
    sbc objbuffers.1.yofs,x
    sta objbuffers.1.ypos+1,x
    stz objbuffers.1.yvel,x
    brl _oicmtstyn4

_oicmtstyn3:
    iny
    iny
    dec objtmp1
    bne _oicmtstyn1

_oicmtstyn4:
    ldx objtmp2
    lda objbuffers.1.yvel,x
    clc
    adc #GRAVITY
    cmp #MAX_Y_VELOCITY+1
    bmi _oicmtstyn5
    lda #MAX_Y_VELOCITY

_oicmtstyn5:
    sta objbuffers.1.yvel,x

_oicmtstx:
    ldx objtmp2
    lda objbuffers.1.xvel,x
    bne _oicmtstx1
    jmp _oicmend
_oicmtstx1:
    bpl _oicmtstx11
    jmp _oicmtstxn
_oicmtstx11:
    sec
    sbc objfriction
    bpl _oicmtstx12
    stz objbuffers.1.xvel,x
    jmp _oicmend

_oicmtstx12:
    sta objbuffers.1.xvel,x
    lda objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    bpl +
    lda #0000

+   pha
    and #$0007
    clc
    adc objbuffers.1.height,x
    dec a
    lsr a
    lsr a
    lsr a
    inc a
    sta objtmp1

    pla
    lsr a
    lsr a
    and #$fffe
    tay

    lda objbuffers.1.xvel+1,x
    and #$00ff
    clc
    adc objbuffers.1.xpos+1,x
    clc
    adc objbuffers.1.xofs,x
    clc
    adc objbuffers.1.width,x
    lsr a
    lsr a
    and #$fffe
    clc
    adc.w mapadrrowlut, y

_oicmtstx13:
    tay
    clc
    adc maptile_L1d
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb
    and #$03FF
    asl a
    tax
    lda metatilesprop, x

    ldx objtmp2
    sta objbuffers.1.tilebprop,x
    lda objbuffers.1.tilebprop,x
    beq _oicmtstx14
    and #$ff00
    beq _oicmtstx14

    lda objbuffers.1.xvel+1,x
    and #$00ff
    clc
    adc objbuffers.1.xpos+ 1,x
    clc
    adc objbuffers.1.xofs,x
    clc
    adc objbuffers.1.width,x
    inc a
    and #$fff8
    sec
    sbc objbuffers.1.xofs,x
    sec
    sbc objbuffers.1.width,x
    sta objbuffers.1.xpos+ 1,x
    stz objbuffers.1.xvel,x
    brl _oicmend

_oicmtstx14:
    tya
    clc
    adc.l maprowsize
    dec objtmp1
    bne _oicmtstx13
    brl _oicmend

_oicmtstxn:
    clc
    adc objfriction
    bmi _oicmtstxna
    stz objbuffers.1.xvel,x
    brl _oicmend

_oicmtstxna:
    sta objbuffers.1.xvel,x
    lda objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    bpl +
    lda #0000

+   pha
    and #$0007
    clc
    adc objbuffers.1.height,x
    dec a
    lsr a
    lsr a
    lsr a
    inc a
    sta objtmp1

    pla
    lsr a
    lsr a
    and #$fffe
    tay

    lda objbuffers.1.xvel+1,x
    ora #$ff00
    clc
    adc objbuffers.1.xpos+1,x
    clc
    adc objbuffers.1.xofs,x

    bpl _oicmtstxnb
    jmp _oicmend

_oicmtstxnb:
    lsr a
    lsr a
    and #$fffe
    clc
    adc.w mapadrrowlut, y

_oicmtstxnc:
    tay
    clc
    adc maptile_L1d
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb
    and #$03FF
    asl a
    tax
    lda metatilesprop, x

    ldx objtmp2
    sta objbuffers.1.tilebprop,x
    lda objbuffers.1.tilebprop,x
    beq _oicmtstxnd

    and #$ff00
    beq _oicmtstxnd

    lda objbuffers.1.xvel+1,x
    ora #$ff00
    clc
    adc objbuffers.1.xpos+ 1,x
    clc
    adc objbuffers.1.xofs,x
    clc
    adc #8
    and #$fff8
    sec
    sbc objbuffers.1.xofs,x
    sta objbuffers.1.xpos+ 1,x
    stz objbuffers.1.xvel,x

    brl _oicmend

_oicmtstxnd:
    tya
    clc
    adc.l maprowsize
    dec objtmp1
    bne _oicmtstxnc

_oicmend:
    ; --- Sync objbuffers → workspace after collision ---
    ldx objtmp2
    SYNC_TO_WORKSPACE

    ply
    plx
    plb
    plp
    rtl

.ENDS

;==============================================================================
; CODE SECTION 2 - objCollidMap1D (no gravity)
;==============================================================================

.SECTION ".objects2_text" SUPERFREE

.accu 16
.index 16
.16bit

;------------------------------------------------------------------------------
; void objCollidMap1D(u16 objhandle)
;------------------------------------------------------------------------------
objCollidMap1D:
    php
    phb

    phx
    phy

    sep #$20
    lda #$7e
    pha
    plb

    rep #$20
    lda 10,s                                ; get index (5+1+2+2)

    ; --- Sync workspace → objbuffers ---
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    SYNC_FROM_WORKSPACE

    stx objtmp2                             ; X preserved by SYNC macro (MVN clobbers A)

    lda objbuffers.1.yvel,x
    bpl _oicm1d1
    jmp _oicm1dtstyn
_oicm1d1:
    xba
    and #$00FF
    clc
    adc objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    clc
    adc objbuffers.1.height,x
    bpl +
    lda #0000

+   lsr a
    lsr a
    and #$FFFE
    tay

    lda objbuffers.1.xpos+1,x
    clc
    adc objbuffers.1.xofs,x
    pha

    and #$0007
    clc
    adc objbuffers.1.width,x
    dec a
    lsr a
    lsr a
    lsr a
    inc a
    sta objtmp1

    pla
    lsr a
    lsr a
    and #$FFFE
    clc
    adc.w mapadrrowlut, y
    tay

    clc
    adc maptile_L1d
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb
    and #$03FF
    asl a
    tax
    lda metatilesprop, x
    sta objtmp3
    bra _oicm1d21

_oicm1d2:
    tya
    clc
    adc maptile_L1d
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb
    and #$03FF
    asl a
    tax
    lda metatilesprop, x
    sta objtmp3

_oicm1d21:
    ldx objtmp2
    sta objbuffers.1.tilesprop,x
    lda objbuffers.1.tilesprop,x
    cmp #T_LADDER
    beq _oicm1d3
    cmp #T_PLATE
    beq _oicm1d3

    cmp #T_FIRES
    bne _oicm1d22
    lda #ACT_BURN
    sta objbuffers.1.action
    brl _oicm1dtstx

_oicm1d22:
    cmp #T_SPIKE
    bne _oicm1d23
    lda #ACT_DIE
    sta objbuffers.1.action
    brl _oicm1dtstx

_oicm1d23:
    and #$ff00
    beq _oicm1d4

_oicm1d3:
    lda objtmp3
    sta objbuffers.1.tilestand,x

    lda objbuffers.1.yvel+1,x
    and #$00ff
    clc
    adc objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    clc
    adc objbuffers.1.height,x
    inc a

    and #$fff8
    sec
    sbc objbuffers.1.yofs,x
    sec
    sbc objbuffers.1.height,x
    sta objbuffers.1.ypos+1,x
    stz objbuffers.1.yvel,x
    jmp _oicm1dtstx

_oicm1d4:
    rep #$20
    iny
    iny
    dec objtmp1
    beq _oicm1d5
    brl _oicm1d2

_oicm1d5:
    ldx objtmp2
    stz objbuffers.1.tilestand,x

    lda objbuffers.1.tilesprop,x
    beq _oicm1d61
    lda objbuffers.1.action,x
    and #ACT_CLIMB
    beq _oicm1d61
    brl _oicm1dtstx

_oicm1d61:
    brl _oicm1dtstx

_oicm1dtstyn:
    stz objbuffers.1.tilestand,x
    xba
    ora #$FF00
    clc
    adc objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    bpl +
    lda #0000

+   lsr a
    lsr a
    and #$FFFE
    tay

    lda objbuffers.1.xpos+1,x
    clc
    adc objbuffers.1.xofs,x
    pha

    and #$0007
    clc
    adc objbuffers.1.width,x
    dec a
    lsr a
    lsr a
    lsr a
    inc a
    sta objtmp1

    pla
    lsr a
    lsr a
    and #$FFFE
    clc
    adc.w mapadrrowlut, y
    tay

    clc
    adc maptile_L1d
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb
    and #$03FF
    asl a
    tax
    lda metatilesprop, x
    bra _oicm1dtstyn2

_oicm1dtstyn1:
    tya
    clc
    adc maptile_L1d
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb
    and #$03FF
    asl a
    tax
    lda metatilesprop, x

_oicm1dtstyn2:
    ldx objtmp2
    sta objbuffers.1.tilesprop,x
    lda objbuffers.1.tilesprop,x
    cmp #T_LADDER
    beq _oicm1dtstx

    and #$ff00
    beq _oicm1dtstyn3

    lda objbuffers.1.yvel+1,x
    ora #$ff00
    clc
    adc objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    clc
    adc #8
    and #$fff8
    sec
    sbc objbuffers.1.yofs,x
    sta objbuffers.1.ypos+1,x
    stz objbuffers.1.yvel,x
    brl _oicm1dtstx

_oicm1dtstyn3:
    iny
    iny
    dec objtmp1
    bne _oicm1dtstyn1


_oicm1dtstx:
    ldx objtmp2
    lda objbuffers.1.xvel,x
    bne _oicm1dtstx1
    jmp _oicm1dend
_oicm1dtstx1:
    bpl _oicm1dtstx11
    jmp _oicm1dtstxn

_oicm1dtstx11:
    lda objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    bpl +
    lda #0000

+   pha
    and #$0007
    clc
    adc objbuffers.1.height,x
    dec a
    lsr a
    lsr a
    lsr a
    inc a
    sta objtmp1

    pla
    lsr a
    lsr a
    and #$fffe
    tay

    lda objbuffers.1.xvel+1,x
    and #$00ff
    clc
    adc objbuffers.1.xpos+1,x
    clc
    adc objbuffers.1.xofs,x
    clc
    adc objbuffers.1.width,x
    lsr a
    lsr a
    and #$fffe
    clc
    adc.w mapadrrowlut, y

_oicm1dtstx13:
    tay
    clc
    adc maptile_L1d
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb
    and #$03FF
    asl a
    tax
    lda metatilesprop, x

    ldx objtmp2
    sta objbuffers.1.tilebprop,x
    lda objbuffers.1.tilebprop,x
    beq _oicm1dtstx14
    and #$ff00
    beq _oicm1dtstx14

    lda objbuffers.1.xvel+1,x
    and #$00ff
    clc
    adc objbuffers.1.xpos+ 1,x
    clc
    adc objbuffers.1.xofs,x
    clc
    adc objbuffers.1.width,x
    inc a
    and #$fff8
    sec
    sbc objbuffers.1.xofs,x
    sec
    sbc objbuffers.1.width,x
    sta objbuffers.1.xpos+ 1,x
    stz objbuffers.1.xvel,x
    brl _oicm1dend

_oicm1dtstx14:
    tya
    clc
    adc.l maprowsize
    dec objtmp1
    bne _oicm1dtstx13
    brl _oicm1dend

_oicm1dtstxn:
    lda objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    bpl +
    lda #0000

+   pha

    and #$0007
    clc
    adc objbuffers.1.height,x
    dec a
    lsr a
    lsr a
    lsr a
    inc a
    sta objtmp1

    pla
    lsr a
    lsr a
    and #$fffe
    tay

    lda objbuffers.1.xvel+1,x
    ora #$ff00
    clc
    adc objbuffers.1.xpos+1,x
    clc
    adc objbuffers.1.xofs,x

    bpl _oicm1dtstxnb
    jmp _oicm1dend

_oicm1dtstxnb:
    lsr a
    lsr a
    and #$fffe
    clc
    adc.w mapadrrowlut, y

_oicm1dtstxnc:
    tay
    clc
    adc maptile_L1d
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb
    and #$03FF
    asl a
    tax
    lda metatilesprop, x

    ldx objtmp2
    sta objbuffers.1.tilebprop,x
    lda objbuffers.1.tilebprop,x
    beq _oicm1dtstxnd

    and #$ff00
    beq _oicm1dtstxnd

    lda objbuffers.1.xvel+1,x
    ora #$ff00
    clc
    adc objbuffers.1.xpos+ 1,x
    clc
    adc objbuffers.1.xofs,x
    clc
    adc #8
    and #$fff8
    sec
    sbc objbuffers.1.xofs,x
    sta objbuffers.1.xpos+ 1,x
    stz objbuffers.1.xvel,x

    brl _oicm1dend

_oicm1dtstxnd:
    tya
    clc
    adc.l maprowsize
    dec objtmp1
    bne _oicm1dtstxnc

_oicm1dend:
    ; --- Sync objbuffers → workspace after collision ---
    ldx objtmp2
    SYNC_TO_WORKSPACE

    ply
    plx
    plb
    plp
    rtl

.ENDS

;==============================================================================
; CODE SECTION 3 - objLoadObjects
;==============================================================================

.SECTION ".objects3_text" SUPERFREE

.accu 16
.index 16
.16bit

;------------------------------------------------------------------------------
; void objLoadObjects(u8 *sourceO)
;------------------------------------------------------------------------------
objLoadObjects:
    php
    phb

    phx
    phy

    sep #$20
    lda #$0
    pha
    plb

    rep #$20
    lda 10,s                                ; sourceO addr (param 1)
    sta.l $4302
    lda #(OB_MAX*5)+1
    asl a
    sta.l $4305
    ldy #objtmpbuf.w
    sty $2181

    sep #$20
    lda #$7e
    sta.l $2183
    lda #$00                                ; bank = $00 (cproc: 16-bit pointers)
    sta.l $4304
    ldx #$8000
    stx $4300

    lda #$01
    sta $420B

    sep #$20
    lda #$7e
    pha
    plb

    rep #$20
    ldx #$0000

_oilo1:
    lda objtmpbuf,x
    cmp #$ffff
    beq _oiloend
    ; Push params in cproc L-to-R order (first param pushed first → farthest from SP)
    ; init(xp, yp, type, minx, maxx)
    pha                                     ; x (param 1, already in A from cmp)
    lda objtmpbuf+2,x                      ; y (param 2)
    pha
    lda objtmpbuf+4,x                      ; type (param 3)
    pha
    lda objtmpbuf+6,x                      ; minx (param 4)
    pha
    lda objtmpbuf+8,x                      ; maxx (param 5, last → closest to SP)
    pha
    lda objtmpbuf+4,x                      ; get type again
    asl a
    asl a
    tay
    inx
    inx
    inx
    inx
    inx
    inx
    inx
    inx
    inx
    inx
    stx objcidx
    lda objfctinit,y
    sta objfctcall
    lda objfctinit+2,y
    sta objfctcallh

    jsl jslcallfct

    ; --- After init callback: sync workspace back for newly created object ---
    ; objNew already copied to workspace, init may have modified it.
    ; Need to copy workspace back to objbuffers for the new object.
    rep #$20
    lda.l objgetid
    beq _oilo_skip_sync                     ; if objNew returned 0, no object created
    and #$00ff                              ; extract index
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    SYNC_FROM_WORKSPACE
_oilo_skip_sync:

    pla
    pla
    pla
    pla
    pla
    ldx objcidx
    bra _oilo1

_oiloend:
    ply
    plx
    plb
    plp
    rtl

.ENDS

;==============================================================================
; CODE SECTION 4 - objCollidObj
;==============================================================================

.SECTION ".objects4_text" SUPERFREE

.accu 16
.index 16
.16bit

;------------------------------------------------------------------------------
; u16 objCollidObj(u16 objhandle1, u16 objhandle2)
; Stack: 5-6 7-8
;------------------------------------------------------------------------------
objCollidObj:
    php
    phb

    phx
    phy

    sep #$20
    lda #$7e
    pha
    plb

    rep #$20
    stz.w tcc__r0

    ; cproc L-to-R: handle2(p2) SP+10, handle1(p1) SP+12
    lda 10,s                                ; handle2 (param 2, closest)
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tay
    lda objbuffers.1.xpos+1,y
    clc
    adc objbuffers.1.xofs,y
    bmi _oicoend
    sta objtmp1

    lda 12,s                                ; handle1 (param 1, farthest)
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    lda objbuffers.1.xpos+1,x
    clc
    adc objbuffers.1.xofs,x
    bpl _oicor1
    lda #$0000
_oicor1:
    sta objtmp2

    cmp objtmp1
    bcs _oicor2
    adc objbuffers.1.width,x

    cmp objtmp1
    bmi _oicoend
    bra _oicor3

_oicor2:
    lda objtmp1
    clc
    adc objbuffers.1.width,y
    cmp objtmp2
    bmi _oicoend

_oicor3:
    lda objbuffers.1.ypos+1,y
    clc
    adc objbuffers.1.yofs,y
    sta objtmp3

    lda objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    bpl _oicor4
    lda #$0000
_oicor4:
    sta objtmp4

    cmp objtmp3
    bcs _oicor5
    adc objbuffers.1.height,x
    cmp objtmp3
    bmi _oicoend

_oicor5:
    lda objtmp3
    clc
    adc objbuffers.1.height,y
    cmp objtmp4
    bmi _oicoend

    lda #$0001
    sta.w tcc__r0

_oicoend:

    ply
    plx
    plb
    plp
    rtl

.ENDS

;==============================================================================
; CODE SECTION 5 - objUpdateXY
;==============================================================================

.SECTION ".objects5_text" SUPERFREE

.accu 16
.index 16
.16bit

;------------------------------------------------------------------------------
; void objUpdateXY(u16 objhandle)
;
; Workspace sync: copies workspace → buffer before, buffer → workspace after.
;------------------------------------------------------------------------------
objUpdateXY:
    php
    phb

    phx

    sep #$20
    lda #$7e
    pha
    plb

    rep #$20
    lda 8,s                                 ; get index (5+1+2)

    ; --- Sync workspace → objbuffers ---
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    phx                                     ; save byte offset for sync-back
    SYNC_FROM_WORKSPACE

    clc
    lda objbuffers.1.xvel,x
    bpl _oicuxy2

    adc objbuffers.1.xpos,x
    sta objbuffers.1.xpos,x
    bcs _oicuxy1

    sep #$20
    dec objbuffers.1.xpos+2,x
    rep #$20
    brl _oicuxy1

_oicuxy2:
    adc objbuffers.1.xpos,x
    sta objbuffers.1.xpos,x
    bcc _oicuxy1
    sep #$20
    inc objbuffers.1.xpos+2,x
    rep #$20

_oicuxy1:
    clc
    lda objbuffers.1.yvel,x
    bpl _oicuxy3

    adc objbuffers.1.ypos,x
    sta objbuffers.1.ypos,x
    bcs _oicuxyend

    sep #$20
    dec objbuffers.1.ypos+2,x
    rep #$20
    brl _oicuxyend

_oicuxy3:
    adc objbuffers.1.ypos,x
    sta objbuffers.1.ypos,x
    bcc _oicuxyend
    sep #$20
    inc objbuffers.1.ypos+2,x
    rep #$20

_oicuxyend:
    ; --- Sync objbuffers → workspace ---
    plx                                     ; restore byte offset
    SYNC_TO_WORKSPACE

    plx
    plb
    plp
    rtl

.ENDS

;==============================================================================
; CODE SECTION 6 - objCollidMapWithSlopes
;==============================================================================

.SECTION ".objects6_text" SUPERFREE

.accu 16
.index 16
.16bit

_lutcol:
    .db $00,$01,$02,$03,$04,$05,$06,$07     ; 0020
    .db $07,$06,$05,$04,$03,$02,$01,$00     ; 0021
    .db $04,$04,$05,$05,$06,$06,$07,$07     ; 0022
    .db $07,$07,$06,$06,$05,$05,$04,$04     ; 0023
    .db $00,$00,$01,$01,$02,$02,$03,$03     ; 0024
    .db $03,$03,$02,$02,$01,$01,$00,$00     ; 0025
_lutcolInv:
    .db $07,$06,$05,$04,$03,$02,$01,$00     ; 0020
    .db $00,$01,$02,$03,$04,$05,$06,$07     ; 0021
    .db $07,$07,$06,$06,$05,$05,$04,$04     ; 0022
    .db $04,$04,$05,$05,$06,$06,$07,$07     ; 0024
    .db $03,$03,$02,$02,$01,$01,$00,$00     ; 0023
    .db $00,$00,$01,$01,$02,$02,$03,$03     ; 0025

;note: objtmp1 = tileCount
;note: objtmp2 = object byte offset
;note: objtmp3 = tileprop
;note: objtmp4 = enable slope flag
;note: tcc__r0 $00 = centerX
;note: tcc__r1 $02 = tile difference
;note: tcc__r2 $04 = centerX relative to tile
;note: tcc__r3 $06 = bottom spot
;note: tcc__r4 $08 = tileFlipState
;note: tcc__r5 $0A = slope correction value

;------------------------------------------------------------------------------
; Macros for slope management
;------------------------------------------------------------------------------

.MACRO OE_SETVELOCYTY
    lda objbuffers.1.yvel,x
    clc
    adc #GRAVITY
    cmp #MAX_Y_VELOCITY+1
    bmi \1
    lda #MAX_Y_VELOCITY
.ENDM

.MACRO OE_GETTILEPROP
    clc
    adc maptile_L1d
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb
    .if \1 == 1
        pha
        and #$C000
        sta $08
        pla
    .endif
    and #$03FF
    asl a
    tax
    lda metatilesprop, x
.ENDM

.MACRO OE_GETYOFFSETLUT
    bpl +
    lda #0000
+   lsr a
    lsr a
    and #$FFFE
.ENDM

.MACRO OE_SETCENTERXNEW
    OE_SETCENTERXOLD

    lda objbuffers.1.xvel,x
    bpl +
    lda objbuffers.1.xvel+1,x
    ora #$FF00
    bra ++
+   lda objbuffers.1.xvel+1,x
    and #$00ff
    bne ++
    inc a
++  clc
    adc $00
    sta $00
.ENDM

.MACRO OE_SETCENTERXOLD
    lda objbuffers.1.width,x
    lsr a
    clc
    adc objbuffers.1.xpos+1,x
    clc
    adc objbuffers.1.xofs,x
    sta $00
.ENDM

.MACRO OE_CPTETILENUM
    pha
    and #$0007
    clc
    adc \2
    dec a
    lsr a
    lsr a
    lsr a
    inc a
    .if \1 == 1
    sec
    sbc objtmp4
    bpl +
    lda #$0000
    +:
    .endif
    sta objtmp1
    pla
    lsr a
    lsr a
    and #$FFFE
.ENDM

.MACRO OE_SETYPOS
    lda objbuffers.1.yvel+1,x
    .if \1 == 1
        and #$00ff
    .else
        ora #$ff00
    .endif
    clc
    adc objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    .if \1 == 1
    clc
    adc objbuffers.1.height,x
    inc a
    .else
        clc
        adc #8
    .endif
    and #$fff8
    sec
    sbc objbuffers.1.yofs,x
    .if \1 == 1
        sec
        sbc objbuffers.1.height,x
    .endif
    sta objbuffers.1.ypos+1,x
    stz objbuffers.1.yvel,x
.ENDM

.MACRO OE_ADDX
    lda objbuffers.1.xvel+1,x
    .if \1 == 1
        and #$00ff
    .else
        ora #$ff00
    .endif
    clc
    adc objbuffers.1.xpos+1,x
    clc
    adc objbuffers.1.xofs,x
    .if \1 == 1
    clc
    adc objbuffers.1.width,x
    .endif
.ENDM

.MACRO OE_SETXPOS
    OE_ADDX \1
    .if \1 == 1
    inc a
    .else
        clc
        adc #8
    .endif
    and #$fff8
    sec
    sbc objbuffers.1.xofs,x
    .if \1 == 1
        sec
        sbc objbuffers.1.width,x
    .endif
    sta objbuffers.1.xpos+1,x
    stz objbuffers.1.xvel,x
.ENDM

.MACRO OE_SETOBJHANDLE_STK
    lda \1,s
    xba
    lsr a
    lsr a
    tax
    sta objtmp2
.ENDM

;------------------------------------------------------------------------------
; void objCollidMapWithSlopes(u16 objhandle)
;------------------------------------------------------------------------------
objCollidMapWithSlopes:
    php
    phb
    phx
    phy
    sep #$20
    lda #$7e
    pha
    plb
    rep #$20
    stz $0A

    ; --- Sync workspace → objbuffers ---
    lda 10,s                                ; get index
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    tax
    SYNC_FROM_WORKSPACE

    OE_SETOBJHANDLE_STK 10
    stz objtmp4

_oicmsPrecheck:
    ldx objtmp2
    lda objbuffers.1.yvel,x
    bpl _oicmsCheckSlopes1
    jmp _oicmsCheckXvelNotZero

_oicmsCheckSlopes1:
    stz objtmp1
    OE_SETCENTERXOLD
    lda objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    clc
    adc objbuffers.1.height,x
    sta $06
    dec a

    OE_GETYOFFSETLUT
    sta $02
    tay

_oicmsCheckSlopes11:
    lda $00
    lsr a
    lsr a
    and #$FFFE
    clc
    adc.w mapadrrowlut, y
    tay
    OE_GETTILEPROP 0
    sta objtmp3

_oicmsCheckSlopes12:
    and #T_SLOPES
    beq ++

    ldx objtmp2
    lda objtmp1
    bne +
    lda objbuffers.1.yvel+1,x
    and #$00ff
    clc
    adc $06
    dec a
    OE_GETYOFFSETLUT
    tay
    cmp $02
    beq _oicmsEnableResolveSlope
    lda objbuffers.1.yvel+1,x
    and #$FF07
    stz objbuffers.1.yvel+1,x

+   bra _oicmsEnableResolveSlope

++  lda objtmp1
    bne _oicmsCheckXvelNotZero

    lda objbuffers.1.yvel+1,x
    and #$00ff
    clc
    adc $06
    dec a
    OE_GETYOFFSETLUT
    tay

    inc objtmp1
    brl _oicmsCheckSlopes11

_oicmsEnableResolveSlope:
    inc objtmp4

_oicmsCheckXvelNotZero:
    ldx objtmp2
    lda objbuffers.1.xvel,x
    bne +
    brl _oicmsCheckYvelDirection
+   bpl _oicmsXRightReduceFriction
    jmp _oicmsXLeftReduceFriction

_oicmsXRightReduceFriction:
    sec
    sbc objfriction
    bpl _oicmststx12
    stz objbuffers.1.xvel,x
    jmp _oicmsCheckYvelDirection

_oicmststx12:
    sta objbuffers.1.xvel,x
    lda objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    bpl +
    lda #0000

+   OE_CPTETILENUM 1 objbuffers.1.height,x
    bne +
    jmp _oicmsCheckYvelDirection

+   tay
    OE_ADDX 1
    lsr a
    lsr a
    and #$fffe
    clc
    adc.w mapadrrowlut, y

_oicmsXResolveColRight:
    tay
    OE_GETTILEPROP 0

    ldx objtmp2
    sta objbuffers.1.tilebprop,x
    lda objbuffers.1.tilebprop,x
    beq _oicmsXSelectNextTile
    and #$ff00
    beq _oicmsXSelectNextTile

    OE_SETXPOS 1
    brl _oicmsCheckYvelDirection

_oicmsXSelectNextTile:
    tya
    clc
    adc.l maprowsize
    dec objtmp1
    bne _oicmsXResolveColRight
    brl _oicmsCheckYvelDirection

_oicmsXLeftReduceFriction:
    clc
    adc objfriction
    bmi _oicmststxna
    stz objbuffers.1.xvel,x
    brl _oicmsCheckYvelDirection

_oicmststxna:
    sta objbuffers.1.xvel,x
    lda objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    bpl +
    lda #0000

+   OE_CPTETILENUM 1 objbuffers.1.height,x
    tay

    OE_ADDX 0
    bpl _oicmststxnb
    jmp _oicmsCheckYvelDirection

_oicmststxnb:
    lsr a
    lsr a
    and #$fffe
    clc
    adc.w mapadrrowlut, y

_oicmststxnc:
    tay
    OE_GETTILEPROP 0
    ldx objtmp2
    sta objbuffers.1.tilebprop,x
    lda objbuffers.1.tilebprop,x
    beq _oicmststxnd

    and #$ff00
    beq _oicmststxnd
    OE_SETXPOS 0

    brl _oicmsCheckYvelDirection

_oicmststxnd:
    tya
    clc
    adc.l maprowsize
    dec objtmp1
    bne _oicmststxnc
    brl _oicmsCheckYvelDirection

_oicmsResolveSlope1:
    lda objbuffers.1.yvel+1,x
    and #$00ff
    clc
    adc $06
    dec a
    sta objtmp4

    OE_GETYOFFSETLUT
    tay
    OE_SETCENTERXNEW
    lsr a
    lsr a
    and #$FFFE
    clc
    adc.w mapadrrowlut, y
    tay
    OE_GETTILEPROP 1
    sta objtmp3
    cmp #$0030
    bcs _oicmsPrepYvelDown
    cmp #$0020
    bcc _oicmsPrepYvelDown

_oicmsResolveSlope2:
    lda $00
    and #$0007
    sta $04

    lda objtmp3
    sec
    sbc #T_SLOPES
    asl a
    asl a
    asl a
    adc $04
    tax

    lda $08
    sep #$20
    xba
    and #$40
    beq +
    lda.l _lutcol,x
    bra ++
+   lda.l _lutcolInv,x
++  sta $0A

_oicmsResolveSlope3:
    ldx objtmp2
    lda objtmp4

    and #$07
    cmp $0A
    rep #$20
    bcs +
    brl _oicms61

 +  lda objtmp3
    sta objbuffers.1.tilestand,x

    lda objtmp4
    and #$fff8
    sec
    sbc objbuffers.1.yofs,x
    sec
    sbc objbuffers.1.height,x
    sep #$20
    clc
    adc $0A
    rep #$20
    inc a

    sta objbuffers.1.ypos+1,x
    stz objbuffers.1.yvel,x
    brl _oicmsend

_oicmsCheckYvelDirection:
    ldx objtmp2
    lda objtmp4
    beq +
    brl _oicmsResolveSlope1
 +  lda objbuffers.1.yvel,x
    bpl _oicms11
    jmp _oicmststyn

_oicmsPrepYvelDown:
    rep #$20
    ldx objtmp2
    stz objtmp4
    lda objbuffers.1.yvel,x

_oicms11:
    xba
    and #$00FF
    clc
    adc objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    clc
    adc objbuffers.1.height,x
    bpl +
    lda #0000

+   lsr a
    lsr a
    and #$FFFE
    tay

    lda objbuffers.1.xpos+1,x
    clc
    adc objbuffers.1.xofs,x
    OE_CPTETILENUM 0 objbuffers.1.width,x
    clc
    adc.w mapadrrowlut, y

    tay
    OE_GETTILEPROP 0
    sta objtmp3
    bra _oicms21

_oicms2:
    tya
    OE_GETTILEPROP 0
    sta objtmp3

_oicms21:
    ldx objtmp2
    sta objbuffers.1.tilesprop,x
    lda objbuffers.1.tilesprop,x
    cmp #T_LADDER
    beq _oicms3
    cmp #T_PLATE
    beq _oicms3

    cmp #T_FIRES
    bne _oicms22
    lda #ACT_BURN
    sta objbuffers.1.action
    jmp _oicmsend

_oicms22:
    cmp #T_SPIKE
    bne _oicms23
    lda #ACT_DIE
    sta objbuffers.1.action
    jmp _oicmsend

_oicms23:
    and #$ff00
    beq _oicms4

_oicms3:
    lda objtmp3
    sta objbuffers.1.tilestand,x
    OE_SETYPOS 1
    jmp _oicmsend

_oicms4:
    rep #$20
    iny
    iny
    dec objtmp1
    beq _oicms5
    brl _oicms2

_oicms5:
    ldx objtmp2
    stz objbuffers.1.tilestand,x

    lda objbuffers.1.tilesprop,x
    beq _oicms61
    lda objbuffers.1.action,x
    and #ACT_CLIMB
    beq _oicms61
    brl _oicmsend

_oicms61:
    OE_SETVELOCYTY _oicms6

_oicms6:
    sta objbuffers.1.yvel,x
    brl _oicmsend

_oicmststyn:
    stz objbuffers.1.tilestand,x
    xba
    ora #$FF00
    clc
    adc objbuffers.1.ypos+1,x
    clc
    adc objbuffers.1.yofs,x
    bpl +

    lda #0000
+   lsr a
    lsr a
    and #$FFFE
    tay

    lda objbuffers.1.xpos+1,x
    clc
    adc objbuffers.1.xofs,x

    OE_CPTETILENUM 0 objbuffers.1.width

    clc
    adc.w mapadrrowlut, y
    tay

    OE_GETTILEPROP 0
    bra _oicmststyn2

_oicmststyn1:
    tya
    OE_GETTILEPROP 0

_oicmststyn2:
    ldx objtmp2
    sta objbuffers.1.tilesprop,x
    lda objbuffers.1.tilesprop,x
    cmp #T_LADDER
    beq _oicmststyn4

    and #$ff00
    beq _oicmststyn3

    OE_SETYPOS 0
    bra _oicmststyn4

_oicmststyn3:
    iny
    iny
    dec objtmp1
    bne _oicmststyn1

_oicmststyn4:
    ldx objtmp2
    OE_SETVELOCYTY _oicmststyn5
_oicmststyn5:
    sta objbuffers.1.yvel,x

_oicmsend:
    ; --- Sync objbuffers → workspace after collision ---
    ldx objtmp2
    SYNC_TO_WORKSPACE

    ply
    plx
    plb
    plp
    rtl

.ENDS
