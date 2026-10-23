/**
 * @file main.c
 * @brief RPG template — a Tiled-driven street of the city, its residents, a dialog box
 * @ingroup examples
 *
 * The SDK's modules composed into a playable RPG skeleton: Kit the courier
 * walks the Rue basse at night, between a noodle bar, a garage, a junk shop
 * and a canal, talks to the residents, opens a locker, and steps into the
 * raccoon's den. Three things make it a template rather than a demo:
 *
 * 1. **The map is a real Tiled map** (`res/town.tmj`): terrain, per-tile
 *    collision (the `attribute` property), entity positions AND each
 *    resident's line and look (`text` and `look` properties on the object)
 *    all live in the map, not in the code. The build converts it with
 *    `opensnes-level` (`res/town.tmj.toml`): the VRAM map, the collision
 *    bytes and the entities as C defines. Adding a resident is a map edit.
 * 2. **A real bordered dialog box and a HUD**, both 9-slice panels on BG2
 *    with their text on BG3 above — the classic SNES RPG window.
 * 3. **Two scenes**: the street, and the inside of the apartment with the
 *    teal door. A scene is a tileset + a palette + a tilemap + a collision
 *    map + entities; switching one is three DMAs under force blank. The den
 *    is its own Tiled map (`res/house.tmj`).
 *
 * Layer roles (Mode 1):
 * - BG1: the scene (4bpp, 64x64 scrolling)
 * - BG2: the HUD (always) and the dialog panel (while talking)
 * - BG3: the dialog text (2bpp, high priority)
 * - OBJ: Kit and the residents, 32x32, one palette each
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - Tiled (.tmj) as the content pipeline: collision and entities are
 *   data, not hardcoded, and the build converts them (opensnes-level)
 * - Metatile movement: the map is 8x8 cells, the world is walked on a
 *   grid of 16x16 metatiles (two cells a side). A character occupies one
 *   metatile and its 32x32 sprite is drawn over it, feet on it, head
 *   overhanging upward: what you see is what collides
 * - A 32x32 sprite is a 4x4 block of the OBJ name table, which is 16 tiles
 *   wide: a frame converted alone is copied in four rows of 128 bytes
 * - The sprite-palette plan: res/palettes.toml gives every character its
 *   OBJ slot, and palettes.inc names it (PAL_OWL_SLOT, PAL_OWL_CGRAM)
 * - A 9-slice dialog box DMA'd to BG2 on open, with layer priorities
 *   stacking town < box < text
 * - Forced blank (`setScreenOff`, INIDISP bit 7) around a multi-KB VRAM
 *   upload — `setBrightness(0)` only blacks the screen, it does NOT open
 *   the VRAM write window, and the tail of the transfer is dropped
 * - A fixed 16-colour palette per scene, imposed on the converter rather
 *   than quantised: adding one tile to a quantised sheet re-derives the
 *   whole palette and every existing tile shifts hue
 * - Off-camera entities MUST be hidden, not just drawn: OAM coordinates
 *   wrap, so an entity two screens away reappears somewhere plausible on
 *   screen (a resident standing inside a wall)
 * - Collision through the SDK's `collideTile()` over the converted map —
 *   its `const` tilemap parameter means the read is bank-honouring, so a
 *   4 KB map needs neither bank $00 nor RAM
 *
 * @par What to Observe
 * The street fades in, with hearts and credits in the HUD. Walk with the
 * D-pad, a metatile a step: buildings, the canal and the railing block you
 * where they look. Face a resident and press A (the owl at the noodle bar,
 * the frog at the garage, the rat at the junk shop, the pangolin over the
 * bridge: each says its line, from the map), or face the lockers south of
 * the street and press A: the credits go up by 10. Walk into the teal door
 * of the apartment, south of the street, and you step inside — the raccoon
 * greets you with no button press. The mat by the door takes you back out.
 *
 * @par Modules Used
 * console, dma, background, sprite, text, input, collision, panel
 *
 * @see res/town.tmj, res/house.tmj — the Tiled content pipeline;
 *      devtools/art_assets/rpg.py, which composes them
 */

