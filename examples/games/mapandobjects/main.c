/*
 * Map scrolling with object engine demo
 * Based on PVSnesLib mapandobjects example by Alekmaul
 */
#include <snes.h>
#include <snes/map.h>
#include <snes/object.h>

#include "mario.h"
#include "goomba.h"
#include "koopatroopa.h"

extern u8 tileset, tilesetend, tilesetpal, tilesetdef, tilesetatt;
extern u8 mapmario, objmario;
extern u8 palsprite;

/* ASM function that registers object callbacks with correct bank bytes */
extern void objRegisterTypes(void);

u16 nbobjects;

int main(void) {
    /* Init BG1 tileset at VRAM $2000, tilemap at $6800 (mandatory for map engine) */
    bgInitTileSet(0, &tileset, &tilesetpal, 0,
                  (&tilesetend - &tileset), 16 * 2, BG_16COLORS, 0x2000);
    bgSetMapPtr(0, 0x6800, SC_64x32);

    /* Mode 1, enable only BG1 + sprites */
    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1 | TM_OBJ;

    /* Sprite palette at CGRAM 128 (sprite palette 0) */
    dmaCopyCGram(&palsprite, 128, 16 * 2);

    /* Init dynamic sprite engine (large at $0000, small at $1000) */
    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);

    /* Object engine */
    objInitEngine();

    /* Register object type callbacks (ASM for correct bank bytes) */
    nbobjects = 1; /* mario is always object 0 */
    objRegisterTypes();

    /* Load objects from map data */
    objLoadObjects((u8 *)&objmario);

    /* Load map and do initial full-screen refresh */
    mapLoad((u8 *)&mapmario, (u8 *)&tilesetdef, (u8 *)&tilesetatt);

    /* Flush tilemap to VRAM while screen is off (force blank).
     * mapLoad sets MAP_UPD_WHOLE but doesn't write to VRAM — that happens
     * in mapVblank. Without this call, the first frame shows garbage VRAM. */
    mapVblank();

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
