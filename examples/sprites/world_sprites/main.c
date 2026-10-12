/**
 * @file main.c
 * @brief World-space sprites — a camera, culling, a depth sort and OAM in one call per batch
 * @ingroup examples
 *
 * A scrolling game keeps its actors in WORLD coordinates (a 512 x 448
 * field here) and shows a 256 x 224 window of it. Every frame it has to
 * subtract the camera from each actor, decide whether the result is on
 * screen, write the OAM entry or hide it, and set the ninth x bit for a
 * sprite half out on the left. oamPlaceWorld() does it
 * in assembly for a whole batch, about 1,700 a sprite on the library bench
 * (`docs/PERF.md`, row `world`).
 *
 * The game keeps one array per property (structure of arrays: what the
 * 65816 indexes cheaply) and fills an OamWorldBatch with their addresses
 * once. Two batches, because a batch has one size: 17 coins of 16 x 16
 * (the SMALL size of OBJ_SIZE16_L32) and six walkers of 32 x 32 (the LARGE
 * size, set once with oamSetSize(): the call never touches the size bit).
 *
 * The walkers show `order`. Seen from above, the one lower on the screen
 * is nearer and must be drawn in front — and OAM order is drawing order,
 * the lower slot in front. So the walkers are sorted by y every frame
 * (an insertion sort of six bytes) and the batch's `order` says which
 * walker each slot shows: the arrays stay indexed by the walker, only the
 * six bytes of `order` move.
 *
 * Hold the D-pad to move the camera. `visible[]` comes back from the call:
 * the example counts the coins in view with it (`world_visible`); a game
 * reads it to stream the animation frames of the sprites that are shown
 * and skip the others — a sprite off screen should cost no VRAM DMA.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - World coordinates minus a camera = screen coordinates; a sprite partly
 *   off the left edge needs the ninth x bit (x in 256..511), which the PPU
 *   reads as negative
 * - OAM culling: a sprite off screen is hidden (x = 257, y = 240), not left
 *   wherever it was
 * - OAM order is drawing order: a depth sort permutes slots, not data
 * - Structure of arrays for per-sprite data, and the size bit set once
 *
 * @par What to Observe
 * Six square walkers pace up and down near the centre and pass in front
 * of and behind one another as their y changes; gold coins drift over the
 * field; the D-pad scrolls the camera and stops at the world's edge.
 * `world_visible` (probe oracle) follows how many coins are in view,
 * `world_front` which walker is in the front slot, `cam_x` / `cam_y` the
 * camera.
 *
 * @par Modules Used
 * console, dma, background, sprite, input, tile
 *
 * @see lib/include/snes/sprite.h — OamWorldBatch, oamPlaceWorld, oamSetSize
 * @see examples/sprites/sprite_swarm — the same sprites moved by direct OAM writes, no camera
 */

#include <snes.h>
#include <snes/tile.h>   /* tileEncode4bpp */

/** @brief Coins in the world (the 16 x 16 batch). */
#define NCOINS 17
/** @brief Walkers (the 32 x 32 batch, sorted by depth). */
#define NWALK 6
/** @brief The walkers' OAM ids follow the coins'. */
#define WALK_ID NCOINS
/** @brief Sprite tile VRAM word address. */
#define SPR_VRAM 0x4000
/** @brief World size in pixels. */
#define WORLD_W 512
#define WORLD_H 448
/** @brief The camera may not show past the world's edge. */
#define CAM_XMAX (WORLD_W - 256)
#define CAM_YMAX (WORLD_H - 224)

/** @brief Probe oracles: the camera, the coins placed this frame, the walker in the front slot. */
u16 cam_x;
u16 cam_y;
u16 world_visible;
u16 world_front;

/* Per-sprite data, one array per property: what OamWorldBatch points at.
 * The coins' arrays are the first NCOINS entries, the walkers' the rest. */
static s16 wx[NCOINS + NWALK];     /**< world x of the top-left corner */
static s16 wy[NCOINS + NWALK];     /**< world y */
static u8  wtile[NCOINS + NWALK];  /**< tile name, low byte */
static u8  wattr[NCOINS + NWALK];  /**< vhoopppc, as OAM_ATTR() builds it */
static u8  wseen[NCOINS + NWALK];  /**< out: 1 placed, 0 hidden */
static s8  wvx[NCOINS + NWALK];    /**< drift */
static s8  wvy[NCOINS + NWALK];

/** @brief The walkers' drawing order: slot i of the batch shows walker order[i]. */
static u8 walk_order[NWALK];

