;==============================================================================
; SNESMOD Audio Driver for OpenSNES
;==============================================================================
;
; Ported from PVSnesLib's SNESMOD implementation
; Original SNESMod by mukunda
; WLA-DX port by KungFuFurby
; Adapted for OpenSNES
;
; Features:
;   - Full Impulse Tracker module playback
;   - 8-channel audio with echo/reverb
;   - Sound effects overlay
;   - Volume fading
;   - Position synchronization
;
; This software is provided 'as-is', without any express or implied
; warranty. In no event will the authors be held liable for any damages
; arising from the use of this software.
;
; Permission is granted to anyone to use this software for any purpose,
; including commercial applications, and to alter it and redistribute it
; freely, subject to the following restrictions:
;
; 1. The origin of this software must not be misrepresented; you must not
;    claim that you wrote the original software.
; 2. Altered source versions must be plainly marked as such, and must not
;    be misrepresented as being the original software.
; 3. This notice may not be removed or altered from any source distribution.
;
;==============================================================================

.include "lib_memmap.inc"

;==============================================================================
; Soundbank Offsets (LoROM mode)
;==============================================================================

.define SB_SAMPCOUNT    $8000
.define SB_MODCOUNT     $8002
.define SB_MODTABLE     $8004
.define SB_SRCTABLE     $8184

;==============================================================================
; Hardware Registers
;==============================================================================

.equ REG_APUIO0         $2140   ; SPC I/O port 0
.equ REG_APUIO1         $2141   ; SPC I/O port 1
.define REG_APUIO2      $2142   ; SPC I/O port 2
.define REG_APUIO3      $2143   ; SPC I/O port 3
.define REG_SLHV        $2137   ; Software latch for H/V counter
.define REG_OPVCT       $213D   ; Y scanline location

.equ REG_NMI_TIMEN      $4200

;==============================================================================
; SPC Commands
;==============================================================================

.define CMD_LOAD        $00
.define CMD_LOADE       $01
.define CMD_VOL         $02
.define CMD_PLAY        $03
.define CMD_STOP        $04
.define CMD_MVOL        $05
.define CMD_FADE        $06
.define CMD_RES         $07
.define CMD_FX          $08
.define CMD_SSIZE       $09
.define CMD_PAUSE       $0A
.define CMD_RESUME      $0B

;==============================================================================
; Constants
;==============================================================================

.define PROCESS_TIME    5       ; Process for 5 scanlines
.define INIT_DATACOPY   13
.define SPC_BOOT        $0400   ; SPC entry/load address

;==============================================================================
; Zero Page Variables ($0040-$007F reserved for SNESMOD)
;==============================================================================

.RAMSECTION ".snesmod_zp" BANK 0 SLOT 1 ORGA $40 FORCE

spc_ptr:        DSB 3           ; Soundbank pointer
spc_v:          DSB 1           ; Sync/handshake byte
spc_bank:       DSB 1           ; Current soundbank bank

spc1:           DSB 2           ; Temp variables
spc2:           DSB 2

spc_fread:      DSB 1           ; FIFO read pointer
spc_fwrite:     DSB 1           ; FIFO write pointer

; Port record (for interruption)
spc_pr:         DSB 4

; Streaming variables
digi_src:       DSB 3
digi_src2:      DSB 3

SoundTable:     DSB 3

spc_sfx_next:   DSB 1
spc_q:          DSB 1

digi_init:      DSB 1
digi_pitch:     DSB 1
digi_vp:        DSB 1
digi_remain:    DSB 2
digi_active:    DSB 1
digi_copyrate:  DSB 1

.ENDS

;==============================================================================
; RAM Section - Command FIFO
;==============================================================================

.RAMSECTION ".snesmod_fifo" BANK 0 SLOT 1 ORGA $0600 FORCE

spc_fifo:       DSB 256         ; 256-byte command queue

.ENDS

;==============================================================================
; Code Section
;==============================================================================

.SECTION ".snesmod_code" SUPERFREE

