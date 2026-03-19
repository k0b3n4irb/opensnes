/**
 * @file main.c
 * @brief Tiled map editor integration with the map scrolling engine
 * @ingroup examples
 *
 * Demonstrates how to use maps created in the Tiled editor
 * (https://www.mapeditor.org/) with the OpenSNES map engine. The map
 * is a large 224x30 tile level (1792x240 pixels) stored in ROM. The
 * map engine streams the visible 32-tile window into VRAM as the
 * camera scrolls, so only the on-screen portion uses VRAM at any time.
 *
 * The Tiled workflow:
 * 1. Design your level in Tiled (paint tiles, set collision properties)
 * 2. Export as .tmj (JSON) or .tmx (XML)
 * 3. Convert with tmxconv to .m16 (tilemap) + .t16 (tile defs) + .b16 (attributes)
 * 4. Convert tileset PNG with gfx4snes to .pic + .pal
 * 5. Include all binaries in data.asm
 *
 * This example uses pre-converted binaries. The original Tiled source
 * file (maplevel01.tmj) is included in res/ for reference.
 *
 * Ported from PVSnesLib "tiled" example by alekmaul.
 *
 * @par SNES Concepts
 * - Map engine: mapLoad() / mapUpdate() / mapUpdateCamera() / mapVblank()
 * - Streaming tilemap: only 32 visible columns are in VRAM at any time
 * - SC_64x32 tilemap layout at VRAM $6800 (required by map engine)
 * - Tile data at VRAM $2000 (4bpp, 16-color)
 * - Horizontal camera scrolling with pixel-level precision
 * - VBlank DMA for tilemap updates (mapVblank)
 *
 * @par What to Observe
 * - Press LEFT/RIGHT on D-pad to scroll through the level
 * - The background scrolls smoothly at 1 pixel per frame
 * - Notice the level is much wider than the screen (1792px vs 256px)
 * - The tileset uses platformer-style tiles (platforms, ladders, walls)
 *
 * @par Modules Used
 * console, sprite, dma, input, background, map
 *
 * @see map.h, background.h, input.h, dma.h
 */

#include <snes.h>
#include <snes/map.h>

/*============================================================================
 * External Assets (defined in data.asm)
 *============================================================================*/

/** @brief 4bpp tileset graphics (converted from tileslevel1.png) */
extern u8 tileset[], tileset_end[];

/** @brief Tileset palette (3 palette banks x 16 colors = 96 bytes) */
extern u8 tilesetpal[], tilesetpal_end[];

/** @brief Full level tilemap — 224x30 tiles, 16-bit entries (from BG1.m16) */
extern u8 mapdata[];

/** @brief Metatile definitions — 4 tile indices per metatile (from .t16) */
extern u8 tilesetdef[];

/** @brief Tile collision attributes — one property byte per metatile (from .b16) */
extern u8 tilesetatt[];

/*============================================================================
 * Game State
 *============================================================================*/

/** @brief Camera X position in map pixels. Controls which part of the level is visible. */
s16 mapscx;

/**
 * @brief Entry point — initialize hardware, load map, scroll with D-pad
 * @return Never returns (infinite game loop)
 */
int main(void) {
    u16 pad;

    /*
     * Step 1: Load tileset to VRAM
     *
     * bgInitTileSet loads tile graphics to VRAM $2000 and palette to CGRAM.
     * The palette size (16*2*3 = 96 bytes) covers 3 palette banks of 16 colors,
     * matching the gfx4snes -o 48 flag used during conversion.
     *
     * The tilemap address $6800 with SC_64x32 is required by the map engine —
     * it uses this specific VRAM region for streaming.
     */
    bgInitTileSet(0, tileset, tilesetpal, 0,
                  tileset_end - tileset,
                  tilesetpal_end - tilesetpal,
                  BG_16COLORS, 0x2000);
    bgSetMapPtr(0, 0x6800, SC_64x32);

    /* Step 2: Set Mode 1, enable only BG1 */
    setMode(BG_MODE1, 0);
    setMainScreen(LAYER_BG1);

    /*
     * Step 3: Load the full map into the map engine (screen still off)
     *
     * mapLoad copies the 224x30 tilemap, metatile definitions, and collision
     * attributes into extended WRAM (bank $7E). It also performs the initial
     * full-screen tilemap refresh to VRAM.
     *
     * This MUST happen before setScreenOn() — otherwise the tilemap write
     * is visible as garbage for 1-2 frames.
     */
    mapLoad(mapdata, tilesetdef, tilesetatt);

    /* Step 4: Flush the initial tilemap to VRAM before screen on.
     * The map engine updates one column per mapUpdate() cycle, so two
     * full update+flush cycles ensure every visible tile is in VRAM. */
    mapUpdate();
    WaitForVBlank();
    mapVblank();
    mapUpdate();
    WaitForVBlank();
    mapVblank();

    /* Step 5: Screen on — all VRAM is ready */
    setScreenOn();

    /*
     * Step 5: Main loop — scroll the camera with D-pad
     *
     * The camera X position (mapscx) is in map pixel coordinates.
     * The map engine handles converting this to tile-aligned VRAM updates.
     *
     * Bounds: 128px (half screen) to 1664px (map width - half screen)
     */
    mapscx = 16 * 8;

    while (1) {
        /* Update the visible tilemap window based on camera position */
        mapUpdate();

        /* Read input and move camera */
        pad = padHeld(0);

        if (pad & KEY_LEFT) {
            if (mapscx > 16 * 8)
                mapscx--;
        }
        if (pad & KEY_RIGHT) {
            if (mapscx < 208 * 8)
                mapscx++;
        }

        /* Tell the map engine where the camera is */
        mapUpdateCamera(mapscx, 0);

        /* Wait for VBlank, then DMA the tilemap updates to VRAM */
        WaitForVBlank();
        mapVblank();
    }

    return 0;
}