/** @brief The two batches: the coins (16 x 16) and the walkers (32 x 32, sorted). */
static OamWorldBatch coins;
static OamWorldBatch walkers;

/** @brief Scratch: an 8 x 8 tile as pixel indices, and its 4bpp form. */
static u8 px[64];
static u8 tilebuf[32];

/** @brief Encode one 8 x 8 shape (8 rows of bits, colour index 1) to VRAM name `name`,
 *  mirrored left-right (`flipx`) and / or top-bottom (`flipy`): a big sprite has ONE
 *  attribute byte for all its tiles, so each corner and edge is its own tile. */
static void load_shape(const u8 *rows, u16 name, u8 flipx, u8 flipy) {
    u8 r, c;
    for (r = 0; r < 8; r++)
        for (c = 0; c < 8; c++) {
            u8 sr = flipy ? (u8)(7 - r) : r;
            u8 sc = flipx ? (u8)(7 - c) : c;
            px[r * 8 + c] = (u8)(((rows[sr] >> (7 - sc)) & 1) ? 1 : 0);
        }
    tileEncode4bpp(px, tilebuf);
    dmaCopyVram(tilebuf, (u16)(SPR_VRAM + name * 16), 32);
}

/** @brief Sort the walkers by y, the lowest on screen first: an insertion sort of six bytes. */
static void sort_walkers(void) {
    u8 i, j, id;
    for (i = 1; i < NWALK; i++) {
        id = walk_order[i];
        j = i;
        while (j > 0 && wy[WALK_ID + walk_order[j - 1]] < wy[WALK_ID + id]) {
            walk_order[j] = walk_order[j - 1];
            j--;
        }
        walk_order[j] = id;
    }
}

