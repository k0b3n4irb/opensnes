;==============================================================================
; OpenSNES CRT0 - Universal Startup Code
;==============================================================================
;
; This file initializes the SNES hardware and C runtime before calling main().
;
; Execution flow:
;   1. Reset vector jumps to Start
;   2. Switch to 65816 native mode, set up stack
;   3. Initialize all PPU and CPU registers
;   4. Enable NMI (VBlank interrupt)
;   5. Call main()
;
; Author: OpenSNES Team
; License: MIT
;
;==============================================================================

; Include ROM header (memory map, vectors)
.include "hdr.asm"

;------------------------------------------------------------------------------
; Compiler Imaginary Registers
;------------------------------------------------------------------------------
; 816-tcc uses these addresses for intermediate calculations.
; They MUST be at $00:0000 for the compiler to work correctly.
;------------------------------------------------------------------------------

.RAMSECTION ".registers" BANK 0 SLOT 1 ORGA 0 FORCE
    tcc__r0     dsb 2
    tcc__r0h    dsb 2
    tcc__r1     dsb 2
    tcc__r1h    dsb 2
    tcc__r2     dsb 2
    tcc__r2h    dsb 2
    tcc__r3     dsb 2
    tcc__r3h    dsb 2
    tcc__r4     dsb 2
    tcc__r4h    dsb 2
    tcc__r5     dsb 2
    tcc__r5h    dsb 2
    tcc__r9     dsb 2
    tcc__r9h    dsb 2
    tcc__r10    dsb 2
    tcc__r10h   dsb 2
.ENDS

;------------------------------------------------------------------------------
; System Variables
;------------------------------------------------------------------------------

.RAMSECTION ".system" BANK 0 SLOT 1
    vblank_flag     dsb 1   ; Set by NMI handler, cleared by WaitVBlank
.ENDS

;------------------------------------------------------------------------------
; ROM Setup
;------------------------------------------------------------------------------

.BANK 0
.ORG 0
.EMPTYFILL $00

;------------------------------------------------------------------------------
; Note: EmptyHandler is defined in hdr.asm (required by vector tables)
;------------------------------------------------------------------------------

;==============================================================================
; Hardware Initialization
;==============================================================================

.SECTION ".init" SEMIFREE

;------------------------------------------------------------------------------
; InitHardware - Initialize All SNES Registers
;------------------------------------------------------------------------------
; Sets PPU and CPU registers to safe defaults.
; Screen is blanked during initialization.
;------------------------------------------------------------------------------
InitHardware:
    rep #$10            ; 16-bit X/Y
    sep #$20            ; 8-bit A

    ; Screen off (force blank)
    lda #$8F
    sta $2100

    ; Clear sprite registers
    stz $2101           ; OBJSEL
    stz $2102           ; OAMADDL
    stz $2103           ; OAMADDH

    ; Clear BG mode and tilemap registers ($2105-$210C)
    ldx #$2105
-   stz.w $0000,x
    inx
    cpx #$210D
    bne -

    ; Clear scroll registers ($210D-$2114) - write twice each
    ldx #$210D
-   stz.w $0000,x
    stz.w $0000,x
    inx
    cpx #$2115
    bne -

    ; VRAM increment mode
    lda #$80
    sta $2115

    ; Clear VRAM address
    stz $2116
    stz $2117

    ; Mode 7 - identity matrix
    stz $211A
    stz $211B
    lda #$01
    sta $211B
    stz $211C
    stz $211C
    stz $211D
    stz $211D
    stz $211E
    lda #$01
    sta $211E
    stz $211F
    stz $211F
    stz $2120
    stz $2120

    ; Clear CGRAM address and set background color to black
    stz $2121           ; CGADD = 0
    stz $2122           ; CGDATA low = 0 (black)
    stz $2122           ; CGDATA high = 0

    ; Clear window registers ($2123-$212B)
    ldx #$2123
-   stz.w $0000,x
    inx
    cpx #$212C
    bne -

    ; Main screen - BG1 enabled
    lda #$01
    sta $212C

    ; Sub screen disabled
    stz $212D
    stz $212E
    stz $212F

    ; Color math
    lda #$30
    sta $2130
    stz $2131
    lda #$E0
    sta $2132

    ; Screen mode
    stz $2133

    ; CPU registers - disable interrupts initially
    stz $4200

    ; I/O port
    lda #$FF
    sta $4201

    ; Clear math/DMA registers ($4202-$420D)
    ldx #$4202
-   stz $0000,x
    inx
    cpx #$420E
    bne -

    rts

.ENDS

;==============================================================================
; Entry Point
;==============================================================================

