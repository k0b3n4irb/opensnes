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

;------------------------------------------------------------------------------
; ASCII Table Definition
;------------------------------------------------------------------------------
; Define identity mapping for .ASC directives (character → same character).
; This suppresses the WLA-DX warning "No .ASCIITABLE defined" and makes
; the intended mapping explicit.
;------------------------------------------------------------------------------
.ASCIITABLE
MAP ' ' TO '~' = ' '    ; Printable ASCII: space (32) to tilde (126)
.ENDA

;==============================================================================
; WLA-DX Section Types Used in This File
;==============================================================================
;
; FORCE    - Section placed at an exact address. The ORGA directive specifies
;            the absolute address. Content MUST fit exactly. Used for hardware
;            requirements like compiler registers at $0000 and OAM buffer at
;            $7E:0300 (avoiding WRAM mirror overlap).
;
; SEMIFREE - Section placed after a specified ORG within the bank, but the
;            linker can move it if needed. Good for startup code that needs
;            to be in bank 0 but doesn't need a specific address.
;
; SUPERFREE - Section can be placed in ANY bank with available space. Used
;             for library code and data that doesn't need specific placement.
;             The linker optimizes placement automatically.
;
; RAMSECTION - Allocates RAM without generating ROM data. Combined with
;              ORGA FORCE for precise placement or without for automatic.
;
; For more details, see .claude/WLA-DX.md
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
    tilemap_update_flag dsb 1 ; Set when tilemap buffer needs DMA to VRAM
    tilemap_vram_addr dsb 2   ; VRAM word address for tilemap DMA target
    tilemap_src_addr dsb 2    ; 16-bit RAM address of tilemap buffer (bank $00)
    frame_count     dsb 2   ; Frame counter (incremented by NMI handler)
    frame_count_svg dsb 2   ; Saved frame count
    lag_frame_counter dsb 2 ; Lag frame detection
    ; Input state (read in VBlank ISR like PVSnesLib)
    pad_keys        dsb 10  ; Current button state (5 pads × 16 bits)
    pad_keysold     dsb 10  ; Previous frame button state
    pad_keysdown    dsb 10  ; Buttons pressed this frame (edge detection)
    bg_scroll_x     dsb 8   ; u16[4] BG1-4 horizontal scroll shadows
    bg_scroll_y     dsb 8   ; u16[4] BG1-4 vertical scroll shadows
    ; Mouse state (read in VBlank ISR when mouse_con != 0)
    mouse_con       dsb 1   ; bitmask: bit 0 = port 1 mouse, bit 1 = port 2 mouse
    mouse_x         dsb 2   ; port 1 X displacement (sign-magnitude raw byte in low)
    mouse_y         dsb 2   ; port 1 Y displacement (sign-magnitude raw byte in low)
    mouse_x2        dsb 2   ; port 2 X displacement
    mouse_y2        dsb 2   ; port 2 Y displacement
    mouse_buttons   dsb 1   ; port 1 buttons (bit 0 = left, bit 1 = right)
    mouse_buttons2  dsb 1   ; port 2 buttons
    mouse_btnsold   dsb 1   ; port 1 previous frame buttons
    mouse_btnsold2  dsb 1   ; port 2 previous frame buttons
    mouse_btnsdown  dsb 1   ; port 1 newly pressed (edge detection)
    mouse_btnsdown2 dsb 1   ; port 2 newly pressed
    mouse_sens      dsb 1   ; port 1 sensitivity (0-2)
    mouse_sens2     dsb 1   ; port 2 sensitivity
.ENDS

;------------------------------------------------------------------------------
; Super Scope State Variables (port 2 only)
;------------------------------------------------------------------------------
; Separate RAMSECTION to avoid inflating ".system" (which must stay small
; enough for the linker to place variables like tilemap_src_addr within
; 8-bit direct page addressing range).
;------------------------------------------------------------------------------

