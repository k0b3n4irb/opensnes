/**
 * @file main.c
 * @brief Scrolling platformer with object engine and enemy AI
 * @ingroup examples
 *
 * Demonstrates the OpenSNES map engine and object engine working together
 * to create a scrolling platformer with multiple entity types. The map
 * engine handles tile-based scrolling and VRAM streaming, while the
 * object engine manages entity lifecycle, physics, collision callbacks,
 * and per-frame update dispatch.
 *
 * Three object types are registered from C: Kit (player-controlled, slope
 * collision), the robot insect and the patrol automaton (both patrol
 * between two x read from the map's object layer, and turn at each end).
 * Object spawn positions are loaded from the map's object layer. The level
 * is roofs with ramps at 45 and 22.5 degrees and vents to jump over, the
 * far towers behind on BG2.
 *
 * The dynamic sprite engine handles animated 32x32 sprites with automatic
 * VRAM tile management, freeing the game logic from manual tile uploads.
 *
 * This began as the PVSnesLib mapandobjects example by Alekmaul; the level,
 * its tiles and the three characters are ours (devtools/art_assets/build.py
 * writes the level from assets/neo-lumen/).
 *
 * @par SNES Concepts
 * - Map engine: tile scrolling with mapLoad/mapUpdate/mapVblank pipeline
 * - Object engine: entity registration, spawn from map data, update callbacks
 * - Dynamic sprite engine: automatic VRAM tile allocation for animated sprites
 * - Multiple sprite palettes via CGRAM bank indexing
 * - SC_64x32 tilemap for horizontal scrolling levels
 * - Assembly bank byte resolution for cross-bank function pointers
 *
 * @par What to Observe
 * - Kit walks up and down the ramps and jumps over the vents
 * - Robot insects and automatons patrol back and forth on the roofs
 * - The far towers behind scroll at a quarter of the camera's speed
 * - Camera scrolls to follow Kit's position
 * - All entities animate independently with the dynamic sprite engine
 *
 * @par Modules Used
 * console, sprite, sprite_dynamic, sprite_lut, dma, input, background, map, object
 *
 * @see map.h, object.h, sprite.h, background.h, input.h
 */
#include <snes.h>
#include <snes/map.h>
#include <snes/object.h>

#include "kit.h"
#include "insect.h"
#include "automaton.h"

/** @brief BG1 tileset tile data (4bpp) -- start label */
#include "res/tiles.inc"         /* generated: tiles_tiles[], tiles_pal[], tiles_map[], each with _end */
#include "res/kit.inc"           /* generated: kit_tiles[], kit_pal[] */
#include "res/insect.inc"        /* generated: insect_tiles[], insect_pal[] (the machines' palette) */
#include "res/automaton.inc"     /* generated: automaton_tiles[], automaton_pal[] (the same palette) */
#include "res/skyline.inc"       /* generated: skyline_tiles[], skyline_pal[], skyline_map[] (64x32 in two pages) */
/** @brief BG1 tileset tile data -- end label (for size calculation) */
/** @brief BG1 tileset palette (BGR555, 16 colors) */
/** @brief Tile definition table (visual properties per tile index) */
extern u8 tilesetdef;
/** @brief Tile attribute table (collision flags per tile index) */
extern u8 tilesetatt;
/** @brief Map data (tile indices for the scrolling level layout) */
extern u8 maplevel;
/** @brief Object layer data (spawn positions and type IDs for entities) */
extern u8 objlevel;
/** @brief Shared sprite palette for all entity types (BGR555) */


/**
 * @brief Running count of active objects in the level.
 *
 * Initialized to 1 (Kit is always object 0) and incremented as enemies
 * are loaded from the map's object layer by objLoadObjects().
 */
u16 nbobjects;

/**
 * @brief Main entry point -- scrolling platformer with map and object engines.
 *
 * Initializes the map engine (BG1 tileset + tilemap), object engine (entity
 * registration and spawning), and dynamic sprite engine (animated 32x32
 * sprites). The main loop follows the standard OpenSNES game pipeline:
 *
 * Active display: mapUpdate() + objUpdateAll() + sprite end-frame
 * VBlank: mapVblank() + oamVramQueueUpdate()
 *
 * Object callbacks (Kit, the insect, the automaton) handle their own physics,
 * collision, animation, and sprite drawing within objUpdateAll().
 *
 * @return 0 (never reached -- infinite game loop)
 */
int main(void) {
    consoleInit();

    /* Init BG1 tileset at VRAM $2000, tilemap at $6800 (mandatory for map engine) */
    bgInitTileSet(0, tiles_tiles, tiles_pal, 0,
                  (tiles_tiles_end - tiles_tiles), PALETTE_16_SIZE, BG_16COLORS, 0x2000);
    bgSetMapPtr(0, 0x6800, SC_64x32);

    /* Mode 1: BG1 the level, BG2 the far towers, and the sprites */
    setMode(BG_MODE1, 0);
    /* BG2: the far towers, in the dynamic engine's small-sprite pool at
     * $1000 (this game draws only large sprites), map at $7000 */
    bgSetGfxPtr(1, 0x1000);
    bgSetMapPtr(1, 0x7000, SC_64x32);
    dmaCopyVram(skyline_tiles, 0x1000, skyline_tiles_end - skyline_tiles);
    dmaCopyVram(skyline_map, 0x7000, skyline_map_end - skyline_map);
    dmaCopyCGram(skyline_pal, 16, skyline_pal_end - skyline_pal);

    setMainScreen(TM_BG1 | TM_BG2 | TM_OBJ);

    /* Sprite palette at CGRAM 128 (sprite palette 0) */
    dmaCopyCGram(kit_pal, OBJ_CGRAM_BASE, PALETTE_16_SIZE);
    /* the machines' palette: OBJ palette 1 (the insect's and the automaton's are the same) */
    dmaCopyCGram(insect_pal, OBJ_CGRAM_BASE + 16, PALETTE_16_SIZE);

    /* Init dynamic sprite engine (large at $0000, small at $1000) */
    {
        static const OamDynamicConfig dyn_cfg = {
            .vramLarge     = 0x0000,
            .vramSmall     = 0x1000,
            .slotLargeInit = 0,
            .slotSmallInit = 0,
            .sizeMode      = OBJ_SIZE16_L32,
        };
        oamDynamicInit(&dyn_cfg);
    }

    /* Object engine */
    objInitEngine();

    /* Register the three object types. The callbacks are C functions the
     * linker may put in any bank; a function pointer carries its bank, so
     * they register from C as they are. */
    nbobjects = 1; /* Kit is always object 0 */
    objInitFunctions(0, kitinit,          kitupdate,          0);
    objInitFunctions(1, insectinit,       insectupdate,       0);
    objInitFunctions(2, automatoninit,    automatonupdate,    0);

    /* Load objects from map data */
    objLoadObjects((u8 *)&objlevel);

    /* Load map and do initial full-screen refresh */
    mapLoad((u8 *)&maplevel, (u8 *)&tilesetdef, (u8 *)&tilesetatt);

    /* mapLoad flushes VRAM internally — screen on */
    setScreenOn();

    while (1) {
        mapUpdate();
        objUpdateAll();
        bgSetScroll(1, mapGetCameraX() >> 2, 0);   /* the far towers: a quarter of the speed */

        WaitForVBlank();
        mapVblank();
        /* NMI handler auto-flushes the dynamic sprite engine. */
    }
    return 0;
}
