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
    ; VBlank callback (must be in direct page for JML [nmi_callback] to work)
    nmi_callback    dsb 4   ; 24-bit function pointer + padding (PVSnesLib compatible)
.ENDS

;------------------------------------------------------------------------------
; NMI Handler Registers (separate from main code to prevent corruption)
;------------------------------------------------------------------------------
; The VBlank callback runs C code which uses tcc__r0-r10 for calculations.
; Using the same registers as the main loop would corrupt its state.
; PVSnesLib solves this with a separate 48-byte register area for NMI.
;------------------------------------------------------------------------------

.RAMSECTION ".nmi_registers" BANK 0 SLOT 1 ORGA $0080 FORCE
    tcc__nmi_r0     dsb 2
    tcc__nmi_r0h    dsb 2
    tcc__nmi_r1     dsb 2
    tcc__nmi_r1h    dsb 2
    tcc__nmi_r2     dsb 2
    tcc__nmi_r2h    dsb 2
    tcc__nmi_r3     dsb 2
    tcc__nmi_r3h    dsb 2
    tcc__nmi_r4     dsb 2
    tcc__nmi_r4h    dsb 2
    tcc__nmi_r5     dsb 2
    tcc__nmi_r5h    dsb 2
    tcc__nmi_r9     dsb 2
    tcc__nmi_r9h    dsb 2
    tcc__nmi_r10    dsb 2
    tcc__nmi_r10h   dsb 2
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
    ; Input state (read in VBlank ISR like PVSnesLib)
    pad_keys        dsb 10  ; Current button state (5 pads × 16 bits)
    pad_keysold     dsb 10  ; Previous frame button state
    pad_keysdown    dsb 10  ; Buttons pressed this frame (edge detection)
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
; Dynamic Sprite Engine RAM
;------------------------------------------------------------------------------
; oambuffer: Game-level sprite state (128 sprites × 16 bytes = 2048 bytes)
; oamQueueEntry: VRAM upload queue (128 entries × 6 bytes = 768 bytes)
; State variables for sprite management
;------------------------------------------------------------------------------

.RAMSECTION ".dynamic_sprite_buffer" BANK $7E SLOT 1 ORGA $0520 FORCE
    oambuffer       dsb 2048    ; 128 × 16 bytes ($7E:0520-$7E:0D1F)
.ENDS

.RAMSECTION ".dynamic_sprite_queue" BANK $7E SLOT 1 ORGA $0D20 FORCE
    oamQueueEntry   dsb 768     ; 128 × 6 bytes ($7E:0D20-$7E:101F)
.ENDS

.RAMSECTION ".dynamic_sprite_state" BANK $7E SLOT 1 ORGA $1020 FORCE
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

    ; Color math - disabled (backdrop color comes from CGRAM[0])
    stz $2130           ; CGWSEL: no clipping, no color math prevention
    stz $2131           ; CGADSUB: no color addition/subtraction
    stz $2132           ; COLDATA: leave at power-on default (not used)

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

    ; Initialize VBlank callback to default (does nothing)
    rep #$20
    lda #DefaultNmiCallback
    sta nmi_callback
    sep #$20
    lda #:DefaultNmiCallback
    sta nmi_callback+2

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

    ;--------------------------------------------------------------------------
    ; Read Joypads (like PVSnesLib - read in VBlank ISR for reliability)
    ;--------------------------------------------------------------------------
    ; Wait for auto-joypad read to complete
-   lda $4212
    and #$0001
    bne -

    ; Save previous state
    lda pad_keys
    sta pad_keysold
    lda pad_keys+2
    sta pad_keysold+2

    ; Read joypad 1 (16-bit load gets both JOY1L and JOY1H)
    lda $4218           ; 16-bit: A = JOY1H:JOY1L
    bit #$000F          ; Validate: bits 0-3 must be zero for valid joypad
    beq +
        lda #$0000      ; Invalid input - clear it
+   sta pad_keys

    ; Compute edge detection: (current ^ old) & current = newly pressed
    eor pad_keysold
    and pad_keys
    sta pad_keysdown

    ; Read joypad 2
    lda $421A           ; 16-bit: A = JOY2H:JOY2L
    bit #$000F          ; Validate: bits 0-3 must be zero for valid joypad
    beq ++
        lda #$0000      ; Invalid input - clear it
