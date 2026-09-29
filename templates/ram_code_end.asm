;==============================================================================
; RAM code window — end marker (linked last, see ram_code_start.asm)
;==============================================================================

.SECTION ".ram_code_end" BASE $7D APPENDTO ".ram_code"
RamCodeEnd:
    .db 0   ; placeholder, not copied: wla-dx drops an empty section
.ENDS