#include <snes.h>
#include <snes/background.h>
#include <snes/sprite.h>
#include <snes/text.h>
#include <snes/input.h>
#include <snes/collision.h>
#include <snes/panel.h>

#include "res/town_entities.inc"   /* SPAWN, NPC_TABLE, LOCKER, DOOR: the town's Entities layer */
#include "res/house_entities.inc"  /* ARRIVAL, HOST, MAT: the den's */
#include "res/palettes.inc"        /* PAL_HERO_* / PAL_OWL_* …: the sprite-slot plan (opensnes-palette) */

#include "res/town.inc"       /* generated: town_street_quad[] (the VRAM map), town_street_cells[] (collision) */
#include "res/house.inc"      /* generated: house_den_quad[], house_den_cells[] */
#include "res/tileset.inc"    /* generated: tileset_tiles[], tileset_pal[] (the town), each with _end */
#include "res/interior.inc"   /* generated: interior_tiles[], interior_pal[] (the den) */
#include "res/uibox.inc"      /* generated: uibox_tiles[], uibox_pal[] */
#include "res/hero.inc"       /* generated: hero_tiles[], hero_pal[]: Kit's walk */
#include "res/owl.inc"
#include "res/frog.inc"
#include "res/rat.inc"
#include "res/pangolin.inc"
#include "res/raccoon.inc"

/** @brief A resident of the street: where it stands, what it says, how it looks.
 * The struct shape and the rows both come from the Entities layer of
 * town.tmj, so adding one is a map edit. */
typedef struct { NPC_FIELDS } Npc;

/** @brief The residents. `const`, so the table lives in ROM and is read
 * with bank-honouring far addressing. */
static const Npc npcs[NPC_COUNT] = NPC_TABLE;

/* VRAM word layout */
#define VRAM_SCENE_TILES 0x0000   /* the scene's tiles, up to 1024 (the town has 822) */
#define VRAM_SCENE_MAP   0x4000   /* 64x64: four 32x32 pages */
#define VRAM_FONT        0x5000   /* BG3's and BG2's character base: tile 0 blank, the font from 2bpp tile 2 */
#define FONT_TILE        2        /* the font's first 2bpp tile: words $5000-$500F are BG2's blank tile 0 */
#define VRAM_UI_TILES    0x5400   /* BG2's tiles: tile 64 from the base at $5000 */
#define VRAM_TEXT_MAP    0x5800
#define VRAM_UI_MAP      0x5C00
#define VRAM_OBJ         0x6000   /* Kit's 15 frames (tiles 0-255), the residents from tile 256 ($7000) */

#define UI_BASE_TILE 64           /* ($5400 - $5000) / 16 words a 4bpp tile */

/* uibox.png is a 4x3 sheet: the 9-slice border in the first three
 * columns, the HUD icons in the fourth. Raster order, so row n starts
 * at 4n. */
#define BOX_TL 0
#define BOX_T  1
#define BOX_TR 2
#define ICON_HEART 3
#define BOX_L  4
#define BOX_C  5
#define BOX_R  6
#define ICON_HEART_EMPTY 7
#define BOX_BL 8
#define BOX_B  9
#define BOX_BR 10
#define ICON_CHIP 11
#define BOX_PAL 2                 /* BG2 palette 2 -> CGRAM 32-47 */

/* HUD panel (BG2, always on screen) */
#define HUD_X 0
#define HUD_Y 0
#define HUD_W 16
#define HUD_H 3
#define HERO_MAX_HP 3

/* Dialog panel geometry (BG2 tilemap, 32x32) */
#define PANEL_X 2
#define PANEL_Y 22
#define PANEL_W 28
#define PANEL_H 6

#define FACE_DOWN 0
#define FACE_UP   1
#define FACE_LEFT 2
#define FACE_RIGHT 3

#define MAP_CELLS 64              /* the map, in 8x8 cells */
#define MT 2                      /* a metatile, in cells */
#define WORLD_W (MAP_CELLS * 8)
#define WORLD_H (MAP_CELLS * 8)
#define SCREEN_CX 120             /* where the hero's metatile sits on screen */
#define SCREEN_CY 104