++  sta pad_keys+2

    eor pad_keysold+2
    and pad_keys+2
    sta pad_keysdown+2

    sep #$20

    ; Call user VBlank callback (BEFORE OAM update for max VBlank time)
    ; Always called - default callback does nothing (just RTL)
    ;
    ; IMPORTANT: D must be 0 (tcc__r0) for JML [nmi_callback] to work correctly,
    ; because JML [dp] reads from address D+dp. We save the main loop's registers
    ; to the NMI register area, then restore them after the callback.
    rep #$30            ; 16-bit A/X/Y

    ; Save ALL main loop's compiler registers to NMI area (prevent corruption)
    ; The C callback may use any of these registers
    lda tcc__r0
    sta tcc__nmi_r0
    lda tcc__r0h
    sta tcc__nmi_r0h
    lda tcc__r1
    sta tcc__nmi_r1
    lda tcc__r1h
    sta tcc__nmi_r1h
    lda tcc__r2
    sta tcc__nmi_r2
    lda tcc__r2h
    sta tcc__nmi_r2h
    lda tcc__r3
    sta tcc__nmi_r3
    lda tcc__r3h
    sta tcc__nmi_r3h
    lda tcc__r4
    sta tcc__nmi_r4
    lda tcc__r4h
    sta tcc__nmi_r4h
    lda tcc__r5
    sta tcc__nmi_r5
    lda tcc__r5h
    sta tcc__nmi_r5h
    lda tcc__r9
    sta tcc__nmi_r9
    lda tcc__r9h
    sta tcc__nmi_r9h
    lda tcc__r10
    sta tcc__nmi_r10
    lda tcc__r10h
    sta tcc__nmi_r10h

    ; Set data bank to $7E for C variable access
    pea $7E7E
    plb
    plb
    ; Push return address - 1 for RTL (bank:addr-1)
    phk                 ; Push current program bank
    pea @callback_done-1
    jml [nmi_callback]  ; Indirect long jump (D=0, reads from address 0+nmi_callback)
@callback_done:

    ; Restore data bank to $00 for hardware register access
    pea $0000
    plb
    plb

    ; Restore ALL main loop's compiler registers from NMI area
    lda tcc__nmi_r0
    sta tcc__r0
    lda tcc__nmi_r0h
    sta tcc__r0h
    lda tcc__nmi_r1
    sta tcc__r1
    lda tcc__nmi_r1h
    sta tcc__r1h
    lda tcc__nmi_r2
    sta tcc__r2
    lda tcc__nmi_r2h
    sta tcc__r2h
    lda tcc__nmi_r3
    sta tcc__r3
    lda tcc__nmi_r3h
    sta tcc__r3h
    lda tcc__nmi_r4
    sta tcc__r4
    lda tcc__nmi_r4h
    sta tcc__r4h
    lda tcc__nmi_r5
    sta tcc__r5
    lda tcc__nmi_r5h
    sta tcc__r5h
    lda tcc__nmi_r9
    sta tcc__r9
    lda tcc__nmi_r9h
    sta tcc__r9h
    lda tcc__nmi_r10
    sta tcc__r10
    lda tcc__nmi_r10h
    sta tcc__r10h

    sep #$20            ; Back to 8-bit A

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

;------------------------------------------------------------------------------
; DefaultNmiCallback - Default VBlank callback (does nothing)
;------------------------------------------------------------------------------
; This is called by NmiHandler when no user callback is registered.
; Simply returns immediately.
;------------------------------------------------------------------------------
DefaultNmiCallback:
    rtl

.ENDS

;==============================================================================
; IRQ Handler
;==============================================================================

.SECTION ".irq" SEMIFREE

;------------------------------------------------------------------------------
; IrqHandler - Hardware IRQ (H/V counter, etc.)
;------------------------------------------------------------------------------
; Called when H/V timer IRQ fires (if enabled via $4200).
; Must read TIMEUP ($4211) to acknowledge the interrupt.
;------------------------------------------------------------------------------
IrqHandler:
    sep #$20            ; 8-bit A
    lda $4211           ; Read TIMEUP to acknowledge IRQ
    rti

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