.SECTION ".start" SEMIFREE

.ACCU 16
.INDEX 16
.16BIT

;------------------------------------------------------------------------------
; Start - Reset Vector Entry Point
;------------------------------------------------------------------------------
Start:
    sei                 ; Disable interrupts

    ; Switch to native mode
    clc
    xce

    ; 16-bit X/Y, 8-bit A
    rep #$18
    sep #$20

    ; Set up stack at $1FFF
    ldx #$1FFF
    txs

    ; Initialize hardware
    jsr InitHardware

    ; Clear VBlank flag
    stz vblank_flag

    ; Switch to 16-bit mode
    rep #$30

    ; Set Direct Page to compiler registers
    lda #tcc__r0
    tcd

    ; Set Data Bank to $7E (Work RAM)
    pea $7E7E
    plb
    plb

    ; Clear compiler registers
    stz tcc__r0
    stz tcc__r1

    ; Enable NMI (VBlank interrupt)
    sep #$20
    lda #$81            ; NMI + auto joypad
    sta $4200
    rep #$20

    ; Call main() - use JSL since generated code returns with RTL
    jsl main

    ; main() returned - halt
    ; QBE returns value in A register (already there after jsl main returns)
    sep #$20
    sta $FFFD           ; Store for test runners
    stp                 ; Stop CPU

.ENDS

;==============================================================================
; NMI (VBlank) Handler
;==============================================================================

.SECTION ".nmi" SEMIFREE

;------------------------------------------------------------------------------
; NmiHandler - VBlank Interrupt
;------------------------------------------------------------------------------
; Called at the start of each VBlank period (~60Hz).
; Sets vblank_flag for synchronization.
;------------------------------------------------------------------------------
NmiHandler:
    rep #$30
    pha
    phx
    phy
    phd

    ; Set direct page
    lda #tcc__r0
    tcd

    ; Acknowledge NMI
    sep #$20
    lda $4210

    ; Set VBlank flag
    lda #$01
    sta vblank_flag

    ; Restore and return
    rep #$30
    pld
    ply
    plx
    pla
    rti

.ENDS

;==============================================================================
; OAM Helper Functions
;==============================================================================
; These assembly helpers work around 816-tcc code generation issues
; with volatile pointer writes in the main loop.
;==============================================================================

.SECTION ".oam_helpers" SEMIFREE

;------------------------------------------------------------------------------
; oam_set_tile - Set sprite 0's tile number
; Called via JSL, so use RTL to return
; Stack after JSL+PHP: [P:1] [retaddr:3] [param:1] -> param at offset 5
;------------------------------------------------------------------------------
oam_set_tile:
    php
    sep #$20            ; 8-bit A
    lda #$02            ; OAM address = byte 2 (tile)
    sta $2102           ; OAMADDL
    stz $2103           ; OAMADDH = 0
    lda 5,s             ; Get tile param (JSL pushes 3 bytes, PHP pushes 1)
    sta $2104           ; OAMDATA = tile
    plp
    rtl                 ; Return from JSL (not RTS!)

.ENDS

;==============================================================================
; External References
;==============================================================================
; Note: main() is defined in the compiled C code that follows this file.
; No .GLOBAL needed since we build as a single combined file.

.SECTION ".rodata.1" SUPERFREE
tile_data:
	.db 170
	.db 85
	.db 170
	.db 85
	.db 170
	.db 85
	.db 170
	.db 85
.ENDS


; Function: load_tile (framesize=16, slots=7, alloc_slots=1)
; temp 64: slot=-1, alloc=0
; temp 65: slot=-1, alloc=-1
; temp 66: slot=-1, alloc=-1
; temp 67: slot=-1, alloc=-1
; temp 68: slot=-1, alloc=-1
; temp 69: slot=3, alloc=-1
; temp 70: slot=-1, alloc=-1
; temp 71: slot=4, alloc=-1
; temp 72: slot=-1, alloc=-1
; temp 73: slot=5, alloc=-1
; temp 74: slot=6, alloc=-1
; temp 75: slot=-1, alloc=-1
; temp 76: slot=-1, alloc=-1
; temp 77: slot=-1, alloc=-1
; temp 78: slot=2, alloc=-1
; temp 79: slot=-1, alloc=-1
; temp 80: slot=1, alloc=-1
; temp 81: slot=-1, alloc=-1
.SECTION ".text.load_tile" SUPERFREE
load_tile:
	php
	rep #$30
	tsa
	sec
	sbc.w #16
	tas
@start.1:
	jmp @body.2
@body.2:
	lda.w #128
	sep #$20
	sta.l $002115
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002116
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002117
	rep #$20
	lda.w #0
	sta 4,s
	jmp @for_cond.3
