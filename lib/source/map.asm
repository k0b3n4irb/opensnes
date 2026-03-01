;==============================================================================
; OpenSNES Map Engine
;==============================================================================
;
; Scrolling tilemap engine for Mode 1 backgrounds using metatiles.
; Provides hardware-efficient incremental VRAM updates during VBlank.
;
; Based on: PVSnesLib map engine by Alekmaul
; Original: undisbeliever's castle_platformer
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

.DEFINE MAP_BG1_ADR     $6800

.EQU REG_BG1HOFS        $210D
.EQU REG_BG1VOFS        $210E
.EQU REG_BG2HOFS        $210F
.EQU REG_BG2VOFS        $2110

.DEFINE MAP_MAXROWS     256*2               ; Maximum number of rows in a map
.DEFINE MAP_MAXMTILES   512                 ; Number of metatiles per map

.DEFINE MAP_SCRLR_SCRL  128                 ; Screen position to begin scroll left & right
.DEFINE MAP_SCRUP_SCRL  80                  ; Screen position to begin scroll up & down

.DEFINE MAP_MTSIZE      8                   ; Size of metatiles (8pix)
.DEFINE MAP_DISPW       32                  ; 256/8
.DEFINE MAP_DISPH       28                  ; 224/8 -> default SNES screen for scrolling

.DEFINE MAP_UPD_HORIZ   $01
.DEFINE MAP_UPD_POSIT   $02
.DEFINE MAP_UPD_VERT    $80
.DEFINE MAP_UPD_WHOLE   $FF

.DEFINE MAP_OPT_1WAY    $01
.DEFINE MAP_OPT_BG2     $02

;------------------------------------------------------------------------------
; Metatile structure
;------------------------------------------------------------------------------

.STRUCT metatiles_t
topleft                 DSW MAP_MAXMTILES*4 ; a metatile is 4 8x8 pixels max
.ENDST

;------------------------------------------------------------------------------
; RAM - Bank $00 (scroll/display offsets, C-accessible)
;------------------------------------------------------------------------------

.RAMSECTION ".map_bank00" BANK 0 SLOT 1

dispxofs_L1             DW                  ; X scroll offset of layer 1
dispyofs_L1             DW                  ; Y scroll offset of layer 1

bgvvramloc_L1           DW                  ; VRAM word address to store vertical buffer
bgleftvramlocl_L1       DW                  ; VRAM word address for horizontal buffer (left tilemap)
bgrightvramlocr_L1      DW                  ; VRAM word address for horizontal buffer (right tilemap)

x_pos                   DW                  ; x Position of the screen (C-accessible)
y_pos                   DW                  ; y Position of the screen (C-accessible)

.ENDS

;------------------------------------------------------------------------------
; RAM - Bank $7E (bulk data, NOT C-accessible)
;------------------------------------------------------------------------------

.RAMSECTION ".map_bank7e" BANK $7E SLOT 2

metatiles INSTANCEOF metatiles_t            ; entry for each metatile definition

metatilesprop           DSW MAP_MAXMTILES*2 ; tiles properties (block, spike, fire)

mapwidth                DW                  ; Width of the map in pixels
mapheight               DW                  ; Height of the map in pixels

maxx_pos                DW                  ; Maximum value of x_pos
maxy_pos                DW                  ; Maximum value of y_pos

maptile_L1b             DB                  ; map layer 1 tiles bank address
maptile_L1d             DW                  ; map layer 1 tiles data address

mapadrrowlut            DSW MAP_MAXROWS     ; address of each row for fast computation
maprowsize              DW                  ; size of 1 row regarding map

bg_L1                   DSW 32*32           ; Buffer for display layer 1
bgvertleftbuf_L1        DSW 32              ; Vertical tile update buffer

mapvisibletopleftidx    DW                  ; Tile index for top left of visible display
mapvisibletopleftxpos   DW                  ; Pixel X of mapvisibletopleftidx
mapvisibletopleftypos   DW                  ; Pixel Y of mapvisibletopleftidx

mapdisplaydeltax        DW                  ; Pixel X for tile 0,0 of SNES tilemap
mapdisplaydeltay        DW                  ; Pixel Y for tile 0,0 of SNES tilemap

