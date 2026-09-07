; Memory map for standalone C compilation units (HiROM)
; Must match the memory map in hdr_hirom.asm

.MEMORYMAP
    ; HiROM uses full 64KB banks. Code must be placed at $8000+ to be
    ; accessible via the bank $00 mirror ($00:8000-FFFF -> $40:8000-FFFF).
    SLOTSIZE $10000         ; 64KB per slot (full HiROM bank)
    DEFAULTSLOT 0
    SLOT 0 $0000 $10000     ; ROM at $0000-$FFFF (64KB per bank)
    SLOT 1 $0000 $2000      ; Work RAM at $0000-$1FFF (8KB for DP/Stack)
    SLOT 2 $2000 $E000      ; Work RAM at $2000-$FFFF (56KB)
    SLOT 3 $0000 $10000     ; Bank $7E full RAM (64KB)
.ENDME

.ROMBANKSIZE $10000         ; 64KB per bank (HiROM standard)
.ROMBANKS 8                 ; Match ROM configuration

; Bank byte of every 24-bit address and `:label` in this unit = $C0 + linker
; bank (#127.3, 2026-09-07). HiROM maps the FULL 64 KB of each ROM bank only
; at $C0-$FF; banks $00-$3F / $80-$BF expose the upper half ($8000-$FFFF)
; alone. Without this base a label at offset $0000 of linker bank 1 became
; $01:0000 — WRAM mirror, not ROM — which is why the runtime's .mul32/.div32
; at 07:0000 and any far-placed payload read garbage on HiROM. Code at
; $8000+ is reachable both ways ($C0:8xxx mirrors $00:8xxx); $C0-$FF is
; also the FastROM window, so nothing gets slower.
.BASE $C0

; Bank search order for bulk read-only payload: highest first, so it lands
; away from the code bank. A STRING define — a bare 7-1 would be read as an
; expression and BANKS needs the literal text.
.DEFINE ASSET_BANKS "7-1"

; Suppress WLA-DX "No .ASCIITABLE defined" warning for .ASC directives
.ASCIITABLE
MAP ' ' TO '~' = ' '
.ENDA
