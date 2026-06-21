;==============================================================================
; HDMA Wave — Pre-computed sine wave tables
;==============================================================================
; 7 amplitude levels (0, 4, 8, 12, 16, 20, 24 pixels peak displacement) ×
; 1006 bytes per table (335 HDMA entries × 3 bytes + 1 end marker) =
; 7042 bytes total.
;
; Each entry: { 0x81, scroll_lo, scroll_hi }
;   - 0x81 = repeat flag (bit 7) + 1 scanline count (bits 6-0)
;   - scroll_lo, scroll_hi: signed 16-bit BG1HOFS offset
;
; The raw binary lives in res/hdma_wave_tables.bin. Until v0.20.x this
; data was inline in main.c as a `static const u8` array (~408 lines);
; lifting it out lets the linker place it in any bank and brings main.c
; from 661 → ~250 lines. The C side accesses it via:
;
;     extern u8 hdma_tables[];
;     hdmaSetup(... &hdma_tables[amp_offset]);
;
; The 4-byte Kl pointer carries the bank byte (post-A6+A7 ABI), so the
; hdma helpers can reach the data regardless of placement.
;==============================================================================

.section ".rodata_hdma_wave_tables" superfree

hdma_tables:
    .incbin "res/hdma_wave_tables.bin"
hdma_tables_end:

.ends
