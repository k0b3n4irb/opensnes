;==============================================================================
; OpenSNES Library Functions
;==============================================================================
; Minimal library functions for examples.
; These will be replaced by the full library when it's ready.
;==============================================================================

;------------------------------------------------------------------------------
; PPU Register Constants (for use in assembly)
;------------------------------------------------------------------------------
.define REG_INIDISP     $2100
.define REG_OBJSEL      $2101
.define REG_OAMADDL     $2102
.define REG_OAMADDH     $2103
.define REG_OAMDATA     $2104
.define REG_BGMODE      $2105
.define REG_BG1SC       $2107
.define REG_BG2SC       $2108
.define REG_BG3SC       $2109
.define REG_BG4SC       $210A
.define REG_BG12NBA     $210B
.define REG_BG34NBA     $210C
.define REG_BG1HOFS     $210D
.define REG_BG1VOFS     $210E
.define REG_BG2HOFS     $210F
.define REG_BG2VOFS     $2110
.define REG_VMAIN       $2115
.define REG_VMADDL      $2116
.define REG_VMADDH      $2117
.define REG_VMDATAL     $2118
.define REG_VMDATAH     $2119
.define REG_CGADD       $2121
.define REG_CGDATA      $2122
.define REG_TM          $212C
.define REG_TS          $212D
.define REG_NMITIMEN    $4200
.define REG_RDNMI       $4210
.define REG_JOY1L       $4218
.define REG_JOY1H       $4219

;------------------------------------------------------------------------------
; RAM Variables for Library
;------------------------------------------------------------------------------
.RAMSECTION ".lib_vars" BANK 0 SLOT 1
    lib_brightness  dsb 1
    lib_is_pal      dsb 1
    lib_rand_seed   dsb 2
    lib_frame_count dsb 2
    pad0            dsb 2   ; Current joypad 1 state
    pad0_held       dsb 2   ; Buttons being held
    pad0_pressed    dsb 2   ; Buttons just pressed (edge detect)
.ENDS

;------------------------------------------------------------------------------
; consoleInit - Initialize SNES hardware
;------------------------------------------------------------------------------
.SECTION ".lib_console" SUPERFREE

consoleInit:
    php
    sep #$20

    ; Disable NMI during init
    stz $4200

    ; Detect PAL/NTSC
    lda $213F
    and #$10
    sta.l lib_is_pal

    ; Set default brightness
    lda #15
    sta.l lib_brightness

    ; Initialize random seed
    lda $213C           ; H counter
    sta.l lib_rand_seed
    lda $213D           ; V counter
    sta.l lib_rand_seed+1
    ora.l lib_rand_seed
    bne +
    lda #$E1            ; Default if zero
    sta.l lib_rand_seed
    lda #$AC
    sta.l lib_rand_seed+1
+

    ; Clear frame counter
    rep #$20
    stz.w lib_frame_count

    ; Set Mode 1 as default
    sep #$20
    lda #$01
    sta $2105           ; BGMODE

    ; Clear CGRAM to black
    stz $2121           ; CGADD = 0
    ldx #0
-   stz $2122
    stz $2122
    inx
    cpx #256
    bne -

    ; Enable NMI and auto-joypad
    lda #$81
    sta $4200

    plp
    rtl

;------------------------------------------------------------------------------
; setScreenOn - Enable display
;------------------------------------------------------------------------------
setScreenOn:
    php
    sep #$20
    lda.l lib_brightness
    and #$0F
    sta $2100           ; INIDISP
    plp
    rtl

;------------------------------------------------------------------------------
; setScreenOff - Disable display (force blank)
;------------------------------------------------------------------------------
setScreenOff:
    php
    sep #$20
    lda #$80
    sta $2100           ; INIDISP force blank
    plp
    rtl

;------------------------------------------------------------------------------
; setBrightness - Set screen brightness
; void setBrightness(u8 brightness)
;------------------------------------------------------------------------------
setBrightness:
    php
    sep #$20
    lda 5,s             ; Get brightness parameter
    and #$0F
    sta.l lib_brightness
    sta $2100           ; INIDISP
    plp
    rtl