/* The OBJ tiles: Kit's frame f (down 0-4, up 5-9, right 10-14) four to a
 * row of the name table; resident k from tile 256 on, four to a row too. */
#define HERO_FRAME(f) ((u16)(((f) & 3) * 4 + ((f) >> 2) * 64))
#define RESIDENT_TILE(k) ((u16)(256 + ((k) & 3) * 4 + ((k) >> 2) * 64))
#define RESIDENTS 5

enum { ST_FADEIN, ST_EXPLORE, ST_DIALOG };
enum { SCENE_TOWN, SCENE_HOUSE };

/** @brief A resident's look: its 32x32 frame (converted alone) and its OBJ slot. */
typedef struct {
    const u8 *tiles;
    const u8 *pal;
    u16 cgram;
    u8 slot;
} Look;

/** @brief The `look` property of an entity indexes this table: owl, frog, rat, pangolin, raccoon. */
static const Look looks[RESIDENTS] = {
    { owl_tiles,      owl_pal,      PAL_OWL_CGRAM,      PAL_OWL_SLOT },
    { frog_tiles,     frog_pal,     PAL_FROG_CGRAM,     PAL_FROG_SLOT },
    { rat_tiles,      rat_pal,      PAL_RAT_CGRAM,      PAL_RAT_SLOT },
    { pangolin_tiles, pangolin_pal, PAL_PANGOLIN_CGRAM, PAL_PANGOLIN_SLOT },
    { raccoon_tiles,  raccoon_pal,  PAL_RACCOON_CGRAM,  PAL_RACCOON_SLOT },
};

/** @brief Probe oracles / state. hero_x/hero_y are the PIXEL position
 * of the metatile the hero occupies (a multiple of 16 when idle). */
u16 hero_x;
u16 hero_y;
u8  hero_facing;
u8  game_state;
u8  chest_opened;          /**< the locker */
u8  scene;                 /**< SCENE_TOWN or SCENE_HOUSE */
u16 gold;                  /**< HUD: credits, +10 from the locker */
u8  hero_hp;               /**< HUD: static in this template */

static s8 step_dx, step_dy;
static u8 step_count;
static u8 walk_frame, anim_tick;
static u8 fade_level;
static u16 panel_map[32 * 32];

/* ---- collision: the SDK's collideTile() over the converted map ----
 * collideTile takes PIXEL coordinates, converts to 8x8 cells and returns
 * the cell's byte (nonzero = solid). It bounds X and negative coordinates
 * itself (off-map = solid) but cannot bound Y — it does not know the map
 * height — so we clamp that here, as its docs ask. Its `tilemap`
 * parameter is const, so the read is bank-honouring: the 4 KB maps live
 * outside bank $00. */
static u8 cell_walkable(u16 cx, u16 cy) {
    if (cy >= MAP_CELLS) {
        return 0;
    }
    return (u8)(collideTile((s16)(cx << 3), (s16)(cy << 3),
                            scene == SCENE_HOUSE ? house_den_cells : town_street_cells,
                            MAP_CELLS) == 0);
}

/** @brief A metatile (its top-left cell) is walkable if its four cells
 * are, and no one stands on it. */
static u8 metatile_walkable(u16 cx, u16 cy) {
    u8 i;
    if (!cell_walkable(cx, cy) || !cell_walkable(cx + 1, cy)
        || !cell_walkable(cx, cy + 1) || !cell_walkable(cx + 1, cy + 1)) {
        return 0;
    }
    if (scene == SCENE_HOUSE) {
        return (u8)!(cx == HOST_TX && cy == HOST_TY);
    }
    for (i = 0; i < NPC_COUNT; i++) {
        if (cx == npcs[i].tx && cy == npcs[i].ty) {
            return 0;
        }
    }
    return 1;
}

static u16 hero_tx(void) { return (u16)(hero_x >> 3); }
static u16 hero_ty(void) { return (u16)(hero_y >> 3); }

