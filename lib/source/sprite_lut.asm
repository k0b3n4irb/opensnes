;-----------------------------------------------------------------------
; sprite_lut.asm - Sprite Lookup Tables for Dynamic Sprite Management
;
; These tables map sprite indices to VRAM addresses for efficient
; dynamic sprite handling. Based on PVSnesLib's sprite lookup tables.
;
; VRAM is organized as 128 pixels (16 8x8 tiles) wide.
; - 16x16 sprites use tiles at N, N+1, N+16, N+17
; - 32x32 sprites use 16 tiles in a 4x4 pattern
;-----------------------------------------------------------------------

.include "lib_memmap.inc"

;-----------------------------------------------------------------------
; 16x16 Sprite Lookup Tables (64 sprites max)
;-----------------------------------------------------------------------

.SECTION ".lut_sprite16" SUPERFREE

; VRAM source offsets for 16x16 sprites (128-pixel wide sprite sheet)
; Sprite sheet must be 128 pixels (16 tiles) wide to match VRAM layout
; Each 16x16 = 2 tiles wide, so horizontal offset = sprite_col * 2 * 32 = $40
; Each sprite row = 2 tile rows = 32 tiles = $400 bytes
lkup16oamS:
    .word $0000, $0040, $0080, $00C0, $0100, $0140, $0180, $01C0
    .word $0400, $0440, $0480, $04C0, $0500, $0540, $0580, $05C0
    .word $0800, $0840, $0880, $08C0, $0900, $0940, $0980, $09C0
    .word $0C00, $0C40, $0C80, $0CC0, $0D00, $0D40, $0D80, $0DC0
    .word $1000, $1040, $1080, $10C0, $1100, $1140, $1180, $11C0
    .word $1400, $1440, $1480, $14C0, $1500, $1540, $1580, $15C0
    .word $1800, $1840, $1880, $18C0, $1900, $1940, $1980, $19C0
    .word $1C00, $1C40, $1C80, $1CC0, $1D00, $1D40, $1D80, $1DC0

; OAM tile IDs for 16x16 sprites (small size mode)
; Tile IDs increment by 2 (each 16x16 uses 2 tiles per row)
; Offset by $100 for second name table
lkup16idT:
    .word $0100, $0102, $0104, $0106, $0108, $010A, $010C, $010E
    .word $0120, $0122, $0124, $0126, $0128, $012A, $012C, $012E
    .word $0140, $0142, $0144, $0146, $0148, $014A, $014C, $014E
    .word $0160, $0162, $0164, $0166, $0168, $016A, $016C, $016E
    .word $0180, $0182, $0184, $0186, $0188, $018A, $018C, $018E
    .word $01A0, $01A2, $01A4, $01A6, $01A8, $01AA, $01AC, $01AE
    .word $01C0, $01C2, $01C4, $01C6, $01C8, $01CA, $01CC, $01CE
    .word $01E0, $01E2, $01E4, $01E6, $01E8, $01EA, $01EC, $01EE

; OAM tile IDs for 16x16 sprites (large size mode - 16x16/32x32 config)
lkup16idT0:
    .word $0000, $0002, $0004, $0006, $0008, $000A, $000C, $000E
    .word $0020, $0022, $0024, $0026, $0028, $002A, $002C, $002E
    .word $0040, $0042, $0044, $0046, $0048, $004A, $004C, $004E
    .word $0060, $0062, $0064, $0066, $0068, $006A, $006C, $006E
    .word $0080, $0082, $0084, $0086, $0088, $008A, $008C, $008E
    .word $00A0, $00A2, $00A4, $00A6, $00A8, $00AA, $00AC, $00AE
    .word $00C0, $00C2, $00C4, $00C6, $00C8, $00CA, $00CC, $00CE
    .word $00E0, $00E2, $00E4, $00E6, $00E8, $00EA, $00EC, $00EE

; VRAM destination block addresses for 16x16 sprites
; Where to place each sprite's tiles in VRAM
lkup16idB:
    .word $0000, $0020, $0040, $0060, $0080, $00A0, $00C0, $00E0
    .word $0200, $0220, $0240, $0260, $0280, $02A0, $02C0, $02E0
    .word $0400, $0420, $0440, $0460, $0480, $04A0, $04C0, $04E0
    .word $0600, $0620, $0640, $0660, $0680, $06A0, $06C0, $06E0
    .word $0800, $0820, $0840, $0860, $0880, $08A0, $08C0, $08E0
    .word $0A00, $0A20, $0A40, $0A60, $0A80, $0AA0, $0AC0, $0AE0
    .word $0C00, $0C20, $0C40, $0C60, $0C80, $0CA0, $0CC0, $0CE0
    .word $0E00, $0E20, $0E40, $0E60, $0E80, $0EA0, $0EC0, $0EE0

.ENDS

;-----------------------------------------------------------------------
; 32x32 Sprite Lookup Tables (16 sprites max)
;-----------------------------------------------------------------------

.SECTION ".lut_sprite32" SUPERFREE

; VRAM source offsets for 32x32 sprites
; Each 32x32 sprite = 256 bytes (16 tiles * 16 bytes/tile for 4bpp)
lkup32oamS:
    .word $0000, $0080, $0100, $0180, $0800, $0880, $0900, $0980
    .word $1000, $1080, $1100, $1180, $1800, $1880, $1900, $1980

; OAM tile IDs for 32x32 sprites
; Tile IDs increment by 4 (each 32x32 uses 4 tiles per row)
lkup32idT:
    .word $0000, $0004, $0008, $000C, $0040, $0044, $0048, $004C
    .word $0080, $0084, $0088, $008C, $00C0, $00C4, $00C8, $00CC