mapcolidx               DW                  ; Topmost tile updated in vertical buffer
maprowidx               DW                  ; Leftmost tile updated in horizontal buffer
mapcolmetatileofs       DW                  ; Tile offset for vertical update
maprowmetatileofs       DW                  ; VRAM offset for horizontal update

mapendloop              DW                  ; Ending position of draw loop

mapupdbuf               DB                  ; State of buffer update
mapdirty                DB                  ; 1 if map needs full redraw

maptmpvalue             DW                  ; Temporary value (tile flipping, etc.)

mapoptions              DB                  ; Map options (1-way scroll, BG2 mode)

.ENDS

;==============================================================================
; CODE SECTION 0 - Core map functions
;==============================================================================

.SECTION ".maps0_text" SUPERFREE

.accu 16
.index 16
.16bit

;------------------------------------------------------------------------------
; void mapUpdateCamera(u16 xpos, u16 ypos)
;------------------------------------------------------------------------------
mapUpdateCamera:
    php
    phb

    sep #$20
    lda #$7e
    pha
    plb

    ; cproc L-to-R: ypos(p2) SP+6, xpos(p1) SP+8
    rep #$20
    lda 8,s                                 ; xpos (param 1)
    sec
    sbc.l x_pos
    cmp #(256-MAP_SCRLR_SCRL)
    bmi _muc2

    lda 8,s
    sec
    sbc    #(256-MAP_SCRLR_SCRL)
    cmp.l maxx_pos
    bcc _muc1
    lda.l maxx_pos
_muc1:
    sta.l x_pos
    brl _muc4

_muc2:
    sep #$20
    lda mapoptions
    and #MAP_OPT_1WAY
    rep #$20
    bne _muc4

    cmp #MAP_SCRLR_SCRL
    bpl _muc4
    lda 8,s
    sec
    sbc #MAP_SCRLR_SCRL
    bpl _muc22
    lda #$0

_muc22:
    sta.l x_pos

_muc4:
    lda 6,s                                 ; ypos (param 2)
    sec
    sbc.l   y_pos
    cmp #(224-MAP_SCRUP_SCRL)
    bmi _muc6

    lda 6,s
    sec
    sbc    #(224-MAP_SCRUP_SCRL)
    cmp.l maxy_pos
    bcc _muc5
    lda.l maxy_pos
_muc5:
    sta.l y_pos
    brl _mucend

_muc6:
    cmp #MAP_SCRUP_SCRL
    bpl _mucend
    lda 6,s
    sec
    sbc    #MAP_SCRUP_SCRL
    bpl _muc62
    lda #$0

_muc62:
    sta.l y_pos

_mucend:
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; void mapLoad(u8 *layer1map, u8 *layertiles, u8 *tilesprop)
; Stack: 5-8 9-12 13-16
;------------------------------------------------------------------------------
mapLoad:
    php
    phb

    phx
    phy

    ; cproc L-to-R: tilesprop(p3) SP+10, layertiles(p2) SP+12, layer1map(p1) SP+14
    ; cproc passes 16-bit pointers only (no bank byte), use bank $00
    sep #$20
    lda #$00                                ; bank = $00 (cproc: 16-bit pointers)
    pha
    plb
    sta.l maptile_L1b

    rep #$20
    lda 14,s                                ; layer1map addr (param 1)
    tax
    lda 0,x                                 ; get mapwidth
    sta.l mapwidth
    lda 2,x                                 ; get mapheight
    sta.l mapheight

    lda #0
    sta.l x_pos
    sta.l y_pos

    lda 14,s                                ; layer1map addr (param 1) again
    clc
    adc.w #0006                             ; add width, height and size
    sta.l maptile_L1d

    lda 12,s                                ; layertiles addr (param 2)
    sta.l $4302

    lda #MAP_MAXMTILES*4*2                  ; metatile max are 4 x 8x8
    sta.l $4305

    lda #metatiles.w
    sta.l $2181

    sep #$20
    lda #$7e
    sta.l $2183

    lda #$0
    pha
    plb

    lda #$00                                ; bank = $00 (cproc: 16-bit pointers)
    sta.l $4304

    ldx #$8000                              ; type of DMA
    stx $4300

    lda #$01
    sta $420B                               ; do dma for transfer

    rep	#$20
    lda	10,s	                            ; tilesprop addr (param 3)
	sta.l	$4302

    lda.w #MAP_MAXMTILES*2*2
    sta.l $4305

    ldy #metatilesprop.w
    sty $2181

    sep	#$20
    lda #$7e
    sta.l $2183

    lda #$00                                ; bank = $00 (cproc: 16-bit pointers)
    sta.l $4304

    ldx	#$8000
	stx	$4300

    lda #$01
    sta $420B

    stz.w dispxofs_L1
    stz.w dispyofs_L1

    lda #$0
    sta.l mapupdbuf
    sta.l mapdirty
    sta.l mapoptions

    rep #$31
    lda.l mapwidth
    adc #MAP_MTSIZE - 1                     ; carry clear from REP
    lsr
    lsr
    and #$FFFE
    sta.l maprowsize

    lda.l mapwidth
    sec
    sbc #256
    sta.l maxx_pos

    lda.l mapheight
    cmp.w #224
    bcs _mini1
    lda #224