/** @brief The metatile in front of the hero (its top-left cell). */
static void front_tile(u16 *tx, u16 *ty) {
    u16 cx = hero_tx();
    u16 cy = hero_ty();
    switch (hero_facing) {
    case FACE_UP:    cy -= MT; break;
    case FACE_DOWN:  cy += MT; break;
    case FACE_LEFT:  cx -= MT; break;
    case FACE_RIGHT: cx += MT; break;
    default: break;
    }
    *tx = cx;
    *ty = cy;
}

/* ---- BG2: the HUD and the dialog box, both on one tilemap ----
 * Two 9-slice panels in the same map: the HUD at the top, permanent,
 * and the dialog box at the bottom, stamped on open and blanked on
 * close. One upload covers both, so opening a dialog never disturbs
 * the HUD. The `panel` module owns the stamping and does the upload
 * under forced blank; the buffer stays ours so its 2 KB is visible. */
static const Panel ui = {
    panel_map,          /* map        */
    VRAM_UI_MAP,        /* vram_addr  */
    UI_BASE_TILE,       /* base_tile — the sheet sits after BG3's font */
    4,                  /* stride — 4 wide: col 4 holds the HUD icons */
    BOX_PAL,            /* palette    */
    1,                  /* priority — in front of the opaque town */
};

/**
 * @brief Draw the HUD icons: hearts for HP, a chip for the credits.
 *
 * The icons are tiles on BG2 (they came out of the same 4x3 sheet as
 * the dialog border), the number is text on BG3. Splitting it that way
 * costs nothing: both layers are already up for the dialog box.
 */
static void hud_icons(void) {
    u16 i;
    /* panelPut takes an absolute tile: the sheet's index plus its base */
    for (i = 0; i < HERO_MAX_HP; i++) {
        panelPut(&ui, (u8)(1 + i), 1,
                 UI_BASE_TILE + ((i < hero_hp) ? ICON_HEART : ICON_HEART_EMPTY));
    }
    panelPut(&ui, 8, 1, UI_BASE_TILE + ICON_CHIP);
}

/** @brief The HUD's numeric half, on BG3. Re-drawn after every
 * textClearRect, since clearing the dialog does not touch these rows. */
static void hud_text(void) {
    textPrintAt(10, 1, "     ");   /* erase the previous amount */
    textSetPos(10, 1);
    textPrintU16(gold);
}

/** @brief Stamp the HUD's frame and icons into the BG2 buffer. */
static void build_panel(void) {
    panelInit(&ui);
    panelDraw(&ui, HUD_X, HUD_Y, HUD_W, HUD_H);
    hud_icons();
}

/** @brief Open the dialog box on a line of text. */
static void dialog_open(const char *line) {
    panelDraw(&ui, PANEL_X, PANEL_Y, PANEL_W, PANEL_H);
    panelFlush(&ui);
    textPrintAt(PANEL_X + 2, PANEL_Y + 2, line);
    textPrintAt(PANEL_X + 2, PANEL_Y + 4, "         (A) OK");
    game_state = ST_DIALOG;
}

/** @brief Close the dialog box, the HUD untouched. */
static void dialog_close(void) {
    /* clear only the dialog rows: textClear() would wipe the HUD too */
    textClearRect(PANEL_X, PANEL_Y, PANEL_W, PANEL_H);
    panelClear(&ui, PANEL_X, PANEL_Y, PANEL_W, PANEL_H);
    panelFlush(&ui);
    game_state = ST_EXPLORE;
}

/* ---- scenes: the street <-> the den ----
 * A scene is a tileset, a palette, a tilemap, a collision map and a set
 * of entities. Switching one is three DMAs under force blank — there is
 * no engine here, just the data swapped over; the collision map and the
 * entities are read in place, by `scene`. */
