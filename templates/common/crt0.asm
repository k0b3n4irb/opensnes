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
    frame_count     dsb 2   ; Frame counter (incremented by NMI handler)
    frame_count_svg dsb 2   ; Saved frame count
    lag_frame_counter dsb 2 ; Lag frame detection
.ENDS

;------------------------------------------------------------------------------
; OAM Shadow Buffer
;------------------------------------------------------------------------------
; OAM is 544 bytes: 512 for main table (128 sprites × 4 bytes) + 32 for extra
; We use a RAM buffer and DMA transfer during VBlank for reliable updates.
;------------------------------------------------------------------------------

.RAMSECTION ".oam_buffer" BANK $7E SLOT 1 ORGA $0300 FORCE
    oamMemory       dsb 544 ; Shadow buffer for OAM ($7E:0300-$7E:051F)
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

    ; Clear frame counters (16-bit)
    rep #$20
    stz frame_count
    stz frame_count_svg
    stz lag_frame_counter
    sep #$20

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

    ; Increment frame counter
    rep #$20
    inc frame_count
    sep #$20

    ; Transfer OAM buffer to hardware during VBlank
    lda oam_update_flag
    beq +
    stz oam_update_flag
    jsl oamUpdate
+

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
; When USE_LIB=1: oamUpdate and oam_* functions are provided by the library.
; When USE_LIB=0: Include oam_helpers.asm in your project's ASMSRC.
;
; The NMI handler calls oamUpdate if oam_update_flag is set.
; This function must be provided by either the library or oam_helpers.asm.
;==============================================================================

;==============================================================================
; External References
;==============================================================================
; Note: main() is defined in the compiled C code that follows this file.
; oamUpdate is provided by either the library or oam_helpers.asm.