_mini1:
    sec
    sbc #224
    sta.l maxy_pos

    lda #MAP_MAXROWS
    tay
    lda    #0
    ldx    #0

_mini2:
    sta.l mapadrrowlut,x
    clc
    adc.l maprowsize
    inx
    inx
    dey
    bne _mini2

    ply
    plx
    plb

;------------------------------------------------------------------------------
; mapRefreshAll - internal: rebuild entire visible tilemap
;------------------------------------------------------------------------------
mapRefreshAll:
    phb

    sep #$20
    lda #$7e
    pha
    plb

_mapRefreshAll1:
    phx
    phy

    rep #$30
    lda.l y_pos
    lsr
    lsr
    and #$FFFE
    tax

    lda.l x_pos
    lsr
    lsr
    and #$FFFE
    clc
    adc mapadrrowlut,x
    sta mapvisibletopleftidx

    clc
    adc.w mapadrrowlut+MAP_DISPH*2
    clc
    adc #(MAP_DISPW*2)-2
    tax

    lda #(MAP_DISPH+1)*1*32*2-2
_mapDAS:
    tay
    sec
    sbc #64
    sta mapendloop

_mapDAS1:
    phx
    lda maptile_L1d
    clc
    adc 1,s
    tax

    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb

    pha
    and #$03FF
    asl a
    tax
    lda.l metatiles.topleft,x
    sta.w maptmpvalue
    pla
    and #$C000
    ora maptmpvalue

    sta.w bg_L1, y
    plx

    dex
    dex
    dey
    dey

    cpy mapendloop
    bne _mapDAS1

    txa
    sec
    sbc.l maprowsize
    clc
    adc #MAP_DISPW*2
    tax

    tya
    bmi _mapDAS2
    brl _mapDAS

_mapDAS2:
    lda.l x_pos
    and.w #$FFFF - (MAP_MTSIZE - 1)
    sta.l mapdisplaydeltax
    sta.l mapvisibletopleftxpos

    lda.l y_pos
    and.w #$FFFF - (MAP_MTSIZE - 1)
    sta.l mapdisplaydeltay
    sta.l mapvisibletopleftypos

    lda.l x_pos
    and.w #(MAP_MTSIZE - 1)
    sta.l dispxofs_L1

	lda.l y_pos
	and.w #(MAP_MTSIZE - 1)
	dec a
	sta.l dispyofs_L1

    stz mapcolidx
    stz maprowidx
    stz mapcolmetatileofs
    stz maprowmetatileofs

    lda.l mapvisibletopleftidx
    clc
    adc #(MAP_DISPW + 1) * 2
    jsr _ProcessVerticalBuffer
    lda #MAP_BG1_ADR + 32 * 32
    sta.l bgvvramloc_L1

    sep    #$20
    lda #MAP_UPD_WHOLE
    sta.l mapupdbuf

    lda #$0
    sta.l mapdirty

    ply
    plx

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; _ProcessHorizontalBuffer (internal)
; A = tile index of the leftmost displayed tile
;------------------------------------------------------------------------------
_ProcessHorizontalBuffer:
    sta.l mapendloop
    clc
    adc.w #(MAP_DISPW + 1) * 2
    tax

    lda.l maprowidx
    clc
    adc.w #(32 + 1) * 2 - 2
    and.w #$7F
    tay

