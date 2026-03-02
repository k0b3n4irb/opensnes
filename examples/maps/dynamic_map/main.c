/*
 * DynamicMap
 *
 * Create and update 16x16 sprites in RAM on a 32x32 or 64x64 map.
 * Sprites are drawn as tilemap entries on BG1 (Mode 3, 256 colors).
 * Text overlay on BG2 shows controls.
 *
 * Includes a C64-to-SNES sprite format converter.
 *
 * Port of PVSnesLib DynamicMap example.
 *
 * Key adaptation: The tilemap buffer lives in bank $7E extended WRAM,
 * accessed via assembly helpers (smapWrite/smapRead/smapDma) because
 * the compiler generates sta.l $0000,x (bank $00 only, 8KB limit).
 * Sprite graphics are DMA'd directly from ROM to VRAM.
 *
 * Controls:
 *   A     = Toggle 32x32 / 64x64 map mode
 *   B     = Toggle scroll lock
 *   X     = Place random sprite
 *   Y     = Convert C64 sprite (Boulder Dash Rockford)
 *   D-PAD = Scroll map
 */

#include <snes.h>
#include "maputil.h"
#include "map32x32.h"
#include "map64x64.h"

#define SPRITE_EMPTY    0
#define SPRITE_GARGOYLE 1
#define SPRITE_ROCKFORD 2

/* ROM data (data.asm) */
extern u8 sprite16, sprite16_end;
extern u8 palsprite16, palsprite16_end;
extern u8 sprite16_64x64, sprite16_64x64_end;
extern u8 palsprite16_64x64, palsprite16_64x64_end;
extern u8 c64_sprite;

/* Assembly helpers (ram.asm) — bank $7E tilemap buffer */
extern void smapClear(u16 byte_count);
extern void smapDma(u16 byte_offset, u16 vram_addr, u16 byte_count);

/* vblank_flag declared in <snes/system.h> (via <snes.h>) */

/* VRAM layout */
#define VRAM_SPRITEMAP  0x1000  /* BG1 tilemap (SC_64x64) */
#define VRAM_BG2_MAP    0x6800  /* BG2 tilemap (text) */
#define VRAM_BG2_GFX    0x2000  /* BG2 tile base */
#define VRAM_FONT       0x3000  /* Font tiles within BG2 */
#define VRAM_SPRITE_GFX 0x4000  /* BG1 sprite tiles (256-color) */

/* Font tile offset = (VRAM_FONT - VRAM_BG2_GFX) / 16 words per 4bpp tile */
#define FONT_TILE_OFFSET 0x100

u16 spritemap_len = 0x2000; /* bytes */
u16 number_of_sprites = 0x40;

u8 scroll_lock = 1;
u8 is_map32x32 = 1;

u16 max_scroll_width;
u16 max_scroll_height;

/* Temp buffer for C64 sprite conversion (bank $00 WRAM) */
static u8 sprite_temp[256];

