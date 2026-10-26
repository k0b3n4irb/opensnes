; Memory map for standalone C compilation units (LoROM)
; Must match the memory map in hdr.asm

.MEMORYMAP
    SLOTSIZE $8000          ; 32KB per slot (default)
    DEFAULTSLOT 0
    SLOT 0 START $8000 SIZE $8000 NAME "ROM"   ; ROM mapped at $8000-$FFFF (32KB); named so GSU_SECTION can say SLOT "ROM" (a bare 0 drew a WLA warning: slot 1 also starts at 0)
    SLOT 1 $0000 $2000      ; Work RAM at $0000-$1FFF (8KB for DP/Stack)
    SLOT 2 $2000 $E000      ; Work RAM at $2000-$FFFF (56KB)
    SLOT 3 $0000 $10000     ; Bank $7E full RAM (64KB) — also the window for Game Pak RAM:
                            ; a Super FX variable is `.RAMSECTION "x" BANK $70 SLOT 3`
.ENDME

.ROMBANKSIZE $8000          ; 32KB banks (LoROM)
;
; Bank byte of every ROM label in this unit = $80 + linker bank. In LoROM
; the banks $80-$BF repeat $00-$3F: WRAM's first 8 KB at $0000-$1FFF, the
; registers, and ROM at $8000-$FFFF, which only there reads at 6 master
; cycles a byte when MEMSEL ($420D) asks for FastROM (fullsnes "CPU Clock
; Cycles / SNES Memory Speed", corpus chunk fb3d2c7a8688f4d2). The library
; is built once for every LoROM project, so its code is always addressed
; there: without FastROM the mirror is as slow as bank $00 and nothing
; changes; with it, every jsl to C or library code lands in the fast half.
; Until 2026-10-26 the code was addressed at $00 and USE_FASTROM sped up the
; startup and the NMI entry only (measured: a LoROM ROM spent the same
; master cycles with and without it). Not for a Super FX cartridge, whose
; $80-$BF:8000 is a second CPU ROM that carts leave empty (fullsnes "SNES
; Cart GSU-n Memory Map", chunk fbb657b1a04ee53c). RAMSECTION labels ignore
; the base (compiler/PINS.md, the wla-dx patch).
.IFNDEF SUPERFX
.BASE $80
.ENDIF
; ROM size is a project knob since 2026-09-24: make passes -D ROM_BANKS_VAL and
; -D ASSET_BANKS_VAL (make/common.mk, ROM_BANKS); the prebuilt library keeps
; the default. wlalink takes the largest bank count it is given.
.IFDEF ROM_BANKS_VAL
.ROMBANKS ROM_BANKS_VAL
.ELSE
.ROMBANKS 8                 ; default; the header's ROMSIZE byte follows it
.ENDIF

; Bank search order for ASSET_SECTION payload: highest first, so bulk
; read-only data lands away from the code bank without anyone picking
; a number by hand. Must name only banks this memory map declares.
.IFDEF ASSET_BANKS_VAL
.DEFINE ASSET_BANKS ASSET_BANKS_VAL
.ELSE
.DEFINE ASSET_BANKS "7-1"   ; highest asset bank first, bank $00 never
.ENDIF
; Suppress WLA-DX "No .ASCIITABLE defined" warning for .ASC directives
.ASCIITABLE
MAP ' ' TO '~' = ' '
.ENDA
