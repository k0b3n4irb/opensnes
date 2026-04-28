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
 * Three object types are registered via assembly callbacks (for correct
 * bank byte resolution): Mario (player-controlled), Goomba (walks and
 * reverses at walls), and Koopa Troopa (patrols with shell behavior).
 * Object spawn positions are loaded from the map's object layer.
 *
 * The dynamic sprite engine handles animated 16x16 sprites with automatic
 * VRAM tile management, freeing the game logic from manual tile uploads.
 *
 * Port of the PVSnesLib mapandobjects example by Alekmaul.
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
 * - Mario walks and jumps through a scrolling level
 * - Goombas walk back and forth, reversing at obstacles
 * - Koopa Troopas patrol with distinct movement patterns
 * - Camera scrolls to follow Mario's position
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

#include "mario.h"
#include "goomba.h"
#include "koopatroopa.h"

/** @brief BG1 tileset tile data (4bpp) -- start label */
extern u8 tileset;
/** @brief BG1 tileset tile data -- end label (for size calculation) */
extern u8 tilesetend;
/** @brief BG1 tileset palette (BGR555, 16 colors) */
extern u8 tilesetpal;
/** @brief Tile definition table (visual properties per tile index) */
extern u8 tilesetdef;
/** @brief Tile attribute table (collision flags per tile index) */
extern u8 tilesetatt;
/** @brief Map data (tile indices for the scrolling level layout) */
extern u8 mapmario;
/** @brief Object layer data (spawn positions and type IDs for entities) */
extern u8 objmario;
/** @brief Shared sprite palette for all entity types (BGR555) */
extern u8 palsprite;

/**
 * @brief Register all object type callbacks (Mario, Goomba, Koopa) with correct bank bytes.
 *
 * Each object type has an update callback function pointer stored with its
 * ROM bank byte. Since C cannot express the `:label` bank-byte operator,
 * this assembly function handles registration for all three entity types.
 */
extern void objRegisterTypes(void);

/**
 * @brief Running count of active objects in the level.
 *
 * Initialized to 1 (Mario is always object 0) and incremented as enemies
 * are loaded from the map's object layer by objLoadObjects().
 */
u16 nbobjects;

/**
 * @brief Main entry point -- scrolling platformer with map and object engines.
 *
 * Initializes the map engine (BG1 tileset + tilemap), object engine (entity
 * registration and spawning), and dynamic sprite engine (animated 16x16
 * sprites). The main loop follows the standard OpenSNES game pipeline:
 *
 * Active display: mapUpdate() + objUpdateAll() + sprite end-frame
 * VBlank: mapVblank() + oamVramQueueUpdate()
 *
 * Object callbacks (Mario, Goomba, Koopa Troopa) handle their own physics,
 * collision, animation, and sprite drawing within objUpdateAll().
 *
 * @return 0 (never reached -- infinite game loop)
 */
int main(void) {
    /* Init BG1 tileset at VRAM $2000, tilemap at $6800 (mandatory for map engine) */
    bgInitTileSet(0, &tileset, &tilesetpal, 0,
                  (&tilesetend - &tileset), PALETTE_16_SIZE, BG_16COLORS, 0x2000);
    bgSetMapPtr(0, 0x6800, SC_64x32);

    /* Mode 1, enable only BG1 + sprites */
    setMode(BG_MODE1, 0);
    setMainScreen(TM_BG1 | TM_OBJ);

    /* Sprite palette at CGRAM 128 (sprite palette 0) */
    dmaCopyCGram(&palsprite, OBJ_CGRAM_BASE, PALETTE_16_SIZE);

    /* Init dynamic sprite engine (large at $0000, small at $1000) */
    {
        static const OamDynamicConfig dyn_cfg = {
            .vramLarge     = 0x0000,
            .vramSmall     = 0x1000,
            .slotLargeInit = 0,
            .slotSmallInit = 0,
            .sizeMode      = OBJ_SIZE8_L16,
        };
        oamDynamicInit(&dyn_cfg);
    }

    /* Object engine */
    objInitEngine();

    /* Register object type callbacks (ASM for correct bank bytes) */
    nbobjects = 1; /* mario is always object 0 */
    objRegisterTypes();

    /* Load objects from map data */
    objLoadObjects((u8 *)&objmario);

    /* Load map and do initial full-screen refresh */
    mapLoad((u8 *)&mapmario, (u8 *)&tilesetdef, (u8 *)&tilesetatt);

    /* mapLoad flushes VRAM internally — screen on */
    setScreenOn();

    while (1) {
        mapUpdate();
        objUpdateAll();

        WaitForVBlank();
        mapVblank();
        /* NMI handler auto-flushes the dynamic sprite engine. */
    }
    return 0;
}