;------------------------------------------------------------------------------
; setMode - Set BG mode
; void setMode(u8 mode)
;------------------------------------------------------------------------------
setMode:
    php
    sep #$20
    lda 5,s             ; Get mode parameter
    and #$07
    sta $2105           ; BGMODE
    plp
    rtl

;------------------------------------------------------------------------------
; WaitForVBlank - Wait for VBlank
;------------------------------------------------------------------------------
WaitForVBlank:
    php
    sep #$20
    ; Wait for vblank_flag (set by NMI handler in crt0.asm)
-   lda.l vblank_flag
    beq -
    lda #0
    sta.l vblank_flag   ; Clear flag
    ; Increment frame counter
    rep #$20
    inc.w lib_frame_count
    plp
    rtl

;------------------------------------------------------------------------------
; getFrameCount - Return frame counter
;------------------------------------------------------------------------------
getFrameCount:
    php
    rep #$20
    lda.w lib_frame_count
    plp
    rtl

;------------------------------------------------------------------------------
; rand - Return random number
;------------------------------------------------------------------------------
rand:
    php
    rep #$20
    ; Simple LFSR
    lda.w lib_rand_seed
    lsr a
    bcc +
    eor #$B400          ; Polynomial taps
+   sta.w lib_rand_seed
    plp
    rtl

;------------------------------------------------------------------------------
; bgSetDisable - Disable a background layer
; void bgSetDisable(u8 bg)
;------------------------------------------------------------------------------
bgSetDisable:
    php
    sep #$20
    ; Read current TM, clear the bit for this BG
    lda $212C           ; Can't read TM directly, so we track it
    ; For now, just do nothing - examples will set TM directly
    plp
    rtl

;------------------------------------------------------------------------------
; setMainScreen - Enable layers on main screen
; void setMainScreen(u8 layers)
;------------------------------------------------------------------------------
setMainScreen:
    php
    sep #$20
    lda 5,s             ; Get layers bitmask
    sta $212C           ; TM - main screen designation
    plp
    rtl

;------------------------------------------------------------------------------
; setSubScreen - Enable layers on sub screen
; void setSubScreen(u8 layers)
;------------------------------------------------------------------------------
setSubScreen:
    php
    sep #$20
    lda 5,s             ; Get layers bitmask
    sta $212D           ; TS - sub screen designation
    plp
    rtl

;------------------------------------------------------------------------------
; consoleEnableNMI - Enable NMI (VBlank interrupt)
;------------------------------------------------------------------------------
consoleEnableNMI:
    php
    sep #$20
    lda #$81            ; NMI + auto-joypad
    sta $4200           ; NMITIMEN
    plp
    rtl

;------------------------------------------------------------------------------
; consoleDisableNMI - Disable NMI
;------------------------------------------------------------------------------
consoleDisableNMI:
    php
    sep #$20
    lda #$00
    sta $4200           ; NMITIMEN
    plp
    rtl

;------------------------------------------------------------------------------
; padUpdate - Read joypad state and update pressed flags
; Call once per frame, typically after WaitForVBlank
;------------------------------------------------------------------------------
padUpdate:
    php
    rep #$30            ; 16-bit A/X/Y

    ; Wait for auto-joypad read to complete
    sep #$20
-   lda $4212           ; HVBJOY
    and #$01            ; Check auto-read status
    bne -
    rep #$20

    ; Read joypad 1
    lda $4218           ; JOY1L/JOY1H

    ; Calculate newly pressed buttons
    ; pressed = current AND NOT held
    sta.w pad0
    eor.w pad0_held     ; XOR with held to get changes
    and.w pad0          ; AND with current = newly pressed
    sta.w pad0_pressed

    ; Update held state
    lda.w pad0
    sta.w pad0_held

    plp
    rtl

;------------------------------------------------------------------------------
; padPressed - Return buttons pressed this frame (edge-triggered)
; Returns: A = button mask for pressed buttons
;------------------------------------------------------------------------------
padPressed:
    php
    rep #$20
    lda.w pad0_pressed
    plp
    rtl

;------------------------------------------------------------------------------
; padHeld - Return buttons currently held
; Returns: A = button mask for held buttons
;------------------------------------------------------------------------------
padHeld:
    php
    rep #$20
    lda.w pad0_held
    plp
    rtl

.ENDS