static void scene_load(u8 which, u16 tx, u16 ty, u8 facing) {
    setScreenOff();                 /* forced blank: 34 KB will not fit VBlank */
    if (which == SCENE_HOUSE) {
        dmaCopyVram(interior_tiles, VRAM_SCENE_TILES,
                    (u16)(interior_tiles_end - interior_tiles));
        dmaCopyVram(house_den_quad, VRAM_SCENE_MAP,
                    (u16)(house_den_quad_end - house_den_quad));
        dmaCopyCGram(interior_pal, 0, 32);
        bgSetScroll(0, 0, 0);
    } else {
        dmaCopyVram(tileset_tiles, VRAM_SCENE_TILES,
                    (u16)(tileset_tiles_end - tileset_tiles));
        dmaCopyVram(town_street_quad, VRAM_SCENE_MAP,
                    (u16)(town_street_quad_end - town_street_quad));
        dmaCopyCGram(tileset_pal, 0, 32);
    }
    scene = which;
    hero_x = (u16)(tx * 8);
    hero_y = (u16)(ty * 8);
    hero_facing = facing;
    step_count = 0;
    setScreenOn();
}

/* ---- movement: one metatile per step, slid over 8 frames ---- */
static void begin_step(s8 dx, s8 dy) {
    u16 ntx = (u16)(hero_tx() + dx * MT);
    u16 nty = (u16)(hero_ty() + dy * MT);
    if (metatile_walkable(ntx, nty)) {
        step_dx = dx;
        step_dy = dy;
        step_count = 8;
    }
}

/**
 * @brief Copy a resident's 32x32 frame into its block of the OBJ name table.
 *
 * The converter writes a sheet in 128-px rows of tiles, the way the name
 * table is laid out: a 32x32 frame converted on its own becomes four rows of
 * 16 tiles, the frame in the first four of each. The block of a 32x32 sprite
 * is four tiles wide and four rows deep, so the frame goes in four copies of
 * 128 bytes, one name-table row (256 words) apart.
 */
static void load_resident(const u8 *tiles, u16 first_tile) {
    u8 r;
    for (r = 0; r < 4; r++) {
        dmaCopyVram(tiles + r * 512, (u16)(VRAM_OBJ + (first_tile + r * 16) * 16), 128);
    }
}

/**
 * @brief Draw a 32x32 character over its metatile, or hide it.
 *
 * Two things happen here, both of them load-bearing.
 *
 * **The overhang** is what makes collision feel exact: the character's
 * logical position is one 16x16 metatile; the sprite is drawn 8 px left
 * and 16 px up of it, so its feet stand on that metatile and its head
 * overhangs upward — the standard top-down RPG convention. Drawing the
 * sprite at the metatile's corner instead (the naive version) puts the
 * visible body half a metatile away from what collides.
 *
 * **The cull** is not an optimisation, it is correctness. OAM X is 9
 * bits and Y is 8, so an entity standing outside the camera does not
 * quietly vanish — its coordinates WRAP and it reappears somewhere
 * plausible-looking on screen. Anything off-camera goes to oamHide(),
 * which parks it properly (Y AND the X high bit — Y alone still wraps
 * for sprites over 16 px tall).
 */
static void draw_char(u8 oam_id, u16 wx, u16 wy, u16 cam_x, u16 cam_y,
                      u16 tile, u8 palette, u8 flags) {
    s16 sx = (s16)((s16)wx - (s16)cam_x - 8);
    s16 sy = (s16)((s16)wy - (s16)cam_y - 16);

    if (sx < -32 || sx > 255 || sy < -32 || sy > 223) {
        oamHide(oam_id);
        return;
    }
    oamSet(oam_id, (u16)sx, (u16)sy, tile, palette, 2, flags);
    oamSetSize(oam_id, OBJ_LARGE);
}

/** @brief Kit's tile for the current facing and walk frame. */
static u16 hero_tile(void) {
    static const u8 first[4] = { 0, 5, 10, 10 };   /* down, up, left (= right mirrored), right */
    return HERO_FRAME(first[hero_facing] + walk_frame);
}

/** @brief Is the metatile (ftx, fty) one of the lockers' four? */
static u8 at_locker(u16 ftx, u16 fty) {
    return (u8)(ftx >= LOCKER_TX && ftx < LOCKER_TX + 2 * MT
                && fty >= LOCKER_TY && fty < LOCKER_TY + 2 * MT);
}