.index 16
.accu 8

;------------------------------------------------------------------------------
; snesmodInit - Initialize SNESMOD and upload SPC driver
;------------------------------------------------------------------------------
; void snesmodInit(void);
;------------------------------------------------------------------------------
snesmodInit:
    php
    sei
    phb
    sep #$20
    lda #$0
    sta REG_NMI_TIMEN
    pha
    plb                         ; Change bank address to 0

-:  ldx REG_APUIO0              ; Wait for ready signal from SPC
    cpx #$BBAA
    bne -

    stx REG_APUIO1              ; Start transfer
    ldx #SPC_BOOT               ; Port2,3 = transfer address
    stx REG_APUIO2
    lda #$CC                    ; Port0 = $CC
    sta REG_APUIO0

-:  cmp REG_APUIO0              ; Wait for SPC
    bne -

    ; Ready to transfer - read first byte
    lda.l SM_SPC
    xba
    lda #0
    ldx #1
    bra @sb_start

    ; Transfer data loop
@sb_send:
    xba                         ; Swap DATA into A
    lda.l SM_SPC, x             ; Read next byte
    inx
    xba                         ; Swap DATA into B

-:  cmp REG_APUIO0              ; Wait for SPC
    bne -

    ina                         ; Increment counter

@sb_start:
    rep #$20                    ; Write port0+port1 data
    sta REG_APUIO0
    sep #$20

    cpx #SM_SPC_end-SM_SPC      ; Loop until all bytes transferred
    bcc @sb_send

    ; All bytes transferred
-:  cmp REG_APUIO0              ; Wait for SPC
    bne -

    ina                         ; Add 2
    ina

    stz REG_APUIO1              ; Port1=0
    ldx #SPC_BOOT               ; Port2,3 = entry point
    stx REG_APUIO2
    sta REG_APUIO0              ; Write P0 data

-:  cmp REG_APUIO0              ; Final sync
    bne -

    stz REG_APUIO0

    ; Initialize state
    stz spc_v
    stz spc_q
    stz spc_fwrite
    stz spc_fread
    stz spc_sfx_next

    stz spc_pr+0
    stz spc_pr+1
    stz spc_pr+2
    stz spc_pr+3

    stz spc_ptr+0
    stz spc_ptr+1
    stz spc_ptr+2
    stz spc_bank

    stz spc1+0
    stz spc1+1
    stz spc2+0
    stz spc2+1

    stz digi_src+0
    stz digi_src+1
    stz digi_src+2
    stz digi_src2+0
    stz digi_src2+1
    stz digi_src2+2

    stz SoundTable+0
    stz SoundTable+1
    stz SoundTable+2

    stz spc_sfx_next

    stz digi_init
    stz digi_pitch
    stz digi_vp
    stz digi_remain+0
    stz digi_remain+1
    stz digi_active
    stz digi_copyrate

    ; Clear FIFO
    ldx #$0
    lda #$0
-:  sta.w spc_fifo,x
    inx
    cpx #$ff
    bne -

    ; Re-enable NMI (VBlank interrupt) - critical for WaitForVBlank to work!
    lda #$81                    ; NMI enable + auto joypad read
    sta REG_NMI_TIMEN

    plb
    cli
    plp
    rtl

;------------------------------------------------------------------------------
; snesmodSetSoundbank - Set the soundbank bank number
;------------------------------------------------------------------------------
; void snesmodSetSoundbank(u8 bank);
; Stack: 6,s = bank number (low byte)
;
; Note: OpenSNES uses 16-bit pointers, so we pass just the bank number.
;       Soundbanks always start at $8000 (LoROM), so offset is implicit.
;------------------------------------------------------------------------------
snesmodSetSoundbank:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

    lda 6,s                     ; Bank number (u8 parameter)
    sta spc_bank

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; Pointer increment macro
;------------------------------------------------------------------------------
.macro incptr
    iny
    iny
    bmi +
    inc spc_ptr+2
    ldy #$8000