_phb1:
    dex
    dex
    phx

    lda maptile_L1d
    clc
    adc 1,s
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb

    pha
    and #$03FF
    asl a
    tax
    lda.l metatiles.topleft,x
    sta.w maptmpvalue
    pla
    and #$C000
    ora maptmpvalue

    sta bg_L1, y

    plx

    dey
    dey
    bpl _phb2
    ldy #64 * 2 - 2

_phb2:
    cpx.w mapendloop
    bpl _phb1

    rts

;------------------------------------------------------------------------------
; _ProcessVerticalBuffer (internal)
; A = tile index of the topmost displayed tile
;------------------------------------------------------------------------------
_ProcessVerticalBuffer:
    tay
    sec
    sbc.l maprowsize
    sta.l mapendloop
    tya

    clc
    adc.w mapadrrowlut+MAP_DISPH * 2
    dec a
    dec a
    tax

    lda.l mapcolidx
    clc
    adc.w #29*2 - 2
    and.w #$3F
    tay

_pvb1:
    phx

    lda maptile_L1d
    clc
    adc 1,s
    tax
    phb
    sep #$20
    lda #$00
    pha
    plb
    rep #$20
    lda 0,x
    plb

    pha
    and #$03FF
    asl a
    tax
    lda.l metatiles.topleft,x
    sta.w maptmpvalue
    pla
    and #$C000
    ora maptmpvalue

    sta bgvertleftbuf_L1, Y

    pla
    sec
    sbc.l maprowsize
    tax

    dey
    dey
    bpl _pvb2
    ldy #32 * 2 - 2

_pvb2:
    cpx.w mapendloop
    bpl _pvb1

    rts

;------------------------------------------------------------------------------
; void mapVblank(void)
;------------------------------------------------------------------------------
mapVblank:
    php
    phb

    phx
    phy

    sep #$20
    lda.b #$0
    pha
    plb

    lda.l mapupdbuf
    bne _mapvb1
    brl _mapvbend

_mapvb1:
    bpl _mapvb2

    lda #$81                                ; VMAIN_INCREMENT_HIGH | VMAIN_INCREMENT_32
    sta.l $2115

    ldx #$1801
    stx $4300

    ldx #bgvertleftbuf_L1.w
    stx $4302
    lda #:bgvertleftbuf_L1
    sta.l $4304

    ldx bgvvramloc_L1
    ldy #32*2
    lda #$1

    stx $2116
    sty $4305
    sta.l $420B

    lda.l mapupdbuf

_mapvb2:
    cmp #MAP_UPD_WHOLE
    bne _mapvb3

    lda #$80
    sta.l $2115

    ldx #bg_L1.w
    stx $4302

    ldx #MAP_BG1_ADR
    stx $2116

    ldy #32 * 32 * 2                       ; Transfer all 32 rows (rows 30-31 are BSS-zeroed = tile 0)
    sty $4305

    lda #$1
    sta.l $420B

    brl _mapvbend

_mapvb3:
    lsr a
    bcs _mapvb4
    brl _mapvbend

_mapvb4:
    lda #$80
    sta.l $2115

    ldx #$1801
    stx $4300

    ldx #bg_L1.w
    stx $4302

    lda #:bg_L1
    sta.l $4304

    ldy #2 * 32
    lda #$1

    ldx bgleftvramlocl_L1
    stx $2116
    sty $4305
    sta.l $420B

    ldx bgrightvramlocr_L1
    stx $2116
    sty $4305
    sta.l $420B

_mapvbend:
    sep #$20
    lda.l mapoptions
    and #MAP_OPT_BG2
    bne _mapvbbg2
    lda.l dispxofs_L1
    sta.l REG_BG1HOFS
    lda.l dispxofs_L1 + 1
    sta.l REG_BG1HOFS
    lda.l dispyofs_L1
    sta.l REG_BG1VOFS
    lda.l dispyofs_L1 + 1
    sta.l REG_BG1VOFS
    bra _mapvbend1

