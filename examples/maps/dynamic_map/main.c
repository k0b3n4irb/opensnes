/**
 * @file main.c
 * @brief Tilemap-based sprite engine with 32x32 and 64x64 map modes
 * @ingroup examples
 *
 * Demonstrates a custom tilemap sprite engine where 16x16 "sprites" are
 * drawn as background tile entries on BG1 in Mode 3 (8bpp, 256 colors).
 * This technique allows hundreds of on-screen objects without hitting the
 * 128-hardware-sprite limit, at the cost of tile-aligned positioning.
 *
 * The tilemap buffer lives in bank $7E extended WRAM (above the 8KB
 * low-WRAM limit) and is accessed via assembly helpers (smapWrite,
 * smapRead, smapDma) because the compiler generates `sta.l $0000,x`
 * which always reads bank $00. Tilemap DMA to VRAM uses the 1-page-per-
 * VBlank pattern (2KB per frame) to avoid flicker.
 *
 * Also includes a C64-to-SNES 8bpp sprite format converter that
 * demonstrates bitplane interleaving for the SNES graphics format.
 *
 * Port of the PVSnesLib DynamicMap example.
 *
 * @par SNES Concepts
 * - Mode 3: 8bpp BG1 (256 colors) + 4bpp BG2 (text overlay)
 * - Extended WRAM ($7E:2000+) for large tilemap buffers
 * - SC_64x64 tilemap layout (4 nametable pages, 8KB total)
 * - 1-page-per-VBlank DMA pattern for flicker-free tilemap updates
 * - 8bpp bitplane format and C64-to-SNES pixel conversion
 * - 16x16 tile mode (BG1_TSIZE16x16) for 64x64 map variant
 *
 * @par What to Observe
 * - A gargoyle sprite border frames the map area
 * - A = toggle between 32x32 and 64x64 map modes
 * - B = toggle scroll lock on/off
 * - X = place gargoyle sprites at random positions
 * - Y = display a converted C64 Boulder Dash Rockford sprite
 * - D-pad = scroll the map (respects scroll lock bounds)
 *
 * @par Modules Used
 * console, sprite, dma, background, text, text4bpp, input
 *
 * @see background.h, dma.h, input.h, text.h
 */

#include <snes.h>
#include "maputil.h"
#include "map32x32.h"
#include "map64x64.h"

/** @brief Tile index for an empty (cleared) cell in the sprite map */
#define SPRITE_EMPTY    0
/** @brief Tile index for the gargoyle character sprite */
#define SPRITE_GARGOYLE 1
/** @brief Tile index for the C64-converted Rockford character sprite */
#define SPRITE_ROCKFORD 2

/** @brief 8bpp gargoyle sprite tiles for 32x32 map mode (from data.asm) */
extern u8 sprite16, sprite16_end;
/** @brief Palette for gargoyle sprite in 32x32 mode */
extern u8 palsprite16, palsprite16_end;
/** @brief 8bpp gargoyle sprite tiles for 64x64 map mode (from data.asm) */
extern u8 sprite16_64x64, sprite16_64x64_end;
/** @brief Palette for gargoyle sprite in 64x64 mode */
extern u8 palsprite16_64x64, palsprite16_64x64_end;

/**
 * @brief Clear the extended WRAM tilemap buffer ($7E:2000+).
 * @param byte_count Number of bytes to zero in the tilemap buffer
 */
extern void smapClear(u16 byte_count);

/**
 * @brief DMA a portion of the WRAM tilemap buffer to VRAM.
 *
 * Because the tilemap lives in extended WRAM (bank $7E, above $2000),
 * C code cannot access it directly -- the compiler generates `sta.l $0000,x`
 * which always reads bank $00. This assembly helper sets the DMA source
 * bank to $7E and performs the transfer.
 *
 * @param byte_offset Byte offset into the WRAM tilemap buffer
 * @param vram_addr   VRAM word address destination
 * @param byte_count  Number of bytes to transfer
 */
