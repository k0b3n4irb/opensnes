;==============================================================================
; OpenSNES CRT0 - SNES Startup Code
;==============================================================================
;
; This file contains the startup code that runs before main().
; It initializes the SNES hardware and C runtime environment.
;
; Execution flow:
;   1. Reset vector jumps to _start
;   2. Switch to native mode, set up stack
;   3. Initialize PPU and CPU registers
;   4. Copy .data section to RAM
;   5. Clear .bss section
;   6. Call consoleInit()
;   7. Call main()
;
; Attribution:
;   Based on PVSnesLib crt0_snes.asm by Alekmaul
;   License: zlib (compatible with MIT)
;   Modifications:
;     - Reorganized for clarity
;     - Added comprehensive documentation
;     - Simplified register naming
;
; Author: OpenSNES Team
; License: MIT
;
;==============================================================================

;------------------------------------------------------------------------------
; Compiler Imaginary Registers
;------------------------------------------------------------------------------
; 816-tcc uses "imaginary registers" for intermediate values.
; These MUST be at address $00:0000 for proper NMI handling.
;------------------------------------------------------------------------------

.RAMSECTION ".registers" BANK 0 SLOT 1 ORGA 0 FORCE
    ; General purpose registers (16-bit + 16-bit high word)
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

    ; NMI handler pointer (set by consoleInit)
    nmi_handler dsb 4

    ; memcpy/memmove instructions (self-modifying code)
    move_insn           dsb 4   ; MVN + RTS
    move_backwards_insn dsb 4   ; MVP + RTS
.ENDS

;------------------------------------------------------------------------------
; System Variables
;------------------------------------------------------------------------------

.RAMSECTION ".system_vars" BANK 0 SLOT 1
    ; VBlank synchronization
    vblank_flag         dsb 1   ; Set by NMI, cleared by WaitForVBlank

    ; Frame counters
    frame_count         dsb 2   ; Frames since boot (wraps at 65535)
    frame_count_svg     dsb 2   ; Saved frame count

    ; Lag frame detection
    lag_frame_counter   dsb 2
.ENDS

;------------------------------------------------------------------------------
; RAM Sections
;------------------------------------------------------------------------------

; Initialized data section (copied from ROM)
.RAMSECTION ".data" BANK $7E SLOT 2
.ENDS

; Uninitialized data section (zeroed at startup)
.RAMSECTION ".bss" BANK $7E SLOT 2
.ENDS

;------------------------------------------------------------------------------
; ROM Configuration
;------------------------------------------------------------------------------

.BANK 0
.ORG 0

.EMPTYFILL $00      ; Fill unused space with BRK opcode

;------------------------------------------------------------------------------
; Empty Handlers
;------------------------------------------------------------------------------

.SECTION ".vectors" SEMIFREE

; Empty interrupt handler (just return)
_EmptyHandler:
    rti

; Empty NMI (long return for indirect calls)
_EmptyNMI:
    rtl

.ENDS

;==============================================================================
; Hardware Initialization
;==============================================================================

.SECTION ".init" SEMIFREE

;------------------------------------------------------------------------------
; _snes_init - Initialize SNES Hardware Registers
;------------------------------------------------------------------------------
; Sets all PPU and CPU registers to known safe values.
; Screen is blanked (brightness 0, force blank on).
;
; Input:  None
; Output: None
; Clobbers: A, X
;------------------------------------------------------------------------------
_snes_init:
    rep #$10            ; X/Y = 16-bit
    sep #$20            ; A = 8-bit

    ;--- PPU Registers ---

    ; $2100: Display control - screen off, full brightness
    lda #$8F
    sta $2100

    ; $2101-$2104: Sprite registers - clear
    stz $2101           ; Sprite size/address
    stz $2102           ; OAM address low
    stz $2103           ; OAM address high

    ; $2105-$210C: BG mode and tilemap addresses - clear
    ldx #$2105
-   stz.w $0000,x
    inx
    cpx #$210D
    bne -

    ; $210D-$2114: BG scroll registers - clear (write twice each)
    ldx #$210D
-   stz.w $0000,x
    stz.w $0000,x         ; Second write for high bits
    inx
    cpx #$2115
    bne -

    ; $2115: VRAM address increment - increment on high byte write
    lda #$80
    sta $2115

    ; $2116-$2117: VRAM address - clear
    stz $2116
    stz $2117

    ; $211A: Mode 7 settings - clear
    stz $211A

    ; $211B: Mode 7 matrix A - set to 1.0 (identity)
    stz $211B
    lda #$01
    sta $211B

    ; $211C-$211D: Mode 7 matrix B, C - clear
    ldx #$211C
-   stz.w $0000,x
    stz.w $0000,x
    inx
    cpx #$211E
    bne -

    ; $211E: Mode 7 matrix D - set to 1.0 (identity)
    stz $211E
    sta $211E           ; A still = 1

    ; $211F-$2120: Mode 7 center - clear
    stz $211F
    stz $211F
    stz $2120
    stz $2120

    ; $2121: CGRAM address - clear
    stz $2121

    ; $2123-$212B: Window settings - clear
    ldx #$2123