_mapvbbg2:
    lda.l dispxofs_L1
    sta.l REG_BG2HOFS
    lda.l dispxofs_L1 + 1
    sta.l REG_BG2HOFS
    lda.l dispyofs_L1
    sta.l REG_BG2VOFS
    lda.l dispyofs_L1 + 1
    sta.l REG_BG2VOFS
    bra _mapvbend1

_mapvbend1:
    lda #$0
    sta.l mapupdbuf

    ply
    plx

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; void mapUpdate(void)
;------------------------------------------------------------------------------
mapUpdate:
    php

    sep #$20
    lda.l mapdirty
    beq _maupd1
    jmp mapRefreshAll

_maupd1:
    phb

    lda #$7e
    pha
    plb

    rep #$30
    lda.l x_pos
    sec
    sbc.w mapvisibletopleftxpos
    bcc _mappud2
    cmp.w #MAP_MTSIZE
    bcc _maupd41
    cmp.w #MAP_MTSIZE * 2
    bcc _maupd4
    jmp _mapRefreshAll1

_maupd4:
    lda.l mapvisibletopleftxpos
    adc.w #MAP_MTSIZE
    sta.l mapvisibletopleftxpos

    lda.l mapvisibletopleftidx
    inc a
    inc a
    sta.l mapvisibletopleftidx

    clc
    adc.w #(MAP_DISPW + 1) * 2
    jsr _ProcessVerticalBuffer

    lda.l maprowidx
    clc
    adc #1 * 2
    sta.l maprowidx

    lda.l mapcolmetatileofs
    inc a
    and #$003F / 1
    sta.l mapcolmetatileofs

    eor #$0020 / 1
    bit #$0020 / 1
    beq _maupd42
    eor #$0420 / 1

_maupd42:
    clc
    adc #MAP_BG1_ADR
    sta.l bgvvramloc_L1

    sep #$20
    lda #MAP_UPD_VERT
    tsb mapupdbuf

    rep    #$20
_maupd41:
    bra _mapupd3

_mappud2:
    cmp.w #$FFF8
    bpl _mapupd5
    jmp _mapRefreshAll1

_mapupd5:
    lda.l mapvisibletopleftxpos
    sec
    sbc #MAP_MTSIZE
    sta.l mapvisibletopleftxpos

    lda.l mapvisibletopleftidx
    dec a
    dec a
    sta.l mapvisibletopleftidx
    inc a
    inc a
    jsr _ProcessVerticalBuffer

    lda.l maprowidx
    sec
    sbc #1 * 2
    sta.l maprowidx

    lda.l mapcolmetatileofs
    dec a
    and #$003F / 1
    sta.l mapcolmetatileofs

    bit #$0020 / 1
    beq _mapupd51
    EOR #$0420 / 1
_mapupd51:
    clc
    adc #MAP_BG1_ADR
    sta.l bgvvramloc_L1

    sep #$20
    lda #MAP_UPD_VERT
    tsb mapupdbuf

    rep #$20
_mapupd3:
    lda.l x_pos
    sec
    sbc mapdisplaydeltax
    sta.l dispxofs_L1

    lda.l y_pos
    sec
    sbc mapvisibletopleftypos
    bcc _mapupd6

    cmp #MAP_MTSIZE
    bcc _mapUpd81
    cmp #MAP_MTSIZE * 2
    bcc _mapupd8
    jmp _mapRefreshAll1
_mapupd8:
    lda.l mapvisibletopleftypos
    adc.w #MAP_MTSIZE
    sta.l mapvisibletopleftypos

    lda.l mapvisibletopleftidx
    clc
    adc.l maprowsize
    sta.l mapvisibletopleftidx
    clc
    adc.w mapadrrowlut+MAP_DISPH * 2
    jsr _ProcessHorizontalBuffer

    lda.l mapcolidx
    clc
    adc.w #1 * 2
    sta.l    mapcolidx

    lda.l maprowmetatileofs
    clc
    adc.w #32 * 1
    sta.l    maprowmetatileofs

    clc
    adc.w #28 * 32
    and #$03FF

    clc
    adc.w #MAP_BG1_ADR
    sta.l bgleftvramlocl_L1
    clc
    adc.w #32 * 32
    sta.l bgrightvramlocr_L1

    sep    #$20
    lda #MAP_UPD_HORIZ
    tsb    mapupdbuf

    rep    #$30