extern void smapDma(u16 byte_offset, u16 vram_addr, u16 byte_count);

/* vblank_flag declared in <snes/system.h> (via <snes.h>) */

/** @brief VRAM word address for BG1 tilemap (SC_64x64 = 4 nametable pages) */
#define VRAM_SPRITEMAP  0x1000
/** @brief VRAM word address for BG2 tilemap (text overlay, SC_32x32) */
#define VRAM_BG2_MAP    0x6800
/** @brief VRAM word address for BG2 tile character base */
#define VRAM_BG2_GFX    0x2000
/** @brief VRAM word address for 4bpp font tiles (within BG2 gfx region) */
#define VRAM_FONT       0x3000
/** @brief VRAM word address for BG1 8bpp sprite tile characters */
#define VRAM_SPRITE_GFX 0x4000

/**
 * @brief Font tile number offset.
 *
 * Tilemap entries reference tiles relative to the BG's character base.
 * The font is loaded at VRAM_FONT but the base is VRAM_BG2_GFX, so
 * the first font tile has index (VRAM_FONT - VRAM_BG2_GFX) / 16 = 0x100.
 * Each 4bpp tile is 32 bytes = 16 VRAM words.
 */
#define FONT_TILE_OFFSET 0x100

/** @brief Total size of the SC_64x64 tilemap in bytes (4 pages x 2KB) */
u16 spritemap_len = 0x2000;
/** @brief Maximum number of unique sprite-tile entries in the map */
u16 number_of_sprites = 0x40;

/** @brief When true, scrolling is clamped to map boundaries; when false, wraps freely */
u8 scroll_lock = 1;
/** @brief When true, 32x32 map mode is active; when false, 64x64 mode */
u8 is_map32x32 = 1;

/** @brief Maximum horizontal scroll offset in pixels for the current map mode */
u16 max_scroll_width;
/** @brief Maximum vertical scroll offset in pixels for the current map mode */
u16 max_scroll_height;

/**
 * @brief Pre-computed Rockford sprite in SNES 8bpp planar format.
 *
 * This 256-byte array encodes a 16x16 sprite as 4 tiles in 8bpp planar
 * format (8 interleaved bitplanes per tile row), ready for direct
 * VRAM upload — no runtime conversion needed.
 */
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
 * Demo Initialization
 *------------------------------------------------------------------------*/

/**
 * @brief Initialize the 32x32 tilemap demo mode.
 *
 * Configures Mode 3 with 8x8 BG1 tiles, loads gargoyle sprite tiles
 * and palette to VRAM/CGRAM, clears the extended WRAM tilemap buffer,
 * and DMAs the empty tilemap to VRAM. Must be called during force blank
 * because it writes to VRAM directly.
 */