@for_cond.3:
	lda 4,s
	cmp.w #8
	bcc +
	lda.w #0
	bra ++
+	lda.w #1
++
	sta 8,s
	lda 8,s
	bne +
	jmp @for_join.6
+
	jmp @for_body.4
@for_body.4:
	lda 4,s
	sta 10,s
	lda 10,s
	clc
	adc.w #tile_data
	sta 12,s
	lda 12,s
	tax
	sep #$20
	lda.l $0000,x
	rep #$20
	and.w #$00FF
	sta 14,s
	lda 14,s
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	jmp @for_cont.5
@for_cont.5:
	lda 4,s
	clc
	adc.w #1
	sta 6,s
	lda 6,s
	sta 4,s
	jmp @for_cond.3
@for_join.6:
	tsa
	clc
	adc.w #16
	tas
	plp
	rtl
.ENDS


; Function: setup_palette (framesize=2, slots=0, alloc_slots=0)
; temp 64: slot=-1, alloc=-1
; temp 65: slot=-1, alloc=-1
; temp 66: slot=-1, alloc=-1
; temp 67: slot=-1, alloc=-1
; temp 68: slot=-1, alloc=-1
.SECTION ".text.setup_palette" SUPERFREE
setup_palette:
	php
	rep #$30
@start.7:
	jmp @body.8
@body.8:
	lda.w #0
	sep #$20
	sta.l $002121
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002122
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002122
	rep #$20
	lda.w #255
	sep #$20
	sta.l $002122
	rep #$20
	lda.w #127
	sep #$20
	sta.l $002122
	rep #$20
	plp
	rtl
.ENDS


; Function: fill_tilemap (framesize=10, slots=4, alloc_slots=1)
; temp 64: slot=-1, alloc=0
; temp 65: slot=-1, alloc=-1
; temp 66: slot=-1, alloc=-1
; temp 67: slot=-1, alloc=-1
; temp 68: slot=-1, alloc=-1
; temp 69: slot=3, alloc=-1
; temp 70: slot=-1, alloc=-1
; temp 71: slot=-1, alloc=-1
; temp 72: slot=-1, alloc=-1
; temp 73: slot=2, alloc=-1
; temp 74: slot=-1, alloc=-1
; temp 75: slot=1, alloc=-1
; temp 76: slot=-1, alloc=-1
.SECTION ".text.fill_tilemap" SUPERFREE
fill_tilemap:
	php
	rep #$30
	tsa
	sec
	sbc.w #10
	tas
@start.9:
	jmp @body.10
@body.10:
	lda.w #128
	sep #$20
	sta.l $002115
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002116
	rep #$20
	lda.w #8
	sep #$20
	sta.l $002117
	rep #$20
	lda.w #0
	sta 4,s
	jmp @for_cond.11
@for_cond.11:
	lda 4,s
	cmp.w #1024
	bcc +
	lda.w #0
	bra ++
+	lda.w #1
++
	sta 8,s
	lda 8,s
	bne +
	jmp @for_join.14
+
	jmp @for_body.12
@for_body.12:
	lda.w #0
	sep #$20
	sta.l $002118
	rep #$20
	lda.w #0
	sep #$20
	sta.l $002119
	rep #$20
	jmp @for_cont.13
@for_cont.13:
	lda 4,s
	clc
	adc.w #1
	sta 6,s
	lda 6,s
	sta 4,s
	jmp @for_cond.11
@for_join.14:
	tsa
	clc
	adc.w #10
	tas
	plp
	rtl
.ENDS


; Function: main (framesize=2, slots=0, alloc_slots=0)
; temp 64: slot=-1, alloc=-1
; temp 65: slot=-1, alloc=-1
; temp 66: slot=-1, alloc=-1
; temp 67: slot=-1, alloc=-1
; temp 68: slot=-1, alloc=-1
.SECTION ".text.main" SUPERFREE
main:
	php
	rep #$30
@start.15:
	jmp @body.16
@body.16:
	lda.w #0
	sep #$20
	sta.l $002105
	rep #$20
	lda.w #8
	sep #$20
	sta.l $002107
	rep #$20
	lda.w #0
	sep #$20
	sta.l $00210B
	rep #$20
	jsl load_tile
	jsl setup_palette
	jsl fill_tilemap
	lda.w #1
	sep #$20
	sta.l $00212C
	rep #$20
	lda.w #15
	sep #$20
	sta.l $002100
	rep #$20
	jmp @while_cond.17
@while_cond.17:
	jmp @while_body.18
@while_body.18:
	jmp @while_cond.17
.ENDS


; End of generated code