/** @brief A press of A while exploring: talk to who stands in front, or open the locker. */
static void interact(void) {
    u16 ftx, fty;
    u8 i;
    front_tile(&ftx, &fty);
    if (scene == SCENE_HOUSE) {
        if (ftx == HOST_TX && fty == HOST_TY) {
            dialog_open(HOST_TEXT);
        }
        return;
    }
    for (i = 0; i < NPC_COUNT; i++) {
        if (ftx == npcs[i].tx && fty == npcs[i].ty) {
            dialog_open(npcs[i].text);
            return;
        }
    }
    if (at_locker(ftx, fty)) {
        if (chest_opened) {
            dialog_open("THE LOCKER IS EMPTY.");
        } else {
            chest_opened = 1;
            gold = (u16)(gold + 10);
            hud_text();
            dialog_open("YOU FOUND 10 CREDITS!");
        }
    }
}

int main(void) {
    u16 keys, cam_x, cam_y;
    u8 moving, i;

    consoleInit();
    setMode(BG_MODE1, 0x08);               /* BG3 high priority */

    /* BG1 the street, from the Tiled map */
    dmaCopyVram(tileset_tiles, VRAM_SCENE_TILES, (u16)(tileset_tiles_end - tileset_tiles));
    dmaCopyVram(town_street_quad, VRAM_SCENE_MAP, (u16)(town_street_quad_end - town_street_quad));
    bgSetGfxPtr(0, VRAM_SCENE_TILES);
    bgSetMapPtr(0, VRAM_SCENE_MAP, SC_64x64);
    dmaCopyCGram(tileset_pal, 0, 32);

    /* BG3 text overlay. BG2 shares its character base, and an empty cell of
     * a panel is tile 0: the first 16 words stay zero, a blank 4bpp tile,
     * and the font starts after them (2bpp tiles 0-1 are those words). */
    dmaFillVRAM(0, VRAM_FONT, 32);
    textInit(VRAM_TEXT_MAP, FONT_TILE, 4);
    text_config.priority = 1;
    textLoadFont((u16)(VRAM_FONT + FONT_TILE * 8));
    bgSetGfxPtr(2, VRAM_FONT);
    bgSetMapPtr(2, VRAM_TEXT_MAP, SC_32x32);
    setColor(4 * 4 + 1, RGB(29, 31, 31));

    /* BG2 dialog box: its tiles after the font, on the same character base */
    dmaCopyVram(uibox_tiles, VRAM_UI_TILES, (u16)(uibox_tiles_end - uibox_tiles));
    dmaCopyCGram(uibox_pal, BOX_PAL * 16, 32);
    bgSetGfxPtr(1, VRAM_FONT);
    bgSetMapPtr(1, VRAM_UI_MAP, SC_32x32);
    build_panel();

    /* OBJ: Kit's walk in the first name table, the residents in the second;
     * one palette each, at the CGRAM offset the plan gave it
     * (res/palettes.inc), so a new resident never collides with the others. */
    dmaCopyVram(hero_tiles, VRAM_OBJ, (u16)(hero_tiles_end - hero_tiles));
    dmaCopyCGram(hero_pal, PAL_HERO_CGRAM, 32);
    for (i = 0; i < RESIDENTS; i++) {
        load_resident(looks[i].tiles, RESIDENT_TILE(i));
        dmaCopyCGram(looks[i].pal, looks[i].cgram, 32);
    }
    oamInit(OBJ_SIZE8_L32, OBJ_NAME_BASE(VRAM_OBJ));

    /* spawn from the Tiled Entities layer */
    hero_x = SPAWN_TX * 8;
    hero_y = SPAWN_TY * 8;
    hero_facing = FACE_DOWN;
    chest_opened = 0;
    scene = SCENE_TOWN;
    gold = 0;
    hero_hp = HERO_MAX_HP;

    /* BG2 carries the HUD, so it is on screen for good — the dialog
     * panel just appears in its lower half when someone talks. */
    hud_icons();
    panelFlush(&ui);
    hud_text();
    setMainScreen(TM_BG1 | TM_BG2 | TM_BG3 | LAYER_OBJ);
    setBrightness(0);
    setScreenOn();
    game_state = ST_FADEIN;
    fade_level = 0;

    while (1) {
        WaitForVBlank();
        keys = padHeld(0);
        moving = 0;

        if (game_state == ST_FADEIN) {
            fade_level++;
            setBrightness((u8)(fade_level >> 1));
            if (fade_level >= 30) {
                setBrightness(15);
                game_state = ST_EXPLORE;
            }
        } else if (game_state == ST_EXPLORE) {
            if (step_count > 0) {
                hero_x = (u16)(hero_x + step_dx * 2);
                hero_y = (u16)(hero_y + step_dy * 2);
                step_count--;
                moving = 1;
                if (step_count == 0) {
                    /* the step just landed: on a door, or on the mat? */
                    if (scene == SCENE_TOWN
                        && hero_tx() == DOOR_TX && hero_ty() == DOOR_TY) {
                        scene_load(SCENE_HOUSE, ARRIVAL_TX, ARRIVAL_TY, FACE_UP);
                        /* the host greets you — no button to press */
                        dialog_open(HOST_TEXT);
                    } else if (scene == SCENE_HOUSE
                               && hero_tx() == MAT_TX && hero_ty() == MAT_TY) {
                        scene_load(SCENE_TOWN, DOOR_TX, DOOR_TY + MT, FACE_DOWN);
                    }
                }
            } else {
                if (keys & KEY_UP) {
                    hero_facing = FACE_UP; begin_step(0, -1); moving = 1;
                } else if (keys & KEY_DOWN) {
                    hero_facing = FACE_DOWN; begin_step(0, 1); moving = 1;
                } else if (keys & KEY_LEFT) {
                    hero_facing = FACE_LEFT; begin_step(-1, 0); moving = 1;
                } else if (keys & KEY_RIGHT) {
                    hero_facing = FACE_RIGHT; begin_step(1, 0); moving = 1;
                } else if (padPressed(0) & KEY_A) {
                    interact();
                }
            }
        } else {                            /* ST_DIALOG */
            if (padPressed(0) & KEY_A) {
                dialog_close();
            }
        }

        /* the walk: frames 1-4 of the facing, one every 4 frames; 0 standing */
        if (moving) {
            anim_tick++;
            if (anim_tick >= 4) {
                anim_tick = 0;
                walk_frame = (u8)(walk_frame >= 4 ? 1 : walk_frame + 1);
            }
        } else {
            walk_frame = 0;
            anim_tick = 0;
        }

        /* The den is exactly one screen, so it does not scroll.
         * The street scrolls with the hero, clamped to the world. */
        if (scene == SCENE_HOUSE) {
            cam_x = 0;
            cam_y = 0;
        } else {
            cam_x = (hero_x > SCREEN_CX) ? (u16)(hero_x - SCREEN_CX) : 0;
            cam_y = (hero_y > SCREEN_CY) ? (u16)(hero_y - SCREEN_CY) : 0;
            if (cam_x > WORLD_W - 256) { cam_x = WORLD_W - 256; }
            if (cam_y > WORLD_H - 224) { cam_y = WORLD_H - 224; }
            bgSetScroll(0, cam_x, cam_y);
        }

        draw_char(0, hero_x, hero_y, cam_x, cam_y, hero_tile(), PAL_HERO_SLOT,
                  hero_facing == FACE_LEFT ? OBJ_FLIPX : 0);
        if (scene == SCENE_HOUSE) {
            draw_char(1, HOST_TX * 8, HOST_TY * 8, cam_x, cam_y,
                      RESIDENT_TILE(HOST_LOOK), looks[HOST_LOOK].slot, 0);
            for (i = 1; i < NPC_COUNT; i++) {
                oamHide((u8)(i + 1));
            }
        } else {
            for (i = 0; i < NPC_COUNT; i++) {
                draw_char((u8)(i + 1), (u16)(npcs[i].tx * 8), (u16)(npcs[i].ty * 8),
                          cam_x, cam_y, RESIDENT_TILE(npcs[i].look),
                          looks[npcs[i].look].slot, 0);
            }
        }
    }

    return 0;
}
