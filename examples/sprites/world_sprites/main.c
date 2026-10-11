/**
 * @file main.c
 * @brief World-space sprites — a camera, culling and OAM in one call per batch
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
 * once. Two batches, because a batch has one size: the 23 coins are 8 x 8
 * sprites, the player is 16 x 16 (the LARGE size of OBJ_SIZE8_L16, set once
 * with oamSetSize(): the call never touches the size bit).
 *
 * Hold the D-pad to move the camera; the player sits at the centre of the
 * view and the coins drift. `visible[]` comes back from the call: the
 * example counts the coins on screen with it (`world_visible`), the way a
 * game would skip animation or streaming for what is not shown.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - World coordinates minus a camera = screen coordinates; a sprite partly
 *   off the left edge needs the ninth x bit (x in 256..511), which the PPU
 *   reads as negative
 * - OAM culling: a sprite off screen is hidden (x = 257, y = 240), not left
 *   wherever it was
 * - Structure of arrays for per-sprite data, and the size bit set once
 *
 * @par What to Observe
 * Coins drift over a dark field; the player's square stays centred while
 * the D-pad scrolls the camera. A coin leaving the left edge is cut by the
 * edge, not wrapped to the right; `world_visible` (probe oracle) follows
 * how many coins are in view, `cam_x` / `cam_y` the camera.
 *
 * @par Modules Used
 * console, dma, background, sprite, input, tile
 *
 * @see lib/include/snes/sprite.h — OamWorldBatch, oamPlaceWorld, oamSetSize
 * @see examples/sprites/sprite_swarm — the same sprites moved by direct OAM writes, no camera
 */

#include <snes.h>
#include <snes/tile.h>   /* tileEncode4bpp */

/** @brief Coins in the world (the 8 x 8 batch). */
#define NCOINS 23
/** @brief The player's OAM id: the one 16 x 16 sprite, after the coins. */
#define PLAYER_ID NCOINS
/** @brief Sprite tile VRAM word address. */
#define SPR_VRAM 0x4000
/** @brief World size in pixels. */
#define WORLD_W 512
#define WORLD_H 448
/** @brief The camera may not show past the world's edge. */
#define CAM_XMAX (WORLD_W - 256)
#define CAM_YMAX (WORLD_H - 224)

/** @brief Probe oracles: the camera, and the coins oamPlaceWorld() placed this frame. */
u16 cam_x;
u16 cam_y;
u16 world_visible;

/* Per-sprite data, one array per property: what OamWorldBatch points at.
 * The coins' arrays are the first NCOINS entries, the player's the last. */
static s16 wx[NCOINS + 1];     /**< world x of the top-left corner */
static s16 wy[NCOINS + 1];     /**< world y */
static u8  wtile[NCOINS + 1];  /**< tile name, low byte */
static u8  wattr[NCOINS + 1];  /**< vhoopppc, as OAM_ATTR() builds it */
static u8  wseen[NCOINS + 1];  /**< out: 1 placed, 0 hidden */
static s8  wvx[NCOINS];        /**< the coins' drift */
static s8  wvy[NCOINS];

/** @brief The two batches: the coins (8 x 8) and the player (16 x 16). */
static OamWorldBatch coins;
static OamWorldBatch player;

/** @brief Scratch: an 8 x 8 tile as pixel indices, and its 4bpp form. */
static u8 px[64];
static u8 tilebuf[32];

/** @brief Encode one 8 x 8 shape (8 rows of bits, colour index 1) to VRAM name `name`,
 *  mirrored left-right (`flipx`) and / or top-bottom (`flipy`): a 16 x 16 sprite has ONE
 *  attribute byte for its four tiles, so each corner of the frame is its own tile. */
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