+:
.endm

;------------------------------------------------------------------------------
; snesmodLoadModule - Load a module from the soundbank
;------------------------------------------------------------------------------
; void snesmodLoadModule(u16 moduleId);
;------------------------------------------------------------------------------
snesmodLoadModule:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

    rep #$30
    lda 6,s                     ; module_id
    tax
    sep #$20

    phx                         ; Flush FIFO first
    jsr xspcFlush
    plx

    phx
    ldy #SB_MODTABLE
    sty spc2
    jsr get_address

    rep #$20
    lda [spc_ptr], y            ; X = MODULE SIZE
    tax

    incptr

    lda [spc_ptr], y            ; Read SOURCE LIST SIZE

    incptr

    sty spc1                    ; pointer += listsize*2
    asl
    adc spc1
    bmi +
    ora #$8000
    inc spc_ptr+2
+:  tay

    sep #$20
    lda spc_v                   ; Wait for SPC
    pha
-:  cmp REG_APUIO1
    bne -

    lda #CMD_LOAD               ; Send LOAD message
    sta REG_APUIO0
    pla
    eor #$80
    ora #$01
    sta spc_v
    sta REG_APUIO1

-:  cmp REG_APUIO1              ; Wait for SPC
    bne -

    jsr do_transfer

    ; Transfer sources
    plx
    ldy #SB_MODTABLE
    sty spc2
    jsr get_address
    incptr

    rep #$20
    lda [spc_ptr], y            ; x = number of sources
    tax

    incptr

@transfer_sources:
    lda [spc_ptr], y            ; Read source index
    sta spc1

    incptr

    phy                         ; Push memory pointer and counter
    sep #$20
    lda spc_ptr+2
    pha
    phx

    jsr transfer_source

    plx                         ; Pull memory pointer and counter
    pla
    sta spc_ptr+2
    ply

    dex
    bne @transfer_sources

    ; End transfers
    stz REG_APUIO0
    lda spc_v
    eor #$80
    sta spc_v
    sta REG_APUIO1
-:  cmp REG_APUIO1
    bne -

    sta spc_pr+1
    stz spc_sfx_next            ; Reset SFX counter

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; transfer_source - Transfer a single source
;------------------------------------------------------------------------------
transfer_source:
    ldx spc1
    ldy #SB_SRCTABLE
    sty spc2
    jsr get_address

    lda #$01                    ; Port0=$01
    sta REG_APUIO0
    rep #$20
    lda [spc_ptr], y            ; x = length (bytes->words)
    incptr
    ina
    lsr
    tax
    lda [spc_ptr], y            ; Port2,3 = loop point
    sta REG_APUIO2
    incptr
    sep #$20

    lda spc_v                   ; Send message
    eor #$80
    ora #$01
    sta spc_v
    sta REG_APUIO1

-:  cmp REG_APUIO1              ; Wait for SPC
    bne -

    cpx #0
    beq end_transfer
    bra do_transfer

;------------------------------------------------------------------------------
; transfer_again / do_transfer - Data transfer loop
;------------------------------------------------------------------------------
transfer_again:
    eor #$80
    sta REG_APUIO1
    sta spc_v
    incptr

-:  cmp REG_APUIO1
    bne -

do_transfer:
    rep #$20                    ; Transfer 1 word
    lda [spc_ptr], y
    sta REG_APUIO2
    sep #$20
    lda spc_v
    dex
    bne transfer_again

    incptr

end_transfer:
    lda #0                      ; Final word transferred
    sta REG_APUIO1              ; Write p1=0 to terminate
    sta spc_v

-:  cmp REG_APUIO1
    bne -

    sta spc_pr+1
    rts

