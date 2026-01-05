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
    ; Use Direct Page,X (stz 0,x) which always accesses bank 0
    ldx #$2105
-   stz 0,x
    inx
    cpx #$210D
    bne -

    ; Set BG1 tilemap at VRAM $0800 (word address)
    ; $2107 = (base >> 9) << 2 = ($0800 >> 9) << 2 = $08
    lda #$08
    sta $2107

    ; Clear scroll registers ($210D-$2114) - write twice each
    ldx #$210D
-   stz 0,x
    stz 0,x
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
-   stz 0,x
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
-   stz 0,x
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

    ; Set up stack FIRST (before any stack operations!)
    rep #$10            ; 16-bit X/Y
    sep #$20            ; 8-bit A
    ldx #$1FFF
    txs

    ; Now we can safely use stack operations
    ; DBR is undefined after reset, but we don't need to set it
    ; because we'll use short addressing which works with DBR=$00
    ; (65816 initializes DBR to $00 after reset in most implementations)

    ; Initialize hardware
    jsr InitHardware

    ; Switch to 16-bit mode
    rep #$30

    ; Set Direct Page to compiler registers
    lda #tcc__r0
    tcd

    ; NOTE: DBR stays at $00 (default after reset)
    ; Setting DBR to $7E causes issues with C code VRAM writes
    ; because absolute addressing (sta $xxxx) uses DBR for the bank
    ; pea $7E7E
    ; plb
    ; plb

    ; Clear compiler registers
    stz tcc__r0
    stz tcc__r1

    ;==========================================================================
    ; Complete graphics setup in assembly (bypass C code issues)
    ; This is identical to hello_asm.asm which works perfectly
    ;==========================================================================
    rep #$10        ; 16-bit X/Y
    sep #$20        ; 8-bit A

    ; VMAIN for word increment after high byte
    lda #$80
    sta $2115

    ; Write font tiles to VRAM $0000
    stz $2116       ; VMADDL = 0
    stz $2117       ; VMADDH = 0

    ; Tile 0: D
    lda #$7C
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$7C
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 1: E
    lda #$7E
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$7C
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$7E
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 2: H
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$7E
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 3: L
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$40
    sta $2118
    stz $2119
    lda #$7E
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 4: O
    lda #$3C
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$3C
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 5: R
    lda #$7C
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$7C
    sta $2118
    stz $2119
    lda #$48
    sta $2118
    stz $2119
    lda #$44
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 6: W
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    lda #$5A
    sta $2118
    stz $2119
    lda #$66
    sta $2118
    stz $2119
    lda #$42
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 7: !
    lda #$18
    sta $2118
    stz $2119
    lda #$18
    sta $2118
    stz $2119
    lda #$18
    sta $2118
    stz $2119
    lda #$18
    sta $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    lda #$18
    sta $2118
    stz $2119
    stz $2118
    stz $2119

    ; Tile 8: Space (empty)
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119
    stz $2118
    stz $2119

    ; Clear tilemap at $0800 with tile 8 (space)
    stz $2116       ; VMADDL = 0
    lda #$08
    sta $2117       ; VMADDH = 8 -> word address $0800

    ldx #1024
_clear_tilemap:
    lda #8          ; Tile 8 = space
    sta $2118
    stz $2119       ; Attributes = 0
    dex
    bne _clear_tilemap

    ; Write "HELLO WORLD!" at row 14, column 10
    ; Tilemap address = $0800 + (14*32) + 10 = $0800 + $1CA = $09CA
    lda #$CA
    sta $2116       ; VMADDL = $CA
    lda #$09
    sta $2117       ; VMADDH = $09

    ; H=2, E=1, L=3, L=3, O=4, space=8, W=6, O=4, R=5, L=3, D=0, !=7
    lda #2
    sta $2118
    stz $2119       ; H
    lda #1
    sta $2118
    stz $2119       ; E
    lda #3
    sta $2118
    stz $2119       ; L
    lda #3
    sta $2118
    stz $2119       ; L
    lda #4
    sta $2118
    stz $2119       ; O
    lda #8
    sta $2118
    stz $2119       ; (space)
    lda #6
    sta $2118
    stz $2119       ; W
    lda #4
    sta $2118
    stz $2119       ; O
    lda #5
    sta $2118
    stz $2119       ; R
    lda #3
    sta $2118
    stz $2119       ; L
    lda #0
    sta $2118
    stz $2119       ; D
    lda #7
    sta $2118
    stz $2119       ; !

    ; Set palette
    stz $2121       ; CGADD = 0

    ; Color 0: Dark blue
    stz $2122       ; Low byte
    lda #$28
    sta $2122       ; High byte

    ; Color 1: White
    lda #$FF
    sta $2122
    lda #$7F
    sta $2122

    ; Enable BG1 on main screen
    lda #$01
    sta $212C

    ; Screen ON (full brightness)
    lda #$0F
    sta $2100

    ; Call main() - C code entry point
    rep #$30
    jsl main

    ; main() returned - halt
_halt:
    bra _halt

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