-   stz.w $0000,x
    inx
    cpx #$212C
    bne -

    ; $212C: Main screen - enable BG1
    lda #$01
    sta $212C

    ; $212D-$212F: Sub screen and window masks - clear
    stz $212D
    stz $212E
    stz $212F

    ; $2130-$2132: Color math
    lda #$30
    sta $2130           ; Color math control
    stz $2131           ; Color math designation
    lda #$E0
    sta $2132           ; Fixed color (black)

    ; $2133: Screen settings - clear
    stz $2133

    ;--- CPU Registers ---

    ; $4200: Interrupt enable - disable all
    stz $4200

    ; $4201: I/O port - all bits high
    lda #$FF
    sta $4201

    ; $4202-$420D: Math and DMA registers - clear
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
; _start - Program Entry Point (Reset Vector)
;------------------------------------------------------------------------------
; This is where execution begins after reset.
;------------------------------------------------------------------------------
_start:
    ; Disable interrupts during initialization
    sei

    ; Switch to 65816 native mode
    clc                 ; Clear carry
    xce                 ; Exchange carry and emulation bit

    ; Set processor mode: 16-bit index, 8-bit accumulator
    rep #$18            ; Clear decimal mode, 16-bit X/Y
    sep #$20            ; 8-bit A

    ; Set up stack at $1FFF
    ldx #$1FFF
    txs

    ; Initialize hardware registers
    jsr _snes_init

    ; Clear VBlank flag
    stz vblank_flag

    ; Switch to 16-bit mode for rest of init
    rep #$30            ; 16-bit A, X, Y

    ; Set Direct Page to imaginary registers
    lda #tcc__r0
    tcd

    ; Set up empty NMI handler
    lda #_EmptyNMI
    sta nmi_handler
    lda #:_EmptyNMI
    sta nmi_handler + 2

    ; Copy .data section from ROM to RAM
    ldx #0
-   lda.l SECTIONSTART_.data_rom,x
    sta.l SECTIONSTART_.data,x
    inx
    inx
    cpx #SECTIONEND_.data_rom - SECTIONSTART_.data_rom
    bcc -

    ; Set Data Bank to $7E (Work RAM)
    pea $7E7E
    plb
    plb

    ; Clear .bss section
    ldx #SECTIONEND_.bss - SECTIONSTART_.bss
    beq +               ; Skip if empty
-   dex
    dex
    stz SECTIONSTART_.bss,x
    bne -
+

    ; Set up memcpy instruction (MVN)
    lda #$0054          ; MVN opcode + bank byte
    sta move_insn
    lda #$6000          ; Bank byte + RTS
    sta move_insn + 2

    ; Set up memmove instruction (MVP)
    lda #$0044          ; MVP opcode + bank byte
    sta move_backwards_insn
    lda #$6000          ; Bank byte + RTS
    sta move_backwards_insn + 2

    ; Clear registers
    stz tcc__r0
    stz tcc__r1

    ; Clear frame counters
    stz frame_count
    stz frame_count_svg
    stz lag_frame_counter

    ; Reset Data Bank to $00 for hardware register access
    ; (was set to $7E for .bss clearing above)
    pea $0000
    plb
    plb

    ; Initialize console (sets up VBlank handler, etc.)
    jsl consoleInit

    ; Call main() - use JSL since QBE generates RTL returns
    jsl main

    ; Main returned - halt
    ; Store return value at $FFFD for test runners
    ; QBE returns value in A register (already there after jsl main returns)
    sep #$20
    sta $FFFD
    rep #$20

    ; Stop processor
    stp

.ENDS

;==============================================================================
; VBlank (NMI) Handler
;==============================================================================

.SECTION ".nmi" SEMIFREE

;------------------------------------------------------------------------------
; _nmi_handler - VBlank Interrupt Handler
;------------------------------------------------------------------------------
; Called automatically at the start of each VBlank period.
; Sets the vblank_flag and calls the user NMI handler.
;------------------------------------------------------------------------------
_nmi_handler:
    ; Save registers
    rep #$30
    pha
    phx
    phy
    phd
    phb

    ; Set direct page to our registers
    lda #tcc__r0
    tcd

    ; Set data bank to $00
    pea $0000
    plb
    plb

    ; Acknowledge NMI by reading $4210
    sep #$20
    lda $4210

    ; Set VBlank flag
    lda #$01
    sta vblank_flag

    ; Increment frame counter
    rep #$20
    inc frame_count

    ; Call user NMI handler (if set)
    jml [nmi_handler]

_nmi_return:
    ; Restore registers
    rep #$30
    plb
    pld
    ply
    plx
    pla

    rti

.ENDS

;==============================================================================
; External References
;==============================================================================

.SECTION ".externs" SEMIFREE

; These are defined in C code
.GLOBAL main
.GLOBAL consoleInit

.ENDS