;------------------------------------------------------------------------------
; get_address - Get address from soundbank table
;------------------------------------------------------------------------------
; spc2 = table offset, x = index
; Returns: spc_ptr = 0,0,bank, Y = address
;------------------------------------------------------------------------------
get_address:
    lda spc_bank
    sta spc_ptr+2
    rep #$20
    stx spc1
    txa
    asl
    adc spc1
    adc spc2
    sta spc_ptr

    lda [spc_ptr]               ; Read address
    pha
    sep #$20
    ldy #2
    lda [spc_ptr],y             ; Read bank#

    clc
    adc spc_bank
    sta spc_ptr+2
    ply
    stz spc_ptr
    stz spc_ptr+1
    rts

;------------------------------------------------------------------------------
; snesmodLoadEffect - Load a sound effect
;------------------------------------------------------------------------------
; u8 snesmodLoadEffect(u16 sfxIndex);
;------------------------------------------------------------------------------
snesmodLoadEffect:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

    rep #$30
    lda 6,s                     ; id
    tax
    sep #$20

    ldy #SB_SRCTABLE
    sty spc2
    jsr get_address

    lda spc_v                   ; Sync with SPC
-:  cmp REG_APUIO1
    bne -

    lda #CMD_LOADE              ; Write message
    sta REG_APUIO0

    lda spc_v                   ; Dispatch and wait
    eor #$80
    ora #$01
    sta spc_v
    sta REG_APUIO1
-:  cmp REG_APUIO1
    bne -

    rep #$20
    lda [spc_ptr], y            ; x = length (bytes->words)
    ina
    lsr
    incptr
    tax
    incptr                      ; Skip loop
    sep #$20

    jsr do_transfer

    lda spc_sfx_next            ; Return SFX index
    inc spc_sfx_next

    sta tcc__r0
    stz tcc__r0+1

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; QueueMessage - Queue a message to the SPC
;------------------------------------------------------------------------------
; A = id, spc1 = params
;------------------------------------------------------------------------------
QueueMessage:
    sei                         ; Disable IRQ

    sep #$10
    ldx spc_fwrite
    sta.w spc_fifo, x
    inx
    lda spc1
    sta.w spc_fifo, x
    inx
    lda spc1+1
    sta.w spc_fifo, x
    inx
    stx spc_fwrite
    rep #$10

    cli

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; snesmodFlush - Wait for command queue to empty
;------------------------------------------------------------------------------
; void snesmodFlush(void);
;------------------------------------------------------------------------------
snesmodFlush:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

@flush_loop:
    lda spc_fread
    cmp spc_fwrite
    beq @exit
    jsr spcProcessMessages
    bra @flush_loop

@exit:
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; xspcFlush - Internal flush (no context switch)
;------------------------------------------------------------------------------
xspcFlush:
    lda spc_fread
    cmp spc_fwrite
    beq @exit
    jsr xspcProcessMessages
    bra xspcFlush
@exit:
    rts

;------------------------------------------------------------------------------
; xspcProcessMessages - Internal message processor
;------------------------------------------------------------------------------
xspcProcessMessages:
    sep #$10
    lda spc_fwrite
    cmp spc_fread
    beq @exit2

    ldy #PROCESS_TIME

@process_again:
    lda spc_v
    cmp REG_APUIO1
    bne @next

    ldx spc_fread
    lda.w spc_fifo, x
    sta REG_APUIO0
    sta spc_pr+0
    inx
    lda.w spc_fifo, x
    sta REG_APUIO2
    sta spc_pr+2
    inx
    lda.w spc_fifo, x
    sta REG_APUIO3
    sta spc_pr+3
    inx
    stx spc_fread

    lda spc_v
    eor #$80
    sta spc_v
    sta REG_APUIO1
    sta spc_pr+1

    lda spc_fread
    cmp spc_fwrite
    beq @exit2

@next:
    lda REG_SLHV
    lda REG_OPVCT
    cmp spc1
    beq @process_again
    sta spc1
    dey
    bne @process_again

@exit2:
    rep #$10
    rts

;------------------------------------------------------------------------------
; snesmodProcess - Process audio (call every frame!)
;------------------------------------------------------------------------------
; void snesmodProcess(void);
;------------------------------------------------------------------------------
snesmodProcess:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

    lda digi_active
    beq spcProcessMessages
    jsr spcProcessStream