.RAMSECTION ".scope" BANK 0 SLOT 1
    scope_con       dsb 1   ; 1 = Super Scope connected and active
    scope_sinceshot dsb 2   ; frames since last shot (u16)
    scope_shoth     dsb 2   ; H position, calibration-adjusted (u16)
    scope_shotv     dsb 2   ; V position, calibration-adjusted (u16)
    scope_shothraw  dsb 2   ; H position, raw from PPU (u16)
    scope_shotvraw  dsb 2   ; V position, raw from PPU (u16)
    scope_centerh   dsb 2   ; H calibration offset (s16)
    scope_centerv   dsb 2   ; V calibration offset (s16)
    scope_down      dsb 2   ; buttons currently held (u16)
    scope_now       dsb 2   ; buttons newly pressed this frame (u16)
    scope_held      dsb 2   ; buttons held past holddelay (u16)
    scope_last      dsb 2   ; previous frame button state (u16)
    scope_port2down dsb 2   ; raw auto-joypad port 2 (u16)
    scope_port2last dsb 2   ; previous frame raw port 2 (u16)
    scope_port2now  dsb 2   ; newly pressed raw port 2 (u16)
    scope_holddelay dsb 2   ; frames before hold triggers (u16, default 60)
    scope_repdelay  dsb 2   ; frames between repeat fires (u16, default 20)
    scope_tohold    dsb 2   ; countdown to hold (u16)
.ENDS

;------------------------------------------------------------------------------
; Reserved Bank $00 Region (WRAM Mirror Protection)
;------------------------------------------------------------------------------
; Bank $00 addresses $0000-$1FFF mirror Bank $7E's $0000-$1FFF.
; We reserve $0300-$051F in Bank $00 to prevent the linker from placing
; variables there, avoiding collision with Bank $7E's OAM buffer.
; Note: Dynamic sprite buffers ($0520+) are only used by advanced projects
; that typically manage their own memory layout.
;------------------------------------------------------------------------------

.RAMSECTION ".reserved_7e_mirror" BANK 0 SLOT 1 ORGA $0300 FORCE
    _reserved_7e_mirror dsb 544 ; Reserved: mirrors Bank $7E oamMemory
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
;
; oambuffer MUST be in Bank $00 mirror range ($0000-$1FFF) because the C
; compiler generates `sta.l $0000,x` which always writes to bank $00.
; Address $0520 is right after oamMemory ($0300-$051F).
; Assembly code with DB=$7E reads $7E:0520 = same physical RAM (mirror).
;------------------------------------------------------------------------------

; oambuffer (2048 bytes) is defined in sprite_dynamic.asm (BANK 0 SLOT 1)
; so C code can access it via sta.l $0000,x. Only projects using
; LIB_MODULES := ... sprite_dynamic ... allocate it.

.RAMSECTION ".dynamic_sprite_queue" BANK $7E SLOT 2 ORGA $2800 FORCE
    oamQueueEntry   dsb 768     ; 128 × 6 bytes ($7E:2800-$7E:2AFF)
.ENDS

.RAMSECTION ".dynamic_sprite_state" BANK $7E SLOT 2 ORGA $2B00 FORCE
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
; LoROM: Slot 0 starts at $8000, so .ORG 0 = address $8000
; HiROM: Slot 0 starts at $0000 (64KB banks), so .ORG $8000 = address $8000
;        Code must be at $8000+ to be accessible via bank $00 mirror.
;------------------------------------------------------------------------------

.BANK 0
.ifdef HIROM
.ORG $8000              ; HiROM: 64KB bank, place code at $8000+
.else
.ORG 0                  ; LoROM: Slot starts at $8000, so .ORG 0 = $8000
.endif
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

    ;--------------------------------------------------------------------------
    ; Initialize OAM buffer to hide all sprites
    ;--------------------------------------------------------------------------
    ; The NMI handler DMAs oamMemory to hardware OAM every frame.
    ; If not initialized, garbage sprites appear on screen.
    ; Y position = 240 ($F0) hides sprites below visible screen.
    ;--------------------------------------------------------------------------
    ; Set data bank to $7E to access oamMemory with absolute addressing
    pea $7E7E
    plb
    plb

    rep #$20            ; 16-bit A
    ldx #0
    lda #$00F0          ; Little-endian: low byte=$F0 (Y=240), high byte=$00 (tile=0)
