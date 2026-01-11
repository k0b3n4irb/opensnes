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
.include "project_hdr.asm"

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
    oam_update_flag dsb 1   ; Set when OAM buffer needs transfer
.ENDS

;------------------------------------------------------------------------------
; OAM Shadow Buffer
;------------------------------------------------------------------------------
; OAM is 544 bytes: 512 for main table (128 sprites × 4 bytes) + 32 for extra
; We use a RAM buffer and DMA transfer during VBlank for reliable updates.
;------------------------------------------------------------------------------

.RAMSECTION ".oam_buffer" BANK 0 SLOT 1
    oamMemory       dsb 544 ; Shadow buffer for OAM
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

    ; Main screen - all layers disabled (examples enable what they need)
    stz $212C

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
-   stz.w $0000,x
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

    ; Clear system flags
    stz vblank_flag
    stz oam_update_flag

    ; Switch to 16-bit mode
    rep #$30

    ; Set Direct Page to compiler registers
    lda #tcc__r0
    tcd

    ; Set Data Bank to $7E (Work RAM) for variable access
    pea $7E7E
    plb
    plb

    ; Clear compiler registers
    stz tcc__r0
    stz tcc__r1

    ; Reset Data Bank to $00 for hardware register access
    pea $0000
    plb
    plb

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
; Transfers OAM buffer to hardware if needed, then sets vblank_flag.
;------------------------------------------------------------------------------
NmiHandler:
    rep #$30
    pha
    phx
    phy
    phd
    phb                 ; Save data bank

    ; Set direct page
    lda #tcc__r0
    tcd

    ; Set data bank to $00 for hardware registers
    pea $0000
    plb
    plb

    ; Acknowledge NMI
    sep #$20
    lda $4210

    ; OAM buffer transfer disabled - examples use direct OAM writes
    ; If using the buffer system, uncomment below:
    ; lda oam_update_flag
    ; beq +
    ; stz oam_update_flag
    ; jsr oamUpdate
    ;+

    ; Set VBlank flag
    lda #$01
    sta vblank_flag

    ; Restore and return
    rep #$30
    plb                 ; Restore data bank
    pld
    ply
    plx
    pla
    rti

.ENDS

;==============================================================================
; IRQ Handler
;==============================================================================

.SECTION ".irq" SEMIFREE

;------------------------------------------------------------------------------
; IrqHandler - Hardware IRQ (H/V counter, etc.)
;------------------------------------------------------------------------------
IrqHandler:
    rti                     ; Just return for now

.ENDS

;==============================================================================
; OAM Helper Functions
;==============================================================================
; These functions write to the OAM shadow buffer in RAM.
; The NMI handler transfers the buffer to OAM via DMA.
;==============================================================================

.SECTION ".oam_helpers" SEMIFREE

;------------------------------------------------------------------------------
; oam_set_tile - Set sprite 0's tile number in buffer
; void oam_set_tile(u8 tile)
; Stack: [P:1] [retaddr:3] [tile:2] -> tile at offset 5
;------------------------------------------------------------------------------
oam_set_tile:
    php
    sep #$20            ; 8-bit A
    lda 5,s             ; Get tile param
    sta oamMemory+2     ; Write to buffer byte 2 (tile)
    lda #$01
    sta oam_update_flag ; Mark buffer as needing update
    plp
    rtl

;------------------------------------------------------------------------------
; oam_init_sprite0 - Initialize sprite 0 with full attributes
; void oam_init_sprite0(u8 x, u8 y, u8 tile, u8 attr)
; Stack: [P:1] [retaddr:3] [attr:2] [tile:2] [y:2] [x:2]
;        attr at 5, tile at 7, y at 9, x at 11
; Note: With OBJSEL=0x60, small=16x16, large=32x32. We use small size.
;------------------------------------------------------------------------------
oam_init_sprite0:
    php
    sep #$20            ; 8-bit A
    lda 11,s            ; Get X
    sta oamMemory+0
    lda 9,s             ; Get Y
    sta oamMemory+1
    lda 7,s             ; Get tile
    sta oamMemory+2
    lda 5,s             ; Get attr (priority, palette, etc)
    sta oamMemory+3
    ; High table byte 0: sprite 0 uses bits 0-1
    ; Bit 0 = X high bit (keep 0 for X < 256)
    ; Bit 1 = size (0=small=16x16, 1=large=32x32)
    stz oamMemory+512   ; Small size, X bit 8 = 0
    lda #$01
    sta oam_update_flag
    plp
    rtl

;------------------------------------------------------------------------------
; oam_hide_all - Hide all 128 sprites (set Y=240 for each)
; Call this during initialization
;------------------------------------------------------------------------------
oam_hide_all:
    php
    rep #$10            ; 16-bit X/Y
    sep #$20            ; 8-bit A
    ldx #0
-   stz oamMemory,x     ; X = 0
    lda #240
    sta oamMemory+1,x   ; Y = 240 (off screen)
    stz oamMemory+2,x   ; Tile = 0
    stz oamMemory+3,x   ; Attr = 0
    inx
    inx
    inx
    inx
    cpx #512            ; 128 sprites * 4 bytes
    bne -
    ; Clear high table (32 bytes)
    ldx #512
-   stz oamMemory,x
    inx
    cpx #544
    bne -
    lda #$01
    sta oam_update_flag
    plp
    rtl

;------------------------------------------------------------------------------
; oam_set_pos - Set sprite 0's X and Y position in buffer
; void oam_set_pos(u8 x, u8 y)
; Stack: [P:1] [retaddr:3] [y:2] [x:2] -> x at 7, y at 5
;------------------------------------------------------------------------------
oam_set_pos:
    php
    sep #$20            ; 8-bit A
    lda 7,s             ; Get X param
    sta oamMemory+0     ; Write to buffer byte 0 (X)
    lda 5,s             ; Get Y param
    sta oamMemory+1     ; Write to buffer byte 1 (Y)
    lda #$01
    sta oam_update_flag ; Mark buffer as needing update
    plp
    rtl

;------------------------------------------------------------------------------
; oam_set_attr - Set sprite 0's attribute byte
; void oam_set_attr(u8 attr)
; Attribute byte format: vhoopppc
;   v=vflip, h=hflip, oo=priority, ppp=palette, c=tile high bit
; Stack: [P:1] [retaddr:3] [attr:2] -> attr at 5
;------------------------------------------------------------------------------
oam_set_attr:
    php
    sep #$20            ; 8-bit A
    lda 5,s             ; Get attr param
    sta oamMemory+3     ; Write to buffer byte 3 (attr)
    lda #$01
    sta oam_update_flag
    plp
    rtl

;------------------------------------------------------------------------------
; oamUpdate - DMA transfer OAM buffer to hardware
; Called during VBlank from NMI handler
;------------------------------------------------------------------------------
oamUpdate:
    php
    rep #$20            ; 16-bit A

    lda #$0000
    sta $2102           ; OAM address = 0

    lda #$0400          ; DMA mode: CPU->PPU, auto-increment, target $2104
    sta $4300

    lda #544            ; Transfer size = 544 bytes (full OAM)
    sta $4305

    lda #oamMemory
    sta $4302           ; DMA source address (low word)

    sep #$20            ; 8-bit A
    lda #:oamMemory
    sta $4304           ; DMA source bank

    lda #$01            ; Enable DMA channel 0
    sta $420B

    plp
    rts

.ENDS

;==============================================================================
; External References
;==============================================================================
; Note: main() is defined in the compiled C code that follows this file.
; No .GLOBAL needed since we build as a single combined file.
