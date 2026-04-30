/**
 * @file main.c
 * @brief Scrolling map with animated sprite character
 * @ingroup examples
 *
 * A large Tiled map scrolls as a Mario sprite moves left and right.
 * The map engine streams only the visible tiles to VRAM, so the map
 * can be much wider than the screen. The sprite animates between 4
 * walk frames and flips horizontally based on direction.
 *
 * This is the stepping stone between "static background" and
 * "continuous_scroll" — it teaches the map engine basics with a
 * simple player character.
 *
 * Ported from PVSnesLib "mapscroll" example by alekmaul.
 *
 * @par SNES Concepts
 * - Map engine: mapLoad / mapUpdate / mapUpdateCamera / mapVblank
 * - Camera-follow: sprite position drives the viewport
 * - Sprite animation: 4-frame walk cycle with horizontal flip
 * - oamSet for simple single-sprite rendering
 * - x_pos / y_pos camera exports for screen-relative sprite positioning
 *
 * @par What to Observe
 * - Press LEFT/RIGHT to move Mario through the level
 * - The background scrolls to follow the character
 * - Mario's walk animation cycles every 2 frames of movement
 * - Mario flips horizontally when changing direction
 *
 * @par Modules Used
 * console, sprite, dma, input, background, map
 *
 * @see map.h, sprite.h, input.h
 */

#include <snes.h>
#include <snes/map.h>

/*============================================================================
 * External Assets (defined in data.asm)
 *============================================================================*/

/** @brief 4bpp tileset graphics */
extern u8 tileset[], tileset_end[];

/** @brief Tileset palette (3 banks x 16 colors) */
extern u8 tilesetpal[], tilesetpal_end[];

/** @brief Full level tilemap from tmx2snes */
extern u8 mapdata[];

/** @brief Metatile definitions */
extern u8 tilesetdef[];

/** @brief Tile collision attributes */
extern u8 tilesetatt[];

/** @brief 16x16 Mario sprite tiles (4 animation frames) */
extern u8 gfxsprite[], gfxsprite_end[];

/** @brief Mario sprite palette */
extern u8 palsprite[], palsprite_end[];

/*============================================================================
 * Game State
 *============================================================================*/

/** @brief Walk frame tile offsets: each 16x16 sprite = 2 tiles in VRAM */
static const u8 sprTiles[4] = {0, 2, 4, 6};

/** @brief Mario X position in map pixels */
u16 xloc;

/** @brief Mario Y position in map pixels */
u16 yloc;

/** @brief Current animation frame index (0-3) */
u16 frameidx;

/** @brief Current sprite tile number */
u16 frame;

/** @brief Horizontal flip flag (1 = facing left) */
u16 flipx;

/** @brief Animation toggle (advance every other input frame) */
u16 flip;

/**
 * @brief Entry point — scrolling map with animated Mario
 * @return Never returns (infinite game loop)
 */
int main(void) {
    u16 pad;

    /* Load tileset to VRAM $2000, palette to CGRAM */
    bgInitTileSet(0, tileset, tilesetpal, 0,
                  tileset_end - tileset,
                  tilesetpal_end - tilesetpal,
                  BG_16COLORS, 0x2000);

    /* Map engine requires tilemap at $6800, SC_64x32 */
    bgSetMapPtr(0, 0x6800, SC_64x32);

    /* Mode 1, BG1 + sprites */
    setMode(BG_MODE1, 0);
    setMainScreen(LAYER_BG1 | LAYER_OBJ);

    /* Load sprite tiles to VRAM $0000, palette to CGRAM 128 */
    oamInitGfxSet(gfxsprite, gfxsprite_end - gfxsprite,
                  palsprite, palsprite_end - palsprite,
                  0, 0x0000, OBJ_SIZE16_L32);

    /* Load map (screen still off — no garbage) */
    mapLoad(mapdata, tilesetdef, tilesetatt);

    /* Initial position: near left edge, standing on ground (row 26 = y 208) */
    xloc = 16 * 8;
    yloc = 208 - 16;
    frameidx = 0;
    frame = 0;
    flipx = 0;
    flip = 0;

    mapUpdateCamera(xloc, yloc);

    /* Screen on — mapLoad flushes VRAM internally */
    setScreenOn();

    /* Main loop */
    while (1) {
        pad = padHeld(0);

        /* Move left */
        if (pad & KEY_LEFT) {
            if (xloc > 0) xloc--;
            flipx = 1;
            mapUpdateCamera(xloc, yloc);
            if (flip++ & 1) {
                frameidx = (frameidx + 1) & 3;
                frame = sprTiles[frameidx];
            }
        }

        /* Move right */
        if (pad & KEY_RIGHT) {
            xloc++;
            flipx = 0;
            mapUpdateCamera(xloc, yloc);
            if (flip++ & 1) {
                frameidx = (frameidx + 1) & 3;
                frame = sprTiles[frameidx];
            }
        }

        /* Draw Mario relative to camera position
         * oamSet(id, x, y, tile, palette, priority, flags)
         * flags bit 6 = horizontal flip */
        oamSet(0, xloc - x_pos, yloc - y_pos, frame, 0, 3,
               flipx ? OBJ_FLIPX : 0);

        /* Update map engine */
        mapUpdate();
        WaitForVBlank();
        mapVblank();
    }

    return 0;
}