spcProcessMessages:
    sep #$10
    lda spc_fwrite
    cmp spc_fread
    beq @exit2

    ldy #PROCESS_TIME

@process_again:
    lda spc_v
    cmp REG_APUIO1
    bne @next

    ldx spc_fread
    lda.w spc_fifo, x
    sta REG_APUIO0
    sta spc_pr+0
    inx
    lda.w spc_fifo, x
    sta REG_APUIO2
    sta spc_pr+2
    inx
    lda.w spc_fifo, x
    sta REG_APUIO3
    sta spc_pr+3
    inx
    stx spc_fread

    lda spc_v
    eor #$80
    sta spc_v
    sta REG_APUIO1
    sta spc_pr+1

    lda spc_fread
    cmp spc_fwrite
    beq @exit2

@next:
    lda REG_SLHV
    lda REG_OPVCT
    cmp spc1
    beq @process_again
    sta spc1
    dey
    bne @process_again

@exit2:
    rep #$10
    plb
    plp
    rtl

;------------------------------------------------------------------------------
; snesmodPlay - Start module playback
;------------------------------------------------------------------------------
; void snesmodPlay(u8 startPosition);
;------------------------------------------------------------------------------
snesmodPlay:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

    lda 6,s                     ; startPos
    sta spc1+1
    lda #CMD_PLAY
    jmp QueueMessage

;------------------------------------------------------------------------------
; snesmodStop - Stop module playback
;------------------------------------------------------------------------------
; void snesmodStop(void);
;------------------------------------------------------------------------------
snesmodStop:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

    lda #CMD_STOP
    jmp QueueMessage

;------------------------------------------------------------------------------
; snesmodPause - Pause module playback
;------------------------------------------------------------------------------
; void snesmodPause(void);
;------------------------------------------------------------------------------
snesmodPause:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

    lda #CMD_PAUSE
    jmp QueueMessage

;------------------------------------------------------------------------------
; snesmodResume - Resume module playback
;------------------------------------------------------------------------------
; void snesmodResume(void);
;------------------------------------------------------------------------------
snesmodResume:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

    lda #CMD_RESUME
    jmp QueueMessage

;------------------------------------------------------------------------------
; snesmodSetModuleVolume - Set module volume
;------------------------------------------------------------------------------
; void snesmodSetModuleVolume(u8 volume);
;------------------------------------------------------------------------------
snesmodSetModuleVolume:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

    lda 6,s                     ; volume
    sta spc1+1
    lda #CMD_MVOL
    jmp QueueMessage

;------------------------------------------------------------------------------
; snesmodFadeVolume - Fade module volume
;------------------------------------------------------------------------------
; void snesmodFadeVolume(u8 targetVolume, u8 speed);
;------------------------------------------------------------------------------
snesmodFadeVolume:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

    lda 6,s                     ; speed
    sta spc1
    lda 7,s                     ; target volume
    sta spc1+1
    lda #CMD_FADE
    jmp QueueMessage

;------------------------------------------------------------------------------
; snesmodPlayEffect - Play a sound effect
;------------------------------------------------------------------------------
; u8 snesmodPlayEffect(u16 effectId, u8 volume, u8 pan, u16 pitch);
; Returns: channel used (0-7)
;------------------------------------------------------------------------------
snesmodPlayEffect:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

    ; Get parameters
    rep #$20
    lda 6,s                     ; pitch
    tay
    lda 8,s                     ; effectId
    tax
    sep #$20
    lda 10,s                    ; volpan (packed: vol<<4 | pan)

    sta spc1                    ; spc1.l = "vp"
    sty spc2                    ; Save pitch
    txa
    asl
    asl
    asl
    asl
    ora spc2                    ; spc1.h = "id<<4 | pitch_h"
    sta spc1+1

    lda #CMD_FX
    jmp QueueMessage

