/**
 * @file main.c
 * @brief World-space sprites — a camera, culling, a depth sort and OAM in one call per batch
 * @ingroup examples
 *
 * A square of the Rue basse, seen from above: data shards drift over the
 * paving and six residents pace up and down round the neon ring at its
 * centre. A scrolling game keeps its actors in WORLD coordinates (the
 * square is 512 x 448, BG1 a 64x64 map scrolled with the camera) and shows
 * a 256 x 224 window of it. Every frame it has to
 * subtract the camera from each actor, decide whether the result is on
 * screen, write the OAM entry or hide it, and set the ninth x bit for a
 * sprite half out on the left. oamPlaceWorld() does it
 * in assembly for a whole batch, about 1,700 a sprite on the library bench
 * (`docs/PERF.md`, row `world`).
 *
 * The game keeps one array per property (structure of arrays: what the
 * 65816 indexes cheaply) and fills an OamWorldBatch with their addresses
 * once. Two batches, because a batch has one size: 17 shards of 16 x 16
 * (the SMALL size of OBJ_SIZE16_L32) and six residents of 32 x 32 (the LARGE
 * size, set once with oamSetSize(): the call never touches the size bit).
 *
 * The residents show `order`. Seen from above, the one lower on the screen
 * is nearer and must be drawn in front — and OAM order is drawing order,
 * the lower slot in front. So the residents are sorted by y every frame
 * (an insertion sort of six bytes) and the batch's `order` says which
 * resident each slot shows: the arrays stay indexed by the resident, only
 * the six bytes of `order` move. Each one faces the way it walks: its
 * frame (south going down, north going up) is its entry of `tile`.
 *
 * Hold the D-pad to move the camera. `visible[]` comes back from the call:
 * the example counts the shards in view with it (`world_visible`); a game
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
 * Six residents (Kit, a frog, an owl, a pangolin, a raccoon, a rat) pace
 * up and down round the ring and pass in front of and behind one another
 * as their y changes, turning their back when they walk up; the shards
 * drift over the square; the D-pad scrolls the camera (and the paving with
 * it) and stops at the kerb. `world_visible` (probe oracle) follows how
 * many shards are in view, `world_front` which resident is in the front
 * slot, `cam_x` / `cam_y` the camera.
 *
 * @par Modules Used
 * console, dma, background, sprite, input
 *
 * @see lib/include/snes/sprite.h — OamWorldBatch, oamPlaceWorld, oamSetSize
 * @see examples/sprites/sprite_swarm — the same sprites moved by direct OAM writes, no camera
 */

#include <snes.h>

#include "res/plaza.inc"
#include "res/residents.inc"
/* generated: plaza_tiles/_pal/_map, residents_tiles/_pal (each with _end) */

/** @brief Shards in the world (the 16 x 16 batch). */
#define NSHARDS 17
/** @brief Residents (the 32 x 32 batch, sorted by depth). */
#define NRES 6
/** @brief The residents' OAM ids follow the shards'. */
#define RES_ID NSHARDS
/** @brief BG1's 64x64 map (4096 words, four 32x32 pages) and its tiles. */
#define MAP_VRAM 0x0000
#define BG_VRAM  0x1000
/** @brief Sprite tile VRAM word address. */
#define SPR_VRAM 0x4000
/** @brief The shard's tile: the first 16 x 16 block of the sheet's fourth row. */
#define SHARD_TILE 192
/** @brief World size in pixels. */
#define WORLD_W 512
#define WORLD_H 448
/** @brief The camera may not show past the world's edge. */
#define CAM_XMAX (WORLD_W - 256)
#define CAM_YMAX (WORLD_H - 224)

/** @brief Probe oracles: the camera, the shards placed this frame, the resident in the front slot. */
u16 cam_x;
u16 cam_y;
u16 world_visible;
u16 world_front;

/* Per-sprite data, one array per property: what OamWorldBatch points at.
 * The shards' arrays are the first NSHARDS entries, the residents' the rest. */
static s16 wx[NSHARDS + NRES];     /**< world x of the top-left corner */
static s16 wy[NSHARDS + NRES];     /**< world y */
static u8  wtile[NSHARDS + NRES];  /**< tile name, low byte */
static u8  wattr[NSHARDS + NRES];  /**< vhoopppc, as OAM_ATTR() builds it */
static u8  wseen[NSHARDS + NRES];  /**< out: 1 placed, 0 hidden */
static s8  wvx[NSHARDS + NRES];    /**< drift */
static s8  wvy[NSHARDS + NRES];

/** @brief The residents' drawing order: slot i of the batch shows resident order[i]. */
static u8 res_order[NRES];

/** @brief The two batches: the shards (16 x 16) and the residents (32 x 32, sorted). */
static OamWorldBatch shards;
static OamWorldBatch crowd;   /**< the residents */

/**
 * @brief The OAM tile of frame @p f of res/residents.png
 *
 * A 32x32 frame is 4x4 tiles and a row of OBJ tiles is 16 wide, so frame f
 * (four a row of the sheet) starts at column (f % 4) * 4 of tile row
 * (f / 4) * 4. Frame 2k is resident k facing south, 2k + 1 facing north.
 */
static u8 frame_tile(u16 f) {
    return (u8)(((f & 3) << 2) + ((f >> 2) << 6));
}

/** @brief Sort the residents by y, the lowest on screen first: an insertion sort of six bytes. */
static void sort_residents(void) {
    u8 i, j, id;
    for (i = 1; i < NRES; i++) {
        id = res_order[i];
        j = i;
        while (j > 0 && wy[RES_ID + res_order[j - 1]] < wy[RES_ID + id]) {
            res_order[j] = res_order[j - 1];
            j--;
        }
        res_order[j] = id;
    }
}

