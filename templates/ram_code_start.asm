;==============================================================================
; RAM code window — start (linked only when RAM_CODE_SIZE > 0)
;==============================================================================
; Code that must run while the CPU cannot read the Game Pak ROM (a Super FX
; job with SCMR RON = 1) lives here. It is STORED in ROM, in the top
; RAM_CODE_SIZE bytes of bank 1 (LoROM / SA-1 / Super FX: $01:xxxx; HiROM:
; the same bytes seen through the $01:8000-$FFFF mirror), and LINKED at the
; same 16-bit address in bank $7E: the section's BASE turns bank 1 into $7E
; for every label, so jumps, calls and data references inside it are the
; RAM ones. crt0 copies the bytes RamCodeStart..RamCodeEnd from $01 to $7E
; after the data-init copy.
;
; Add code to the window with the RAM_CODE_SECTION macro (assets.inc), which
; appends to ".ram_code". ram_code_end.o is linked after every other object,
; like data_init_end.o, so RamCodeEnd closes the window.
;
; The WRAM the window occupies is reserved by .ram_code_space, so no FAR or
; RAMSECTION variable is placed over it; wlalink fails the link if the code
; outgrows the window (the section would run past $FFFF).
;==============================================================================

.RAMSECTION ".ram_code_space" BANK $7E SLOT 2 ORGA RAM_CODE_ORG_VAL FORCE
    ram_code_space dsb (65536 - RAM_CODE_ORG_VAL)
.ENDS

.SECTION ".ram_code" BANK 1 SLOT "ROM" ORGA RAM_CODE_ORG_VAL BASE $7D FORCE   ; the slot by name: a bare 0 drew WLA's "SLOT number 0 / SLOT with starting address 0" warning (2026-10-04)
RamCodeStart:
    .db 0   ; placeholder: wla-dx drops an empty section, and APPENDTO needs it (as data_init_start)
.ENDS