static void initDemoMap32x32(void) {
    u16 i;

    is_map32x32 = 1;
    max_scroll_width = MAX_SCROLL_WIDTH_32x32;
    max_scroll_height = MAX_SCROLL_HEIGHT_32x32;

    setMode(BG_MODE3, 0); /* BG1_TSIZE8x8 = 0 */

    initSpriteMap32x32(spritemap_len);

    /* Force blank for VRAM writes */
    setScreenOff();

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

/**
 * @brief Initialize the 64x64 tilemap demo mode with 16x16 BG tiles.
 *
 * Similar to initDemoMap32x32() but uses BG1_TSIZE16x16 (bit 4 of mode
 * register). In 16x16 tile mode, the SNES fetches the top and bottom
 * halves of each tile from VRAM addresses 512 words apart, so tile data
 * must be split between two VRAM regions per slot.
 */
static void initDemoMap64x64(void) {
    u16 i;

    is_map32x32 = 0;
    max_scroll_width = MAX_SCROLL_WIDTH_64x64;
    max_scroll_height = MAX_SCROLL_HEIGHT_64x64;

    setMode(BG_MODE3, 0x10); /* BG1_TSIZE16x16 = bit 4 */

    initSpriteMap64x64(spritemap_len);

    /* Force blank for VRAM writes */
    setScreenOff();

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

/**
 * @brief Draw a border frame of sprites around the 32x32 map perimeter.
 *
 * Fills the top, bottom, left, and right edges of the 32x32 tilemap
 * with the given sprite tile index, creating a rectangular frame.
 * Writes go to the WRAM tilemap buffer (not VRAM) -- the caller must
 * DMA the buffer to VRAM afterward.
 *
 * @param sprite Sprite element index to place at each border cell
 */
static void drawSpriteFrame32x32(u16 sprite) {
    u8 x, y;
    for (x = 0; x < 32; x++)
        for (y = 0; y < 32; y++)
            if (x == 0 || y == 0 || x == 31 || y == 31)
                drawSprite32x32(x, y, element2sprite32x32(sprite));
}

/**
 * @brief Draw a border frame of sprites around the 64x64 map perimeter.
 *
 * Same as drawSpriteFrame32x32() but for the larger 64x64 map mode.
 * Uses the 64x64 coordinate system and tile mapping functions.
 *
 * @param sprite Sprite element index to place at each border cell
 */
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

/**
 * @brief Main entry point -- dynamic tilemap sprite engine demo.
 *
 * Initializes Mode 3 (8bpp BG1) with a text overlay on BG2, sets up the
 * extended WRAM tilemap buffer via assembly helpers, and enters a loop
 * handling scrolling, map mode toggling (32x32 / 64x64), random sprite
 * placement, and C64 sprite conversion display. Tilemap updates to VRAM
 * use the 1-page-per-VBlank pattern (2KB per frame) for flicker-free DMA.
 *
 * @return 0 (never reached -- infinite game loop)
 */
int main(void) {
    s16 sxbg0 = 0;
    s16 sybg0 = 0;
    u16 pad0;
    u16 pad0_released;

    consoleInit();

    /* Set up text on BG2 (Mode 3 BG2 = 16 colors / 4bpp) */
    bgSetGfxPtr(1, VRAM_BG2_GFX);
    bgSetMapPtr(1, VRAM_BG2_MAP, SC_32x32);
    textLoadFont4bpp(VRAM_FONT);
    textInit(VRAM_BG2_MAP, FONT_TILE_OFFSET, 1);

    /* Set text palette (palette 1, color 1 = white) */
    setColor(17, RGB(31, 31, 31));  /* palette 1, color 1 = white */

    textPrintAt(6, 10, "DynamicMap");
    textPrintAt(6, 12, "A = Map size 32x32");
    textPrintAt(6, 14, "B = Scroll lock ON ");
    textPrintAt(6, 16, "X = Random sprite");
    textPrintAt(6, 18, "Y = Convert C64 sprite");
    textPrintAt(6, 20, "DPAD = Scroll map");

    /* Init 32x32 map demo */
    initDemoMap32x32();

    /* Draw gargoyle border while still in force blank */
    drawSpriteFrame32x32(SPRITE_GARGOYLE);

    /* DMA the updated tilemap to VRAM during force blank */
    setScreenOff();
    smapDma(0, VRAM_SPRITEMAP, spritemap_len);

    /* Restore text palette AFTER sprite palette (which overwrites CGRAM) */
    setColor(17, RGB(31, 31, 31));

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
            else if (sxbg0 < (s16)max_scroll_width)
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
            else if (sybg0 < (s16)max_scroll_height)
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
                setScreenOff();
                initDemoMap32x32();
                setColor(17, RGB(31, 31, 31));
                REG_TM = TM_BG1 | TM_BG2;
                setScreenOn();
                drawSpriteFrame32x32(SPRITE_GARGOYLE);
            } else {
                textPrintAt(6, 12, "A = Map size 64x64");
                setScreenOff();
                initDemoMap64x64();
                setColor(17, RGB(31, 31, 31));
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