int main(void) {
    static const u8 coin_rows[8]   = {0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C};
    static const u8 corner_rows[8] = {0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0};
    static const s8 vtab[4] = {-1, 1, -2, 2};
    u16 pal[64];
    u16 i;
    u16 held;

    consoleInit();
    setMode(BG_MODE1, 0);

    /* Sprite palettes live at CGRAM 128+: palette 0 gold for the coins,
     * palette 1 white for the player. Only colour 1 of each is used. */
    for (i = 0; i < 64; i++) pal[i] = 0;
    pal[0 * 16 + 1] = RGB(31, 26, 6);
    pal[1 * 16 + 1] = RGB(31, 31, 31);
    dmaCopyCGram((u8 *)pal, OBJ_CGRAM_BASE, 128);

    /* Names 0, 1, 16, 17 are the four corners of the 16 x 16 player (a
     * 16 x 16 sprite reads its tiles two to a row, 16 names apart); name 2
     * is the coin. */
    load_shape(corner_rows, 0, 0, 0);     /* top-left corner of the frame */
    load_shape(corner_rows, 1, 1, 0);     /* top-right */
    load_shape(corner_rows, 16, 0, 1);    /* bottom-left */
    load_shape(corner_rows, 17, 1, 1);    /* bottom-right */
    load_shape(coin_rows, 2, 0, 0);

    oamInit(OBJ_SIZE8_L16, SPR_VRAM >> 13);
    oamClear();

    /* The coins: spread over the world, each with a drift. */
    for (i = 0; i < NCOINS; i++) {
        wx[i] = (s16)((i % 6) * 80 + 24);
        wy[i] = (s16)((i / 6) * 96 + 40);
        wvx[i] = vtab[i & 3];
        wvy[i] = vtab[(i >> 2) & 3];
        wtile[i] = 2;
        wattr[i] = OBJ_PRIO(3) | OBJ_PAL(0);
    }
    /* The player: at the centre of the world, 16 x 16, a frame of four
     * corner tiles (names 0, 1, 16, 17). */
    wx[PLAYER_ID] = WORLD_W / 2 - 8;
    wy[PLAYER_ID] = WORLD_H / 2 - 8;
    wtile[PLAYER_ID] = 0;
    wattr[PLAYER_ID] = OBJ_PRIO(3) | OBJ_PAL(1);
    oamSetSize(PLAYER_ID, 1);          /* the size bit, once: the batch call never touches it */

    coins.x = wx;  coins.y = wy;  coins.tile = wtile;  coins.attr = wattr;
    coins.visible = wseen;  coins.first_id = 0;  coins.count = NCOINS;  coins.size = 8;
    coins.order = 0;
    player.x = wx + PLAYER_ID;  player.y = wy + PLAYER_ID;
    player.tile = wtile + PLAYER_ID;  player.attr = wattr + PLAYER_ID;
    player.visible = 0;  player.first_id = PLAYER_ID;  player.count = 1;  player.size = 16;
    player.order = 0;

    /* The camera starts centred on the player. */
    cam_x = WORLD_W / 2 - 128;
    cam_y = WORLD_H / 2 - 112;

    setMainScreen(LAYER_OBJ);
    setScreenOn();

    while (1) {
        /* The camera, bounded by the world; the player stays with it. */
        held = padHeld(0);
        if ((held & KEY_LEFT)  && cam_x > 0)        cam_x -= 2;
        if ((held & KEY_RIGHT) && cam_x < CAM_XMAX) cam_x += 2;
        if ((held & KEY_UP)    && cam_y > 0)        cam_y -= 2;
        if ((held & KEY_DOWN)  && cam_y < CAM_YMAX) cam_y += 2;
        wx[PLAYER_ID] = (s16)(cam_x + 128 - 8);
        wy[PLAYER_ID] = (s16)(cam_y + 112 - 8);

        /* The coins drift and bounce on the world's edges. */
        for (i = 0; i < NCOINS; i++) {
            s16 nx = (s16)(wx[i] + wvx[i]);
            s16 ny = (s16)(wy[i] + wvy[i]);
            if (nx < 0 || nx > WORLD_W - 8)  { wvx[i] = (s8)(-wvx[i]); nx = (s16)(wx[i] + wvx[i]); }
            if (ny < 0 || ny > WORLD_H - 8)  { wvy[i] = (s8)(-wvy[i]); ny = (s16)(wy[i] + wvy[i]); }
            wx[i] = nx;
            wy[i] = ny;
        }

        /* Camera, culling, OAM: one call per batch. */
        oamPlaceWorld(&coins, cam_x, cam_y);
        oamPlaceWorld(&player, cam_x, cam_y);

        /* What the call reported: the coins in view this frame. */
        world_visible = 0;
        for (i = 0; i < NCOINS; i++)
            world_visible += wseen[i];

        WaitForVBlank();
    }
    return 0;
}