/* Pre-computed C64 Rockford sprite in SNES 8bpp format (for verification) */
static const u8 rockford_8bpp[256] = {
    0x00, 0x00, 0x0C, 0x00, 0x0F, 0x00, 0x33, 0x00,
    0x33, 0x00, 0x0F, 0x00, 0x03, 0x00, 0x0F, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x30, 0x00, 0xF0, 0x00, 0xCC, 0x00,
    0xCC, 0x00, 0xF0, 0x00, 0xC0, 0x00, 0xF0, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x33, 0x03, 0x33, 0x30, 0x03, 0x03, 0x03, 0x00,
    0x03, 0x0F, 0x00, 0x0C, 0x00, 0x0C, 0x3C, 0x3C,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xCC, 0xC0, 0xCC, 0x0C, 0xC0, 0xC0, 0xC0, 0x00,
    0xC0, 0xF0, 0x00, 0x30, 0x00, 0x30, 0x3C, 0x3C,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/*------------------------------------------------------------------------
 * C64 Sprite Converter
 *------------------------------------------------------------------------*/

static u8 isBitSet(u8 b, u8 idx) {
    return ((b >> idx) & 1);
}

/*
 * Get pixel from a C64 sprite.
 * C64 sprite: 8x16 pixels (stretched to 16x16), 4 quarter tiles.
 * 2-bit color: 00=black, 01=orange, 10=grey, 11=white.
 */
static u8 getPixel(u8 chr_no, u8 tile, u8 x, u8 y) {
    u16 offset;
    u8 x_even = x & 0xFE; /* stretch by rounding to even */
    u8 row;

    if (tile >= 2) {
        tile -= 2;
        chr_no += 0x10; /* bottom half offset in C64 sprite data */
    }

    offset = (u16)chr_no * 8 + (u16)8 * tile + y;
    row = *(&c64_sprite + offset);

    {
        u8 bit0 = isBitSet(row, 7 - x_even);
        u8 bit1 = isBitSet(row, 7 - x_even - 1);
        return (bit1 << 1) | bit0;
    }
}

/*
 * Convert a C64 sprite to SNES 8bpp format into sprite_temp[].
 * Writes 256 bytes sequentially (4 tiles x 64 bytes each).
 * SNES 8bpp: 8 bitplanes per row, 4 tiles per 16x16 sprite.
 */
static void convertC64Sprite(u8 chr_no) {
    u8 num_tiles = 4;
    u8 bitplanes = 8;
    u8 t, b, mask;
    u16 x, y;
    u16 idx = 0;

    for (t = 0; t < num_tiles; t++) {
        for (b = 0; b < bitplanes; b += 2) {
            for (y = 0; y < 8; y++) {
                u8 data;

                mask = 1 << b;
                data = 0;
                for (x = 0; x < 8; x++) {
                    data = data << 1;
                    if (getPixel(chr_no, t, x, y) & mask)
                        data = data + 1;
                }
                sprite_temp[idx++] = data;

                mask = mask << 1;
                data = 0;
                for (x = 0; x < 8; x++) {
                    data = data << 1;
                    if (getPixel(chr_no, t, x, y) & mask)
                        data = data + 1;
                }
                sprite_temp[idx++] = data;
            }
        }
    }
}

/*------------------------------------------------------------------------
 * Demo Initialization
 *------------------------------------------------------------------------*/

static void refresh(void) {
    screenRefresh(VRAM_SPRITEMAP);
}

static void initDemoMap32x32(void) {
    u16 i;

    is_map32x32 = 1;
    max_scroll_width = MAX_SCROLL_WIDTH_32x32;
    max_scroll_height = MAX_SCROLL_HEIGHT_32x32;

    setMode(BG_MODE3, 0); /* BG1_TSIZE8x8 = 0 */

    initSpriteMap32x32(spritemap_len);

    /* Force blank for VRAM writes */
    REG_INIDISP = 0x80;

    /* Clear sprite 0 (empty tile) VRAM area — 4 tiles × 64 bytes = 256 bytes.
     * Without this, uninitialized VRAM shows garbage in the map interior. */
    dmaFillVRAM(0, VRAM_SPRITE_GFX, 256);

    /* DMA gargoyle sprite from ROM to VRAM for each slot.
     * Each 8bpp tile = 32 VRAM words. Sprite = 4 tiles = 128 words. */
    for (i = 1; i < 10; i++) {
        u16 vram_off = VRAM_SPRITE_GFX + element2sprite32x32(i) * 32;
        dmaCopyVram((u8*)&sprite16, vram_off, 256);
    }

    /* Load palette to CGRAM slot 0 (16 colors = 32 bytes) */
    dmaCopyCGram((u8*)&palsprite16, 0, 32);

    /* Configure BG1 */
    bgSetGfxPtr(0, VRAM_SPRITE_GFX);
    bgSetMapPtr(0, VRAM_SPRITEMAP, SC_64x64);

    /* DMA cleared tilemap to VRAM */
    smapDma(0, VRAM_SPRITEMAP, spritemap_len);
}

static void initDemoMap64x64(void) {
    u16 i;

    is_map32x32 = 0;
    max_scroll_width = MAX_SCROLL_WIDTH_64x64;
    max_scroll_height = MAX_SCROLL_HEIGHT_64x64;

    setMode(BG_MODE3, 0x10); /* BG1_TSIZE16x16 = bit 4 */

    initSpriteMap64x64(spritemap_len);

    /* Force blank for VRAM writes */
    REG_INIDISP = 0x80;

    /* Clear sprite 0 (empty tile) VRAM area.
     * In 16x16 mode: top half at VRAM_SPRITE_GFX (128 bytes),
     * bottom half at VRAM_SPRITE_GFX + 512 (128 bytes). */
    dmaFillVRAM(0, VRAM_SPRITE_GFX, 128);
    dmaFillVRAM(0, VRAM_SPRITE_GFX + 512, 128);

    /* DMA gargoyle sprite from ROM to VRAM for each slot.
     * In 16x16 tile mode: top half (128 bytes) and bottom half (128 bytes)
     * are 512 VRAM words apart (1024 bytes). */
    for (i = 1; i < 9; i++) {
        u16 vram_base = VRAM_SPRITE_GFX + element2sprite64x64(i) * 32;
        /* Top half: first 128 bytes of .pic */
        dmaCopyVram((u8*)&sprite16_64x64, vram_base, 128);
        /* Bottom half: 1024 bytes into .pic, 512 words later in VRAM */
        dmaCopyVram((u8*)&sprite16_64x64 + 1024, vram_base + 512, 128);
    }

    /* Load palette (16 colors = 32 bytes) */
    dmaCopyCGram((u8*)&palsprite16_64x64, 0, 32);

    /* Configure BG1 */
    bgSetGfxPtr(0, VRAM_SPRITE_GFX);
    bgSetMapPtr(0, VRAM_SPRITEMAP, SC_64x64);

    /* DMA cleared tilemap to VRAM */
    smapDma(0, VRAM_SPRITEMAP, spritemap_len);
}

/*------------------------------------------------------------------------
 * Draw sprite frame (border pattern)
 *------------------------------------------------------------------------*/

static void drawSpriteFrame32x32(u16 sprite) {
    u8 x, y;
    for (x = 0; x < 32; x++)
        for (y = 0; y < 32; y++)
            if (x == 0 || y == 0 || x == 31 || y == 31)
                drawSprite32x32(x, y, element2sprite32x32(sprite));
}

static void drawSpriteFrame64x64(u16 sprite) {
    u8 x, y;
    for (x = 0; x < 64; x++)
        for (y = 0; y < 64; y++)
            if (x == 0 || y == 0 || x == 63 || y == 63)
                drawSprite64x64(x, y, element2sprite64x64(sprite));
}

/*------------------------------------------------------------------------
 * Main
 *------------------------------------------------------------------------*/

int main(void) {
    short sxbg0 = 0;
    short sybg0 = 0;
    u16 pad0;
    u16 pad0_released;

    consoleInit();

    /* Set up text on BG2 (Mode 3 BG2 = 16 colors / 4bpp) */
    bgSetGfxPtr(1, VRAM_BG2_GFX);
    bgSetMapPtr(1, VRAM_BG2_MAP, SC_32x32);
    textLoadFont4bpp(VRAM_FONT);
    textInitEx(VRAM_BG2_MAP * 2, FONT_TILE_OFFSET, 1);

    /* Set text palette (palette 1, color 1 = white) */
    REG_CGADD = 17;    /* color index 17 = palette 1, color 1 */
    REG_CGDATA = 0xFF;  /* white (low byte: gggrrrrr) */
    REG_CGDATA = 0x7F;  /* white (high byte: 0bbbbbgg) */

    textPrintAt(6, 10, "DynamicMap");
    textPrintAt(6, 12, "A = Map size 32x32");
    textPrintAt(6, 14, "B = Scroll lock ON ");
    textPrintAt(6, 16, "X = Random sprite");
    textPrintAt(6, 18, "Y = Convert C64 sprite");
    textPrintAt(6, 20, "DPAD = Scroll map");
    textFlush();

    /* Init 32x32 map demo */
    initDemoMap32x32();

    /* Draw gargoyle border while still in force blank */
    drawSpriteFrame32x32(SPRITE_GARGOYLE);

    /* DMA the updated tilemap to VRAM during force blank */
    REG_INIDISP = 0x80;
    smapDma(0, VRAM_SPRITEMAP, spritemap_len);

    /* Restore text palette AFTER sprite palette (which overwrites CGRAM) */
    REG_CGADD = 17;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    /* Enable BG1 + BG2 */
    REG_TM = TM_BG1 | TM_BG2;
    setScreenOn();

    while (1) {
        pad0 = padHeld(0);
        pad0_released = padReleased(0);

        /* Scrolling */
        if (pad0 & KEY_RIGHT) {
            if (!scroll_lock)
                sxbg0 += 4;
            else if (sxbg0 < (short)max_scroll_width)
                sxbg0 += 4;
        } else if (pad0 & KEY_LEFT) {
            if (!scroll_lock)
                sxbg0 -= 4;
            else if (sxbg0 > 0)
                sxbg0 -= 4;
        }
        if (pad0 & KEY_UP) {
            if (!scroll_lock)
                sybg0 -= 4;
            else if (sybg0 > 0)
                sybg0 -= 4;
        } else if (pad0 & KEY_DOWN) {
            if (!scroll_lock)
                sybg0 += 4;
            else if (sybg0 < (short)max_scroll_height)
                sybg0 += 4;
        }

        /* Toggle map mode.
         * 1) Force blank: init sprites/palette/cleared tilemap in VRAM
         * 2) Screen ON: compute border in RAM (no VRAM writes)
         * 3) 1-page-per-VBlank tilemap DMA (smooth, no flicker) */
        if (pad0_released & KEY_A) {
            is_map32x32 = !is_map32x32;
            if (is_map32x32) {
                textPrintAt(6, 12, "A = Map size 32x32");
                textFlush();
                setScreenOff();
                initDemoMap32x32();
                REG_CGADD = 17;
                REG_CGDATA = 0xFF;
                REG_CGDATA = 0x7F;
                REG_TM = TM_BG1 | TM_BG2;
                setScreenOn();
                drawSpriteFrame32x32(SPRITE_GARGOYLE);
            } else {
                textPrintAt(6, 12, "A = Map size 64x64");
                textFlush();
                setScreenOff();
                initDemoMap64x64();
                REG_CGADD = 17;
                REG_CGDATA = 0xFF;
                REG_CGDATA = 0x7F;
                REG_TM = TM_BG1 | TM_BG2;
                setScreenOn();
                drawSpriteFrame64x64(SPRITE_GARGOYLE);
            }
            vblank_flag = 0;
            WaitForVBlank();
            smapDma(0,    VRAM_SPRITEMAP,        2048);
            WaitForVBlank();
            smapDma(2048, VRAM_SPRITEMAP + 1024, 2048);
            WaitForVBlank();
            smapDma(4096, VRAM_SPRITEMAP + 2048, 2048);
            WaitForVBlank();
            smapDma(6144, VRAM_SPRITEMAP + 3072, 2048);
            sxbg0 = 0;
            sybg0 = 0;
        }

        /* Toggle scroll lock */
        if (pad0_released & KEY_B) {
            scroll_lock = !scroll_lock;
            if (scroll_lock)
                textPrintAt(6, 14, "B = Scroll lock ON ");
            else
                textPrintAt(6, 14, "B = Scroll lock OFF");
            textFlush();
        }

        /* Random sprite placement */
        if (pad0 & KEY_X) {
            if (is_map32x32) {
                u8 rx = rand() % 31;
                u8 ry = rand() % 31;
                drawSprite32x32(rx, ry, element2sprite32x32(SPRITE_GARGOYLE));
                WaitForVBlank();
                screenRefreshPos32x32(rx, ry, VRAM_SPRITEMAP);
            } else {
                u8 rx = rand() % 63;
                u8 ry = rand() % 63;
                drawSprite64x64(rx, ry, element2sprite64x64(SPRITE_GARGOYLE));
                WaitForVBlank();
                screenRefreshPos64x64(rx, ry, VRAM_SPRITEMAP);
            }
        }

        /* Display pre-computed C64 Rockford sprite.
         * 1) DMA sprite tiles during VBlank (screen ON, 256 bytes fits)
         * 2) Compute tilemap entries during active display (RAM writes only)
         * 3) DMA tilemap directly (bypassing screenRefresh's __mul16 overhead).
         *    SC_64x64 = 4 pages × 2048 bytes. 2 pages per VBlank = 4KB DMA
         *    (~33K cycles, fits in ~41K available VBlank cycles). */
        if (pad0 & KEY_Y) {
            WaitForVBlank();
            if (is_map32x32) {
                u16 vram_off = VRAM_SPRITE_GFX + element2sprite32x32(SPRITE_ROCKFORD) * 32;
                dmaCopyVram((u8*)rockford_8bpp, vram_off, 256);
                drawSpriteFrame32x32(SPRITE_ROCKFORD);
            } else {
                u16 vram_base = VRAM_SPRITE_GFX + element2sprite64x64(SPRITE_ROCKFORD) * 32;
                dmaCopyVram((u8*)rockford_8bpp, vram_base, 128);
                dmaCopyVram((u8*)rockford_8bpp + 128, vram_base + 512, 128);
                drawSpriteFrame64x64(SPRITE_ROCKFORD);
            }
            /* Clear stale vblank_flag from computation, then DMA tilemap
             * 1 page per VBlank (2KB DMA = ~16K cycles, well within ~41K
             * available). 4 VBlanks total ≈ 67ms — imperceptible. */
            vblank_flag = 0;
            WaitForVBlank();
            smapDma(0,    VRAM_SPRITEMAP,        2048);  /* page 0 */
            WaitForVBlank();
            smapDma(2048, VRAM_SPRITEMAP + 1024, 2048);  /* page 1 */
            WaitForVBlank();
            smapDma(4096, VRAM_SPRITEMAP + 2048, 2048);  /* page 2 */
            WaitForVBlank();
            smapDma(6144, VRAM_SPRITEMAP + 3072, 2048);  /* page 3 */
        }

        bgSetScroll(0, sxbg0, sybg0);
        WaitForVBlank();
    }

    return 0;
}