;------------------------------------------------------------------------------
; snesmodGetPosition - Get current module position
;------------------------------------------------------------------------------
; u8 snesmodGetPosition(void);
;------------------------------------------------------------------------------
snesmodGetPosition:
    php
    sep #$20
    stz tcc__r0
    lda.l REG_APUIO3
    sta tcc__r0
    rep #$20
    plp
    rtl

;------------------------------------------------------------------------------
; spcProcessStream - Process streaming audio
;------------------------------------------------------------------------------
spcProcessStream:
    rep #$20
    lda digi_remain
    bne +
    sep #$20
    stz digi_active
    rts

+:  sep #$20

    lda spc_pr+0                ; Send STREAM signal
    ora #$80
    sta REG_APUIO0

-:  bit REG_APUIO0              ; Wait for SPC
    bpl -

    stz REG_APUIO1
    lda digi_init
    beq @no_init
    stz digi_init
    lda digi_vp
    sta REG_APUIO2
    lda digi_pitch
    sta REG_APUIO3
    lda #1
    sta REG_APUIO1
    lda digi_copyrate
    clc
    adc #INIT_DATACOPY
    bra @newnote

@no_init:
    lda digi_copyrate

@newnote:
    rep #$20
    and #$00FF
    cmp digi_remain
    bcc @nsatcopy
    lda digi_remain
    stz digi_remain
    bra @copysat

@nsatcopy:
    pha
    sec
    sbc digi_remain
    eor #$FFFF
    ina
    sta digi_remain
    pla

@copysat:
    sep #$20
    sta REG_APUIO0

    sep #$10
    tax
    sta spc1
    asl
    clc
    adc spc1
    sta spc1
    ldy #0

@next_block:
    lda [digi_src2], y
    sta spc2
    rep #$20
    lda [digi_src], y

-:  cpx REG_APUIO0
    bne -

    inx
    sta REG_APUIO2
    sep #$20
    lda spc2
    sta REG_APUIO1
    stx REG_APUIO0
    iny
    iny
    iny
    dec spc1
    bne @next_block

-:  cpx REG_APUIO0
    bne -

    lda spc_pr+0                ; Restore port data
    sta REG_APUIO0
    lda spc_pr+1
    sta REG_APUIO1
    lda spc_pr+2
    sta REG_APUIO2
    lda spc_pr+3
    sta REG_APUIO3

    tya
    rep #$31
    and #$FF
    adc digi_src
    sta digi_src
    ina
    ina
    sta digi_src2
    sep #$20

    rts

;------------------------------------------------------------------------------
; snesmodSetSoundTable - Set streaming sound table pointer
;------------------------------------------------------------------------------
; void snesmodSetSoundTable(const u8 *table);
;------------------------------------------------------------------------------
snesmodSetSoundTable:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

    rep #$20
    lda 6,s
    sta SoundTable
    sep #$20
    lda 8,s
    sta SoundTable+2

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; snesmodAllocateSoundRegion - Allocate streaming buffer in SPC RAM
;------------------------------------------------------------------------------
; void snesmodAllocateSoundRegion(u8 size);
;------------------------------------------------------------------------------
snesmodAllocateSoundRegion:
    php
    phb
    sep #$20
    lda #$0
    pha
    plb

    lda 6,s                     ; Size of buffer
    pha
    jsr xspcFlush

    lda spc_v
-:  cmp REG_APUIO1
    bne -

    pla
    sta REG_APUIO3

    lda #CMD_SSIZE
    sta REG_APUIO0
    sta spc_pr+0

    lda spc_v
    eor #$80
    sta REG_APUIO1
    sta spc_v
    sta spc_pr+1

    plb
    plp
    rtl

;------------------------------------------------------------------------------
; Streaming rate table
;------------------------------------------------------------------------------
digi_rates:
    .db 0, 3, 5, 7, 9, 11, 13

.ENDS

;==============================================================================
; Include the SPC driver binary
;==============================================================================

.INCLUDE "sm_spc.asm"