int main(void) {
    /* The 16 x 16 coin: a disc, as four 8 x 8 quadrants mirrored from one. */
    static const u8 disc_rows[8]   = {0x07, 0x1F, 0x3F, 0x7F, 0x7F, 0xFF, 0xFF, 0xFF};
    /* The 32 x 32 walker: a frame of corners and edges, inner tiles empty. */
    static const u8 corner_rows[8] = {0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0};
    static const u8 top_rows[8]    = {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static const u8 left_rows[8]   = {0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0};
    static const u8 blank_rows[8]  = {0, 0, 0, 0, 0, 0, 0, 0};
    static const s8 vtab[4] = {-1, 1, -2, 2};
    u16 pal[128];
    u16 i;
    u16 held;

    consoleInit();
    setMode(BG_MODE1, 0);

    /* Sprite palettes live at CGRAM 128+: palette 0 gold for the coins,
     * palettes 1-6 one tint per walker. Only colour 1 of each is used. */
    for (i = 0; i < 128; i++) pal[i] = 0;
    pal[0 * 16 + 1] = RGB(31, 26, 6);
    pal[1 * 16 + 1] = RGB(31, 31, 31);
    pal[2 * 16 + 1] = RGB(31, 10, 10);
    pal[3 * 16 + 1] = RGB(10, 31, 10);
    pal[4 * 16 + 1] = RGB(12, 16, 31);
    pal[5 * 16 + 1] = RGB(31, 16, 31);
    pal[6 * 16 + 1] = RGB(12, 31, 31);
    dmaCopyCGram((u8 *)pal, OBJ_CGRAM_BASE, 256);

    /* A 32 x 32 sprite reads its tiles four to a row, rows 16 names apart:
     * names 0-3, 16-19, 32-35, 48-51. A 16 x 16 one reads two to a row:
     * the coin is names 4, 5, 20, 21. */
    load_shape(corner_rows, 0, 0, 0);  load_shape(top_rows, 1, 0, 0);
    load_shape(top_rows, 2, 0, 0);     load_shape(corner_rows, 3, 1, 0);
    load_shape(left_rows, 16, 0, 0);   load_shape(blank_rows, 17, 0, 0);
    load_shape(blank_rows, 18, 0, 0);  load_shape(left_rows, 19, 1, 0);
    load_shape(left_rows, 32, 0, 0);   load_shape(blank_rows, 33, 0, 0);
    load_shape(blank_rows, 34, 0, 0);  load_shape(left_rows, 35, 1, 0);
    load_shape(corner_rows, 48, 0, 1); load_shape(top_rows, 49, 0, 1);
    load_shape(top_rows, 50, 0, 1);    load_shape(corner_rows, 51, 1, 1);
    load_shape(disc_rows, 4, 0, 0);    load_shape(disc_rows, 5, 1, 0);
    load_shape(disc_rows, 20, 0, 1);   load_shape(disc_rows, 21, 1, 1);

    oamInit(OBJ_SIZE16_L32, SPR_VRAM >> 13);
    oamClear();

    /* The coins: spread over the world, each with a drift. */
    for (i = 0; i < NCOINS; i++) {
        wx[i] = (s16)((i % 6) * 80 + 24);
        wy[i] = (s16)((i / 6) * 128 + 40);
        wvx[i] = vtab[i & 3];
        wvy[i] = vtab[(i >> 2) & 3];
        wtile[i] = 4;
        wattr[i] = OBJ_PRIO(3) | OBJ_PAL(0);
    }
    /* The walkers: a cluster near the centre, each pacing up or down, so
     * that they overlap and pass one another. */
    for (i = 0; i < NWALK; i++) {
        wx[WALK_ID + i] = (s16)(WORLD_W / 2 - 64 + i * 20);
        wy[WALK_ID + i] = (s16)(WORLD_H / 2 - 40 + (i & 1) * 36);
        wvx[WALK_ID + i] = 0;
        wvy[WALK_ID + i] = (i & 1) ? -1 : 1;
        wtile[WALK_ID + i] = 0;
        wattr[WALK_ID + i] = (u8)(OBJ_PRIO(3) | OBJ_PAL(1 + i));
        oamSetSize((u16)(WALK_ID + i), 1);   /* the size bit, once: the batch call never touches it */
        walk_order[i] = (u8)i;
    }

    coins.x = wx;  coins.y = wy;  coins.tile = wtile;  coins.attr = wattr;
    coins.visible = wseen;  coins.first_id = 0;  coins.count = NCOINS;  coins.size = 16;
    coins.order = 0;
    walkers.x = wx + WALK_ID;  walkers.y = wy + WALK_ID;
    walkers.tile = wtile + WALK_ID;  walkers.attr = wattr + WALK_ID;
    walkers.visible = wseen + WALK_ID;  walkers.first_id = WALK_ID;  walkers.count = NWALK;
    walkers.size = 32;
    walkers.order = walk_order;          /* slot WALK_ID + i shows walker walk_order[i] */

    /* The camera starts centred on the walkers. */
    cam_x = WORLD_W / 2 - 128;
    cam_y = WORLD_H / 2 - 112;

    setMainScreen(LAYER_OBJ);
    setScreenOn();

    while (1) {
        /* The camera, bounded by the world. */
        held = padHeld(0);
        if ((held & KEY_LEFT)  && cam_x > 0)        cam_x -= 2;
        if ((held & KEY_RIGHT) && cam_x < CAM_XMAX) cam_x += 2;
        if ((held & KEY_UP)    && cam_y > 0)        cam_y -= 2;
        if ((held & KEY_DOWN)  && cam_y < CAM_YMAX) cam_y += 2;

        /* The coins drift and bounce on the world's edges. */
        for (i = 0; i < NCOINS; i++) {
            s16 nx = (s16)(wx[i] + wvx[i]);
            s16 ny = (s16)(wy[i] + wvy[i]);
            if (nx < 0 || nx > WORLD_W - 16) { wvx[i] = (s8)(-wvx[i]); nx = (s16)(wx[i] + wvx[i]); }
            if (ny < 0 || ny > WORLD_H - 16) { wvy[i] = (s8)(-wvy[i]); ny = (s16)(wy[i] + wvy[i]); }
            wx[i] = nx;
            wy[i] = ny;
        }
        /* The walkers pace 40 pixels up and down around the centre. */
        for (i = WALK_ID; i < WALK_ID + NWALK; i++) {
            s16 ny = (s16)(wy[i] + wvy[i]);
            if (ny < WORLD_H / 2 - 60 || ny > WORLD_H / 2 + 20) { wvy[i] = (s8)(-wvy[i]); ny = (s16)(wy[i] + wvy[i]); }
            wy[i] = ny;
        }

        /* Depth: the walker lowest on the screen goes to the first slot. */
        sort_walkers();
        world_front = walk_order[0];

        /* Camera, culling, OAM: one call per batch. */
        oamPlaceWorld(&coins, cam_x, cam_y);
        oamPlaceWorld(&walkers, cam_x, cam_y);

        /* What the call reported: the coins in view this frame. */
        world_visible = 0;
        for (i = 0; i < NCOINS; i++)
            world_visible += wseen[i];

        WaitForVBlank();
    }
    return 0;
}
