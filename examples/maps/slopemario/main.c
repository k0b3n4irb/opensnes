/**
 * Slope Mario — Platformer with slope collision
 *
 * Demonstrates objCollidMapWithSlopes() for diagonal terrain.
 * Mario walks, jumps, and slides on slopes with proper physics.
 * No audio — focuses on slope collision stress testing.
 *
 * Based on PVSnesLib slopemario example by Nub1604.
 */
#include <snes.h>
#include <snes/map.h>
#include <snes/object.h>

#include "mario.h"

extern u8 tileset, tilesetend, tilepal, tilesetdef, tilesetatt;
extern u8 mapmario, objmario;

/* ASM function that registers Mario callback with correct bank byte */
extern void objRegisterTypes(void);

int main(void) {
    /* Init BG1 tileset at VRAM $2000, tilemap at $6800 (mandatory for map engine) */
    bgInitTileSet(0, &tileset, &tilepal, 0,
                  (&tilesetend - &tileset), 16 * 2, BG_16COLORS, 0x2000);
    bgSetMapPtr(0, 0x6800, SC_64x32);

    /* Mode 1, enable BG1 + sprites */
    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1 | TM_OBJ;

    /* Init dynamic sprite engine (large at $0000, small at $1000) */
    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);

    /* Object engine */
    objInitEngine();

    /* Register Mario object type (ASM for correct bank bytes) */
    objRegisterTypes();

    /* Load objects from map data */
    objLoadObjects((u8 *)&objmario);

    /* Load map and do initial full-screen refresh */
    mapLoad((u8 *)&mapmario, (u8 *)&tilesetdef, (u8 *)&tilesetatt);

    /* Flush tilemap to VRAM while screen is off */
    mapVblank();

    /* Flush initial sprite frame to VRAM before screen on */
    oamDynamic16Draw(0);
    oamVramQueueUpdate();

    setScreenOn();

    while (1) {
        mapUpdate();
        objUpdateAll();

        oamInitDynamicSpriteEndFrame();
        WaitForVBlank();
        mapVblank();
        oamVramQueueUpdate();
    }
    return 0;
}