@oam_clear_loop:
    stz.w oamMemory,x   ; X position = 0
    sta.w oamMemory+1,x ; Y = 240, tile = 0
    stz.w oamMemory+3,x ; attributes = 0 (only low byte matters)
    inx
    inx
    inx
    inx
    cpx #512            ; 128 sprites × 4 bytes
    bne @oam_clear_loop

    ; Clear OAM extension table (32 bytes)
    ldx #0
@oam_ext_clear:
    stz.w oamMemory+512,x
    inx
    inx
    cpx #32
    bne @oam_ext_clear

    sep #$20            ; Back to 8-bit A

    ; Restore data bank to $00
    pea $0000
    plb
    plb

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

    ; Zero-initialize low WRAM ($0000-$1FFF) — BSS clear
    ; Stack is empty (SP=$1FFF), so safe to zero the entire 8KB range.
    ; InitHardware (next) will overwrite OAM ($0300-$051F) with proper values.
    ; CopyInitData (later) will overwrite initialized statics from ROM.
    pea $0000           ; Set DBR=$00 for bank $00 WRAM access
    plb
    plb
    rep #$20            ; 16-bit A for 2-byte stores
    lda #$0000
    ldx #$2000          ; Clear 8KB: $0000-$1FFF
-   dex
    dex
    sta.w $0000,x
    bne -
    sep #$20            ; Restore 8-bit A for InitHardware

    ; Initialize hardware
    jsr InitHardware

    ; Clear system flags
    stz vblank_flag
    stz oam_update_flag
    stz tilemap_update_flag

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

    ; Initialize static variables (copy init data from ROM to RAM)
    jsr CopyInitData

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
; Initialized Data Copy Routine
;==============================================================================
; Copies initialized static variable data from ROM to RAM at startup.
;
; Init data format (generated by compiler for each initialized static):
;   [target_addr:2][size:2][data:N]
;
; End marker: target_addr = 0
;
; The DataInitStart symbol points to the base .data_init section.
; A placeholder byte precedes the actual init records.
; All .data_init.N sections are appended by WLA-DX using APPENDTO.
;==============================================================================

.SECTION ".copy_init" SEMIFREE

CopyInitData:
    rep #$30            ; 16-bit A and X/Y

    ; Load address of init data start (skip 1-byte placeholder)
    lda #DataInitStart + 1
    sta tcc__r0         ; tcc__r0 = current ROM pointer (low word)