; VRAM destination block addresses for 32x32 sprites
lkup32idB:
    .word $0000, $0040, $0080, $00C0, $0400, $0440, $0480, $04C0
    .word $0800, $0840, $0880, $08C0, $0C00, $0C40, $0C80, $0CC0

.ENDS

;-----------------------------------------------------------------------
; 8x8 Sprite Lookup Tables (128 sprites max)
;-----------------------------------------------------------------------

.SECTION ".lut_sprite8" SUPERFREE

; VRAM source offsets for 8x8 sprites
; Each 8x8 sprite = 32 bytes (1 tile * 32 bytes for 4bpp: 8 rows * 4 bytes/row)
lkup8oamS:
    .word $0000, $0020, $0040, $0060, $0080, $00A0, $00C0, $00E0
    .word $0100, $0120, $0140, $0160, $0180, $01A0, $01C0, $01E0
    .word $0200, $0220, $0240, $0260, $0280, $02A0, $02C0, $02E0
    .word $0300, $0320, $0340, $0360, $0380, $03A0, $03C0, $03E0
    .word $0400, $0420, $0440, $0460, $0480, $04A0, $04C0, $04E0
    .word $0500, $0520, $0540, $0560, $0580, $05A0, $05C0, $05E0
    .word $0600, $0620, $0640, $0660, $0680, $06A0, $06C0, $06E0
    .word $0700, $0720, $0740, $0760, $0780, $07A0, $07C0, $07E0
    .word $0800, $0820, $0840, $0860, $0880, $08A0, $08C0, $08E0
    .word $0900, $0920, $0940, $0960, $0980, $09A0, $09C0, $09E0
    .word $0A00, $0A20, $0A40, $0A60, $0A80, $0AA0, $0AC0, $0AE0
    .word $0B00, $0B20, $0B40, $0B60, $0B80, $0BA0, $0BC0, $0BE0
    .word $0C00, $0C20, $0C40, $0C60, $0C80, $0CA0, $0CC0, $0CE0
    .word $0D00, $0D20, $0D40, $0D60, $0D80, $0DA0, $0DC0, $0DE0
    .word $0E00, $0E20, $0E40, $0E60, $0E80, $0EA0, $0EC0, $0EE0
    .word $0F00, $0F20, $0F40, $0F60, $0F80, $0FA0, $0FC0, $0FE0

; OAM tile IDs for 8x8 sprites
; Simple sequential tile IDs offset by $100
lkup8idT:
    .word $0100, $0101, $0102, $0103, $0104, $0105, $0106, $0107
    .word $0108, $0109, $010A, $010B, $010C, $010D, $010E, $010F
    .word $0110, $0111, $0112, $0113, $0114, $0115, $0116, $0117
    .word $0118, $0119, $011A, $011B, $011C, $011D, $011E, $011F
    .word $0120, $0121, $0122, $0123, $0124, $0125, $0126, $0127
    .word $0128, $0129, $012A, $012B, $012C, $012D, $012E, $012F
    .word $0130, $0131, $0132, $0133, $0134, $0135, $0136, $0137
    .word $0138, $0139, $013A, $013B, $013C, $013D, $013E, $013F
    .word $0140, $0141, $0142, $0143, $0144, $0145, $0146, $0147
    .word $0148, $0149, $014A, $014B, $014C, $014D, $014E, $014F
    .word $0150, $0151, $0152, $0153, $0154, $0155, $0156, $0157
    .word $0158, $0159, $015A, $015B, $015C, $015D, $015E, $015F
    .word $0160, $0161, $0162, $0163, $0164, $0165, $0166, $0167
    .word $0168, $0169, $016A, $016B, $016C, $016D, $016E, $016F
    .word $0170, $0171, $0172, $0173, $0174, $0175, $0176, $0177
    .word $0178, $0179, $017A, $017B, $017C, $017D, $017E, $017F

; VRAM destination block addresses for 8x8 sprites (16 bytes per tile in VRAM)
; Note: VRAM stores 4bpp tiles as 16 words (32 bytes) but addressed as 16 word-increments
lkup8idB:
    .word $0000, $0010, $0020, $0030, $0040, $0050, $0060, $0070
    .word $0080, $0090, $00A0, $00B0, $00C0, $00D0, $00E0, $00F0
    .word $0100, $0110, $0120, $0130, $0140, $0150, $0160, $0170
    .word $0180, $0190, $01A0, $01B0, $01C0, $01D0, $01E0, $01F0
    .word $0200, $0210, $0220, $0230, $0240, $0250, $0260, $0270
    .word $0280, $0290, $02A0, $02B0, $02C0, $02D0, $02E0, $02F0
    .word $0300, $0310, $0320, $0330, $0340, $0350, $0360, $0370
    .word $0380, $0390, $03A0, $03B0, $03C0, $03D0, $03E0, $03F0
    .word $0400, $0410, $0420, $0430, $0440, $0450, $0460, $0470
    .word $0480, $0490, $04A0, $04B0, $04C0, $04D0, $04E0, $04F0
    .word $0500, $0510, $0520, $0530, $0540, $0550, $0560, $0570
    .word $0580, $0590, $05A0, $05B0, $05C0, $05D0, $05E0, $05F0
    .word $0600, $0610, $0620, $0630, $0640, $0650, $0660, $0670
    .word $0680, $0690, $06A0, $06B0, $06C0, $06D0, $06E0, $06F0
    .word $0700, $0710, $0720, $0730, $0740, $0750, $0760, $0770
    .word $0780, $0790, $07A0, $07B0, $07C0, $07D0, $07E0, $07F0

.ENDS