_mapUpd81:
    bra _mapupd9
_mapupd6:
    cmp.w #$FFF8
    bpl _mapupda
    jmp _mapRefreshAll1

_mapupda:
    lda.l mapvisibletopleftypos
    sec
    sbc #MAP_MTSIZE
    sta.l mapvisibletopleftypos

    lda.l mapvisibletopleftidx
    sec
    sbc.l maprowsize
    sta.l mapvisibletopleftidx
    jsr _ProcessHorizontalBuffer

    lda.l mapcolidx
    sec
    sbc #1 * 2
    sta.l mapcolidx

    lda.l maprowmetatileofs
    sec
    sbc #32 * 1
    sta.l maprowmetatileofs

    and #$03FF
    clc
    adc #MAP_BG1_ADR
    sta.l bgleftvramlocl_L1
    clc
    adc #32 * 32
    sta.l bgrightvramlocr_L1

    sep #$20
    lda #MAP_UPD_HORIZ
    tsb mapupdbuf
    rep #$20

_mapupd9:
	lda.l y_pos
	clc
	sbc.l mapdisplaydeltay
	sta.l dispyofs_L1

    sep #$20
    lda #MAP_UPD_POSIT
    tsb mapupdbuf

    plb
    plp
    rtl

.ENDS

;==============================================================================
; CODE SECTION 1 - mapGetMetaTile
;==============================================================================

.SECTION ".maps1_text" SUPERFREE

;------------------------------------------------------------------------------
; u16 mapGetMetaTile(u16 xpos, u16 ypos)
;------------------------------------------------------------------------------
mapGetMetaTile:
    php

    phx
    phy

    ; cproc L-to-R: ypos(p2) SP+9, xpos(p1) SP+11
    rep #$30
    lda  9,s                                ; ypos (param 2, closest)
    lsr
    lsr
    and #$FFFE
    tax

    lda 11,s                                ; xpos (param 1, farthest)
    lsr
    lsr
    and #$FFFE
    clc
    adc mapadrrowlut,x
    tax

    phx
    lda maptile_L1d
    clc
    adc 1,s
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
    plx

    sta.w tcc__r0

    ply
    plx
    plp
    rtl

.ENDS

;==============================================================================
; CODE SECTION 2 - mapGetMetaTilesProp
;==============================================================================

.SECTION ".maps2_text" SUPERFREE

;------------------------------------------------------------------------------
; u16 mapGetMetaTilesProp(u16 xpos, u16 ypos)
;------------------------------------------------------------------------------
mapGetMetaTilesProp:
    php

    phx
    phy

    ; cproc L-to-R: ypos(p2) SP+9, xpos(p1) SP+11
    rep #$30
    lda  9,s                                ; ypos (param 2, closest)
    lsr
    lsr
    and #$FFFE
    tax

    lda 11,s                                ; xpos (param 1, farthest)
    lsr
    lsr
    and #$FFFE
    clc
    adc mapadrrowlut,x
    tax

    phx
    lda maptile_L1d
    clc
    adc 1,s
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
    plx

    asl a
    tax
    lda	metatilesprop, x

    sta.w tcc__r0

    ply
    plx
    plp
    rtl

.ENDS

;==============================================================================
; CODE SECTION 3 - mapSetMapOptions
;==============================================================================

.SECTION ".maps3_text" SUPERFREE

;------------------------------------------------------------------------------
; void mapSetMapOptions(u8 optmap)
;------------------------------------------------------------------------------
mapSetMapOptions:
    php
    phb

    sep #$20
    lda.b #$7E
    pha
    plb

    lda 6,s                                 ; get options
    sta mapoptions

    plb
    plp
    rtl

.ENDS