@init_loop:
    ; Read target RAM address
    lda (tcc__r0)
    beq @init_done      ; If target_addr == 0, we're done
    sta tcc__r2         ; tcc__r2 = RAM target address

    ; Read size
    ldy #2
    lda (tcc__r0),y
    beq @next_record    ; If size == 0, skip to next (shouldn't happen)
    sta tcc__r4         ; tcc__r4 = size

    ; Source is at tcc__r0 + 4 (after addr and size fields)
    lda tcc__r0
    clc
    adc #4
    sta tcc__r1         ; tcc__r1 = ROM source address

    ; Copy loop: copy tcc__r4 bytes from [tcc__r1] to [tcc__r2]
    ; IMPORTANT: Use 8-bit A to copy exactly 1 byte per iteration.
    ; 16-bit lda/sta would write 2 bytes per iteration, causing a
    ; 1-byte overrun past the target on the final iteration.
    sep #$20            ; 8-bit A (X/Y remain 16-bit)
    ldy #0
@copy_byte:
    lda (tcc__r1),y
    sta (tcc__r2),y
    iny
    cpy tcc__r4
    bne @copy_byte
    rep #$20            ; restore 16-bit A

@next_record:
    ; Move to next init record: tcc__r0 += 4 + size
    lda tcc__r0
    clc
    adc #4              ; Skip addr and size fields
    adc tcc__r4         ; Skip data bytes
    sta tcc__r0
    bra @init_loop

@init_done:
    rts

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

    ;--------------------------------------------------------------------------
    ; Read Mouse (if connected)
    ;--------------------------------------------------------------------------
    ; Only runs when mouse_con != 0 (set by mouseInit in C code).
    ; For each active port:
    ;   1. Extract buttons + sensitivity from auto-joypad data
    ;   2. Bit-bang 16 bits of displacement via $4016/$4017
    ;   3. Edge-detect buttons
    ;--------------------------------------------------------------------------
    sep #$20                ; 8-bit A for mouse reading
    lda mouse_con
    bne @mouse_start
    jmp @mouse_done         ; Skip if no mouse initialized (jmp for distance)
@mouse_start:

    ; --- Port 1 mouse ---
    bit #$01
    beq @mouse_port2

    ; Save old buttons for edge detection
    lda mouse_buttons
    sta mouse_btnsold

    ; Extract buttons and sensitivity from auto-joypad JOY1L ($4218)
    ; JOY1L bit layout for mouse:
    ;   Bit 7: Right button
    ;   Bit 6: Left button
    ;   Bits 5-4: Sensitivity (00=low, 01=med, 10=high)
    ;   Bits 3-0: Signature (0001)
    lda $4218               ; JOY1L (8-bit read)
    pha                     ; Save for sensitivity extraction
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    sta mouse_buttons       ; bits 1-0 = right, left

    pla                     ; Restore byte
    lsr a
    lsr a
    lsr a
    lsr a
    and #$03
    sta mouse_sens

    ; Edge detection: newly pressed = (cur ^ old) & cur
    lda mouse_buttons
    eor mouse_btnsold
    and mouse_buttons
    sta mouse_btnsdown

    ; Bit-bang 16 bits of displacement from port 1 ($4016)
    ; Uses cascading ROL like PVSnesLib: reads 16 bits through mouse_x→mouse_y.
    ; After 16 iterations: mouse_y = first 8 bits (Y), mouse_x = last 8 bits (X).
    ; Format: sign-magnitude (bit 7 = direction, bits 6-0 = magnitude)
    rep #$20                ; 16-bit A for word operations
    stz mouse_y             ; Clear displacement accumulators
    stz mouse_x

    sep #$20                ; 8-bit A for bit reading
    ldy #16                 ; 16 bits total (8 Y + 8 X)
@mouse1_disp_loop:
    lda $4016               ; Read bit from port 1 (bit 0)
    lsr a                   ; Shift bit 0 into carry
    rol mouse_x             ; Carry → mouse_x bit 0, mouse_x bit 7 → carry
    rol mouse_y             ; Carry → mouse_y bit 0
    nop                     ; Hyperkin compatibility delay (170+ master cycles)
    nop
    dey
    bne @mouse1_disp_loop

@mouse_port2:
    ; --- Port 2 mouse ---
    lda mouse_con
    bit #$02
    beq @mouse_done

    ; Save old buttons
    lda mouse_buttons2
    sta mouse_btnsold2

    ; Extract from auto-joypad JOY2L ($421A)
    lda $421A               ; JOY2L (8-bit read)
    pha
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    sta mouse_buttons2

    pla
    lsr a
    lsr a
    lsr a
    lsr a
    and #$03
    sta mouse_sens2

    ; Edge detection
    lda mouse_buttons2
    eor mouse_btnsold2
    and mouse_buttons2
    sta mouse_btnsdown2

    ; Bit-bang 16 bits of displacement from port 2 ($4017)
    rep #$20
    stz mouse_y2
    stz mouse_x2

    sep #$20
    ldy #16
@mouse2_disp_loop:
    lda $4017
    lsr a
    rol mouse_x2
    rol mouse_y2
    nop
    nop
    dey
    bne @mouse2_disp_loop

@mouse_done:

    ;--------------------------------------------------------------------------
    ; Read Super Scope (if connected)
    ;--------------------------------------------------------------------------
    ; Only runs when scope_con != 0 (set by scopeInit in C code).
    ; Based on Nintendo SHVC Scope BIOS v1.00 (disassembled by Revenant).
    ; Reads PPU H/V latch for shot position, applies calibration offset,
    ; and processes buttons with hold/repeat delay logic.
    ;--------------------------------------------------------------------------
    sep #$20                ; 8-bit A
    lda scope_con
    bne @scope_start
    jmp @scope_done         ; Skip if no Super Scope initialized
@scope_start:

    ; Read port 2 raw data from auto-joypad and compute edge detection
    rep #$20                ; 16-bit A
    lda scope_port2down
    sta scope_port2last     ; Save previous frame
    lda $421A               ; REG_JOY2L/H (16-bit read)
    sta scope_port2down
    eor scope_port2last
    and scope_port2down
    sta scope_port2now      ; Newly pressed raw port 2

    ; Validate Super Scope signature: bits 0-7 all 1, bits 10-11 both 0
    lda scope_port2down
    and #$0CFF
    cmp #$00FF
    beq @scope_valid
    jmp @scope_disconnect
@scope_valid:

    ; Check PPU H/V counter latch (bit 6 of REG_STAT78)
    sep #$20                ; 8-bit A
    lda $213F               ; REG_STAT78
    and #$40
    beq @scope_no_shot

    ; --- Shot detected: read H/V position ---
    ; REG_OPHCT ($213C): 9-bit H position, read twice (low then high)
    lda $213C               ; OPHCT low byte
    sta scope_shothraw
    lda $213C               ; OPHCT high bit (bit 0 only)
    and #$01
    sta scope_shothraw+1

    ; REG_OPVCT ($213D): 9-bit V position, read twice (low then high)
    lda $213D               ; OPVCT low byte
    sta scope_shotvraw
    lda $213D               ; OPVCT high bit (bit 0 only)
    and #$01
    sta scope_shotvraw+1

    ; Apply calibration offset: adjusted = raw + center offset
    rep #$20                ; 16-bit A
    lda scope_shothraw
    clc
    adc scope_centerh
    sta scope_shoth
    lda scope_shotvraw
    clc
    adc scope_centerv
    sta scope_shotv

    ; Reset shot counter
    stz scope_sinceshot
    bra @scope_buttons      ; Common button processing

@scope_no_shot:
    ; --- No shot this frame ---
    rep #$20                ; 16-bit A
    inc scope_sinceshot

@scope_buttons:
    ; Full button processing: extract bits 15-12 (buttons) and 9-8 (flags)
    ; All buttons (Fire, Cursor, Turbo, Pause) + flags (Offscreen, Noise)
    ; are processed identically on both shot and non-shot frames.
    lda scope_port2down
    and #$F300              ; Keep buttons (15-12) + offscreen (9) + noise (8)
    sta scope_down

    ; Edge detection: newly pressed = (cur ^ last) & cur
    lda scope_last
    eor scope_down
    and scope_down
    sta scope_now

    ; Hold/repeat delay logic
    ; If buttons changed from last frame, reset hold countdown
    lda scope_down
    cmp scope_last
    bne @scope_reset_hold

    ; Same buttons held: decrement countdown
    lda scope_tohold
    beq @scope_held_fire     ; Already at 0 → fire held event
    dec a
    sta scope_tohold
    stz scope_held
    bra @scope_save_last

@scope_held_fire:
    ; Hold triggered: set held = down, reload with repeat delay
    lda scope_down
    sta scope_held
    lda scope_repdelay
    sta scope_tohold
    bra @scope_save_last

@scope_reset_hold:
    ; Buttons changed: reload with hold delay
    lda scope_holddelay
    sta scope_tohold
    stz scope_held
    bra @scope_save_last

@scope_disconnect:
    ; Invalid signature: disconnect Super Scope
    rep #$20
    stz scope_down
    stz scope_now
    stz scope_held
    stz scope_shoth
    stz scope_shotv
    stz scope_shothraw
    stz scope_shotvraw
    stz scope_sinceshot
    sep #$20
    stz scope_con
    bra @scope_done

@scope_save_last:
    ; Save current button state for next frame
    lda scope_down
    sta scope_last

@scope_done:

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
    sep #$20            ; C functions return in 16-bit A, restore 8-bit
+

    ; Transfer tilemap buffer to VRAM during VBlank
    lda tilemap_update_flag
    beq +
    stz tilemap_update_flag
    jsl tilemapFlush
+

    ; Sync BG scroll shadows to hardware ($210D-$2114)
    ; 8-bit A, data bank $00 (WRAM + hardware registers accessible)
    ; Cost: ~128 master cycles (~0.5% of VBlank)
    lda bg_scroll_x      ; BG1 H low
    sta $210D
    lda bg_scroll_x+1    ; BG1 H high
    sta $210D
    lda bg_scroll_y      ; BG1 V low
    sta $210E
    lda bg_scroll_y+1    ; BG1 V high
    sta $210E
    lda bg_scroll_x+2    ; BG2 H low
    sta $210F
    lda bg_scroll_x+3    ; BG2 H high
    sta $210F
    lda bg_scroll_y+2    ; BG2 V low
    sta $2110
    lda bg_scroll_y+3    ; BG2 V high
    sta $2110
    lda bg_scroll_x+4    ; BG3 H low
    sta $2111
    lda bg_scroll_x+5    ; BG3 H high
    sta $2111
    lda bg_scroll_y+4    ; BG3 V low
    sta $2112
    lda bg_scroll_y+5    ; BG3 V high
    sta $2112
    lda bg_scroll_x+6    ; BG4 H low
    sta $2113
    lda bg_scroll_x+7    ; BG4 H high
    sta $2113
    lda bg_scroll_y+6    ; BG4 V low
    sta $2114
    lda bg_scroll_y+7    ; BG4 V high
    sta $2114

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
; Tilemap Flush (DMA tilemap buffer to VRAM)
;==============================================================================
; Called during VBlank by NMI handler when tilemap_update_flag is set.
; DMA transfers 2048 bytes from tilemapBuffer ($7E:3000) to VRAM.
; Uses DMA channel 1 to avoid conflicting with OAM on channel 0/7.
;==============================================================================

.SECTION ".tilemap_flush" SEMIFREE

tilemapFlush:
    php
    phb                 ; Save current data bank

    ; Set DBR to $00 for hardware register access
    pea $0000
    plb
    plb                 ; DBR = $00

    sep #$20            ; 8-bit A
    lda #$80
    sta $2115           ; VMAIN: increment after high byte write

    rep #$20            ; 16-bit A
    lda tilemap_vram_addr
    sta $2116           ; VMADDL/H: VRAM word address

    ; DMA channel 1: word write mode (2 regs write once: VMDATAL+VMDATAH)
    lda #$1801          ; Mode 01 (ab), target $18 (VMDATAL)
    sta $4310

    ; Transfer size = 2048 bytes
    lda #2048
    sta $4315

    ; Source address from tilemap_src_addr (set by textInit in C)
    lda tilemap_src_addr
    sta $4312           ; DMA source address (low word)

    sep #$20            ; 8-bit A
    lda #$00            ; Bank $00 (WRAM mirror, buffer always < $2000)
    sta $4314           ; DMA source bank

    lda #$02            ; Enable DMA channel 1
    sta $420B

    plb                 ; Restore data bank
    plp
    rtl                 ; Return long (called with JSL)

.ENDS

;==============================================================================
; Note on C Code Placement
;==============================================================================
; The compiler generates data accesses using `lda.l $0000,x` which reads from
; bank $00. In both LoROM and HiROM modes, bank $00:$8000-$FFFF contains ROM.
; C code sections use SUPERFREE and will be placed in available ROM space.
; No special BASE directive is needed since code stays in bank $00.
;==============================================================================

;==============================================================================
; External References
;==============================================================================
; Note: main() is defined in the compiled C code that follows this file.
; oamUpdate is provided by either the library or oam_helpers.asm.