int main(void) {
    static const s8 vtab[4] = {-1, 1, -2, 2};
    u16 i;
    u16 held;

    consoleInit();
    setMode(BG_MODE1, 0);

    /* BG1: the square, 275 tiles at $1000 and a 64x64 map (8 KB) at $0000,
     * loaded in force blank before setScreenOn() */
    bgSetMapPtr(0, MAP_VRAM, SC_64x64);
    bgInitTileSet(0, plaza_tiles, plaza_pal, 0,
                  plaza_tiles_end - plaza_tiles,
                  plaza_pal_end - plaza_pal,
                  BG_16COLORS, BG_VRAM);
    dmaCopyVram(plaza_map, MAP_VRAM, plaza_map_end - plaza_map);

    /* The sprites: twelve frames of the residents and the shard, 256 tiles
     * at $4000; OBJ palette 0 the shard's, 1-6 one a resident (CGRAM 128+). */
    oamInitGfxSet(residents_tiles, residents_tiles_end - residents_tiles,
                  residents_pal, residents_pal_end - residents_pal,
                  0, SPR_VRAM, OBJ_SIZE16_L32);
    oamClear();

    /* The shards: spread over the square, each with a drift. */
    for (i = 0; i < NSHARDS; i++) {
        wx[i] = (s16)((i % 6) * 80 + 24);
        wy[i] = (s16)((i / 6) * 128 + 40);
        wvx[i] = vtab[i & 3];
        wvy[i] = vtab[(i >> 2) & 3];
        wtile[i] = SHARD_TILE;
        wattr[i] = OBJ_PRIO(3) | OBJ_PAL(0);
    }
    /* The residents: a cluster round the ring, each pacing up or down, so
     * that they overlap and pass one another. */
    for (i = 0; i < NRES; i++) {
        wx[RES_ID + i] = (s16)(WORLD_W / 2 - 64 + i * 20);
        wy[RES_ID + i] = (s16)(WORLD_H / 2 - 40 + (i & 1) * 36);
        wvx[RES_ID + i] = 0;
        wvy[RES_ID + i] = (i & 1) ? -1 : 1;
        wtile[RES_ID + i] = frame_tile(2 * i + ((i & 1) ? 1 : 0));
        wattr[RES_ID + i] = (u8)(OBJ_PRIO(3) | OBJ_PAL(1 + i));
        oamSetSize((u16)(RES_ID + i), 1);   /* the size bit, once: the batch call never touches it */
        res_order[i] = (u8)i;
    }

    shards.x = wx;  shards.y = wy;  shards.tile = wtile;  shards.attr = wattr;
    shards.visible = wseen;  shards.first_id = 0;  shards.count = NSHARDS;  shards.size = 16;
    shards.order = 0;
    crowd.x = wx + RES_ID;  crowd.y = wy + RES_ID;
    crowd.tile = wtile + RES_ID;  crowd.attr = wattr + RES_ID;
    crowd.visible = wseen + RES_ID;  crowd.first_id = RES_ID;  crowd.count = NRES;
    crowd.size = 32;
    crowd.order = res_order;          /* slot RES_ID + i shows resident res_order[i] */

    /* The camera starts centred on the ring. */
    cam_x = WORLD_W / 2 - 128;
    cam_y = WORLD_H / 2 - 112;

    setMainScreen(LAYER_BG1 | LAYER_OBJ);
    bgSetScroll(0, cam_x, cam_y);
    setScreenOn();

    while (1) {
        /* The camera, bounded by the world. */
        held = padHeld(0);
        if ((held & KEY_LEFT)  && cam_x > 0)        cam_x -= 2;
        if ((held & KEY_RIGHT) && cam_x < CAM_XMAX) cam_x += 2;
        if ((held & KEY_UP)    && cam_y > 0)        cam_y -= 2;
        if ((held & KEY_DOWN)  && cam_y < CAM_YMAX) cam_y += 2;

        /* The shards drift and bounce on the world's edges. */
        for (i = 0; i < NSHARDS; i++) {
            s16 nx = (s16)(wx[i] + wvx[i]);
            s16 ny = (s16)(wy[i] + wvy[i]);
            if (nx < 0 || nx > WORLD_W - 16) { wvx[i] = (s8)(-wvx[i]); nx = (s16)(wx[i] + wvx[i]); }
            if (ny < 0 || ny > WORLD_H - 16) { wvy[i] = (s8)(-wvy[i]); ny = (s16)(wy[i] + wvy[i]); }
            wx[i] = nx;
            wy[i] = ny;
        }
        /* The residents pace 40 pixels up and down around the centre,
         * facing the way they walk: south going down, north going up. */
        for (i = RES_ID; i < RES_ID + NRES; i++) {
            s16 ny = (s16)(wy[i] + wvy[i]);
            if (ny < WORLD_H / 2 - 60 || ny > WORLD_H / 2 + 20) { wvy[i] = (s8)(-wvy[i]); ny = (s16)(wy[i] + wvy[i]); }
            wy[i] = ny;
            wtile[i] = frame_tile(2 * (i - RES_ID) + (wvy[i] < 0 ? 1 : 0));
        }

        /* Depth: the resident lowest on the screen goes to the first slot. */
        sort_residents();
        world_front = res_order[0];

        /* Camera, culling, OAM: one call per batch. */
        oamPlaceWorld(&shards, cam_x, cam_y);
        oamPlaceWorld(&crowd, cam_x, cam_y);

        /* What the call reported: the shards in view this frame. */
        world_visible = 0;
        for (i = 0; i < NSHARDS; i++)
            world_visible += wseen[i];

        /* The paving follows the camera (bgSetScroll defers to the NMI) */
        bgSetScroll(0, cam_x, cam_y);

        WaitForVBlank();
    }
    return 0;
}
