/**
 * @file main.c
 * @brief RPG template (#7) — Tiled-driven overworld with dialog boxes
 * @ingroup examples
 *
 * The SDK's modules composed into a playable RPG skeleton. Two things
 * make it a template rather than a demo:
 *
 * 1. **The map is a real Tiled map** (`res/town.tmj`): terrain,
 *    per-tile collision (the `attribute` property), entity positions
 *    AND each villager's dialogue line (a `text` property on the
 *    object) all live in the map, not in the code. Adding a villager
 *    is a map edit. Edit it in Tiled, re-run `gen_assets.py`, rebuild.
 * 2. **A real bordered dialog box and a HUD**, both 9-slice panels on
 *    BG2 with their text on BG3 above — the classic SNES RPG window.
 * 3. **Two scenes**: the town, and the inside of the blue-roofed house.
 *    A scene is a tileset + a palette + a tilemap + a collision map +
 *    entities; switching one is four DMAs under force blank. The
 *    interior is its own Tiled map (`res/house.tmj`) with its own
 *    16-colour palette, so neither scene gives up colours for the other.
 *
 * Layer roles (Mode 1):
 * - BG1: the town (4bpp, 64x64 scrolling)
 * - BG2: the HUD (always) and the dialog panel (while talking)
 * - BG3: the dialog text (2bpp, high priority)
 * - OBJ: the hero and the villagers (same tiles, two palettes)
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - Tiled (.tmj) as the content pipeline: collision and entities are
 *   data, not hardcoded
 * - Tile-exact collision: the hero OCCUPIES one tile and its 16x16
 *   sprite is drawn straddling it (feet on the tile), so what you see
 *   is what collides — the classic top-down RPG convention
 * - A 9-slice dialog box DMA'd to BG2 on open, with layer priorities
 *   stacking town < box < text
 * - Forced blank (`setScreenOff`, INIDISP bit 7) around a multi-KB VRAM
 *   upload — `setBrightness(0)` only blacks the screen, it does NOT open
 *   the VRAM write window, and the tail of the transfer is dropped
 * - A fixed 16-colour palette per scene, authored by hand rather than
 *   quantised: adding one tile to a quantised sheet re-derives the whole
 *   palette and every existing tile shifts hue
 * - Off-camera entities MUST be parked at OBJ_HIDE_Y, not just drawn:
 *   OAM coordinates wrap, so an entity two screens away reappears
 *   somewhere plausible on screen (a villager standing in the wall)
 * - Collision through the SDK's `collideTile()` over the Tiled map —
 *   its `const` tilemap parameter means the read is bank-honouring
 *   (#121), so a multi-KB map needs neither bank $00 nor RAM
 *
 * @par What to Observe
 * The town fades in, with hearts and a purse in the HUD. Walk with the
 * D-pad — houses, water, trees and the fence block you exactly where
 * they look. Face either villager and press A (each has its own line,
 * from the map), or step onto the chest and press A: the purse goes up
 * by 10. Walk into the door of the BLUE-roofed house, bottom right of
 * the crossroads, and you step inside — the host greets you with no
 * button press. The mat by the door takes you back out.
 *
 * @par Modules Used
 * console, dma, background, sprite, text, input, collision, panel
 *
 * @see gen_assets.py, res/town.tmj — the Tiled content pipeline
 */

#include <snes.h>
#include <snes/background.h>
#include <snes/sprite.h>
#include <snes/text.h>
#include <snes/input.h>
#include <snes/collision.h>
#include <snes/panel.h>

#include "res/entities.inc"     /* SPAWN/CHEST/NPC_TABLE from the .tmj */

/** @brief A villager: where it stands and what it says. The struct
 * shape and the rows both come from the Entities layer of town.tmj, so
 * adding one is a map edit. */
typedef struct { NPC_FIELDS } Npc;

/** @brief The villagers. `const`, so the table lives in ROM and is read
 * with bank-honouring far addressing — `npcs[i].tx` compiles to
 * `lda.l npcs,x` (this needed issue #132 fixed; before that a const
 * array of structs was read from bank $00). */
static const Npc npcs[NPC_COUNT] = NPC_TABLE;

extern u8 town_tiles[], town_tiles_end[];
extern u8 town_map[];
extern u8 town_pal[];
extern const u8 town_collision[];      /* const -> far reads (#121) */
extern u8 hero_tiles[], hero_tiles_end[];
extern u8 hero_pal[];
extern u8 npc_pal[];
extern u8 ui_tiles[], ui_tiles_end[];
extern u8 ui_pal[];
extern u8 house_tiles[], house_tiles_end[];
extern u8 house_map[];
extern u8 house_pal[];
extern const u8 house_collision[];     /* const -> far reads (#121) */

/* VRAM word layout */
#define VRAM_TOWN_TILES 0x0000
#define VRAM_HOUSE_TILES 0x1000
#define VRAM_TOWN_MAP   0x2000
#define VRAM_FONT       0x3000
#define VRAM_TEXT_MAP   0x3800
#define VRAM_UI_TILES   0x4000
#define VRAM_UI_MAP     0x4400
#define VRAM_HERO       0x6000

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
#define ICON_COIN 11
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
#define TEXT_W  (PANEL_W - 4)     /* usable characters per line */

#define FACE_DOWN 0
#define FACE_UP   1
#define FACE_LEFT 2
#define FACE_RIGHT 3

#define MAP_TILES 64
#define WORLD_W (MAP_TILES * 8)
#define WORLD_H (MAP_TILES * 8)
#define SCREEN_CX 124             /* where the hero tile sits on screen */
#define SCREEN_CY 108

enum { ST_FADEIN, ST_EXPLORE, ST_DIALOG };
enum { SCENE_TOWN, SCENE_HOUSE };

#define MAP_TILES_HOUSE 32

/** @brief Probe oracles / state. hero_x/hero_y are the PIXEL position
 * of the tile the hero occupies (a multiple of 8 when idle). */
u16 hero_x;
u16 hero_y;
u8  hero_facing;
u8  game_state;
u8  chest_opened;
u8  scene;                 /**< SCENE_TOWN or SCENE_HOUSE */
u16 gold;                  /**< HUD: coins, +10 per chest */
u8  hero_hp;               /**< HUD: static in this template */

static s8 step_dx, step_dy;
static u8 step_count;
static u8 walk_phase, anim_tick;
static u8 fade_level;
static u16 panel_map[32 * 32];

/* ---- collision: the SDK's collideTile() over the Tiled map ----
 * collideTile takes PIXEL coordinates, converts to 8x8 tiles and
 * returns the map byte (nonzero = solid). It bounds X and negative
 * coordinates itself (off-map = solid) but cannot bound Y — it does
 * not know the map height — so we clamp that here, as its docs ask.
 * Its `tilemap` parameter is const, so the read is bank-honouring:
 * our 4 KB map lives outside bank $00. */
static u8 tile_walkable(u16 tx, u16 ty) {
    if (scene == SCENE_HOUSE) {
        if (ty >= MAP_TILES_HOUSE) {
            return 0;
        }
        return (u8)(collideTile((s16)(tx << 3), (s16)(ty << 3),
                                house_collision, MAP_TILES_HOUSE) == 0);
    }
    if (ty >= MAP_TILES) {
        return 0;
    }
    return (u8)(collideTile((s16)(tx << 3), (s16)(ty << 3),
                            town_collision, MAP_TILES) == 0);
}

static u16 hero_tx(void) { return (u16)(hero_x >> 3); }
static u16 hero_ty(void) { return (u16)(hero_y >> 3); }

static void front_tile(u16 *tx, u16 *ty) {
    u16 cx = hero_tx();
    u16 cy = hero_ty();
    switch (hero_facing) {
    case FACE_UP:    cy--; break;
    case FACE_DOWN:  cy++; break;
    case FACE_LEFT:  cx--; break;
    case FACE_RIGHT: cx++; break;
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
    0,                  /* base_tile — the sheet starts at tile 0 */
    4,                  /* stride — 4 wide: col 4 holds the HUD icons */
    BOX_PAL,            /* palette    */
    1,                  /* priority — in front of the opaque town */
};

/**
 * @brief Draw the HUD icons: hearts for HP, a coin for the purse.
 *
 * The icons are tiles on BG2 (they came out of the same 4x3 sheet as
 * the dialog border), the number is text on BG3. Splitting it that way
 * costs nothing: both layers are already up for the dialog box.
 */
static void hud_icons(void) {
    u16 i;
    for (i = 0; i < HERO_MAX_HP; i++) {
        panelPut(&ui, (u8)(1 + i), 1,
                 (i < hero_hp) ? ICON_HEART : ICON_HEART_EMPTY);
    }
    panelPut(&ui, 8, 1, ICON_COIN);
}

/** @brief The HUD's numeric half, on BG3. Re-drawn after every
 * textClearRect, since clearing the dialog does not touch these rows. */
static void hud_text(void) {
    textPrintAt(10, 1, "     ");   /* erase the previous amount */
    textSetPos(10, 1);
    textPrintU16(gold);
}

static void build_panel(void) {
    panelInit(&ui);
    panelDraw(&ui, HUD_X, HUD_Y, HUD_W, HUD_H);
    hud_icons();
}

static void dialog_open(const char *line) {
    panelDraw(&ui, PANEL_X, PANEL_Y, PANEL_W, PANEL_H);
    panelFlush(&ui);
    textPrintAt(PANEL_X + 2, PANEL_Y + 2, line);
    textPrintAt(PANEL_X + 2, PANEL_Y + 4, "         (A) OK");
    game_state = ST_DIALOG;
}

static void dialog_close(void) {
    /* clear only the dialog rows: textClear() would wipe the HUD too */
    textClearRect(PANEL_X, PANEL_Y, PANEL_W, PANEL_H);
    panelClear(&ui, PANEL_X, PANEL_Y, PANEL_W, PANEL_H);
    panelFlush(&ui);
    game_state = ST_EXPLORE;
}

/* ---- scenes: town <-> house interior ----
 * A scene is a tileset, a palette, a tilemap, a collision map and a set
 * of entities. Switching one is four DMAs under force blank — there is
 * no engine here, just the data swapped over. */
static void scene_load(u8 which, u16 tx, u16 ty, u8 facing) {
    setScreenOff();                 /* forced blank: 8 KB will not fit VBlank */
    if (which == SCENE_HOUSE) {
        dmaCopyVram(house_tiles, VRAM_HOUSE_TILES,
                    (u16)(house_tiles_end - house_tiles));
        dmaCopyVram(house_map, VRAM_TOWN_MAP, 32 * 32 * 2);
        dmaCopyCGram(house_pal, 0, 32);
        bgSetGfxPtr(0, VRAM_HOUSE_TILES);
        bgSetMapPtr(0, VRAM_TOWN_MAP, SC_32x32);
        bgSetScroll(0, 0, 0);
    } else {
        dmaCopyVram(town_tiles, VRAM_TOWN_TILES,
                    (u16)(town_tiles_end - town_tiles));
        dmaCopyVram(town_map, VRAM_TOWN_MAP, 8192);
        dmaCopyCGram(town_pal, 0, 32);
        bgSetGfxPtr(0, VRAM_TOWN_TILES);
        bgSetMapPtr(0, VRAM_TOWN_MAP, SC_64x64);
    dmaCopyCGram(town_pal, 0, 32);
    }
    scene = which;
    hero_x = (u16)(tx * 8);
    hero_y = (u16)(ty * 8);
    hero_facing = facing;
    step_count = 0;
    setScreenOn();
}

/* ---- movement: one tile per step, slid over 8 frames ---- */
static void begin_step(s8 dx, s8 dy) {
    u16 ntx = (u16)(hero_tx() + dx);
    u16 nty = (u16)(hero_ty() + dy);
    if (tile_walkable(ntx, nty)) {
        step_dx = dx;
        step_dy = dy;
        step_count = 8;
    }
}

/**
 * @brief Draw a 16x16 character straddling its 8x8 tile, or hide it.
 *
 * Two things happen here, both of them load-bearing.
 *
 * **The straddle** is what makes collision feel exact: the character's
 * logical position is one tile; the sprite is drawn 4 px left and 8 px
 * up of it, so its feet stand on that tile and its body overhangs
 * upward — the standard top-down RPG convention. Drawing the sprite at
 * the tile's corner instead (the naive version) puts the visible body
 * half a tile away from what collides.
 *
 * **The cull** is not an optimisation, it is correctness. OAM X is 9
 * bits and Y is 8, so an entity standing outside the camera does not
 * quietly vanish — its coordinates WRAP and it reappears somewhere
 * plausible-looking on screen. A villager two screens away shows up
 * standing inside the town wall. Anything off-camera goes to oamHide(),
 * which parks it properly (Y AND the X high bit — Y alone still wraps
 * for sprites over 16 px tall).
 */
static void draw_char(u8 oam_id, u16 wx, u16 wy, u16 cam_x, u16 cam_y,
                      u8 facing, u8 phase, u8 palette) {
    s16 sx = (s16)((s16)wx - (s16)cam_x - 4);
    s16 sy = (s16)((s16)wy - (s16)cam_y - 8);
    u8 f;

    if (sx < -16 || sx > 255 || sy < -16 || sy > 223) {
        oamHide(oam_id);
        return;
    }
    f = (u8)(facing * 4 + phase * 2);
    oamSet(oam_id, (u16)sx, (u16)sy, f, palette, 2, 0);
    oamSetSize(oam_id, OBJ_LARGE);
}

int main(void) {
    u16 keys, cam_x, cam_y;
    u8 moving;

    consoleInit();
    setMode(BG_MODE1, 0x08);               /* BG3 high priority */

    /* BG1 town, from the Tiled map */
    dmaCopyVram(town_tiles, VRAM_TOWN_TILES, (u16)(town_tiles_end - town_tiles));
    dmaCopyVram(town_map, VRAM_TOWN_MAP, 8192);
    bgSetGfxPtr(0, VRAM_TOWN_TILES);
    bgSetMapPtr(0, VRAM_TOWN_MAP, SC_64x64);
    dmaCopyCGram(town_pal, 0, 32);

    /* BG3 text overlay */
    textInit(VRAM_TEXT_MAP, 0, 4);
    text_config.priority = 1;
    textLoadFont(VRAM_FONT);
    bgSetGfxPtr(2, VRAM_FONT);
    bgSetMapPtr(2, VRAM_TEXT_MAP, SC_32x32);
    setColor(4 * 4 + 1, RGB(31, 31, 31));

    /* BG2 dialog box */
    dmaCopyVram(ui_tiles, VRAM_UI_TILES, (u16)(ui_tiles_end - ui_tiles));
    dmaCopyCGram(ui_pal, BOX_PAL * 16, 32);
    bgSetGfxPtr(1, VRAM_UI_TILES);
    bgSetMapPtr(1, VRAM_UI_MAP, SC_32x32);
    build_panel();

    /* OBJ: hero (palette 0) and villager (palette 1) share the tiles */
    dmaCopyVram(hero_tiles, VRAM_HERO, (u16)(hero_tiles_end - hero_tiles));
    dmaCopyCGram(hero_pal, 128, 32);       /* OBJ palette 0 */
    dmaCopyCGram(npc_pal, 144, 32);        /* OBJ palette 1 */
    oamInit(OBJ_SIZE8_L16, OBJ_NAME_BASE(VRAM_HERO));

    /* spawn from the Tiled Entities layer */
    hero_x = SPAWN_TX * 8;
    hero_y = SPAWN_TY * 8;
    hero_facing = FACE_UP;
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
                hero_x = (u16)(hero_x + step_dx);
                hero_y = (u16)(hero_y + step_dy);
                step_count--;
                moving = 1;
                if (step_count == 0) {
                    /* the step just landed: did it land on a door? */
                    if (scene == SCENE_TOWN
                        && hero_tx() == DOOR_TX && hero_ty() == DOOR_TY) {
                        scene_load(SCENE_HOUSE, HOUSE_SPAWN_TX,
                                   HOUSE_SPAWN_TY, FACE_UP);
                        /* the host greets you — no button to press */
                        dialog_open(HOUSE_NPC_LINE);
                    } else if (scene == SCENE_HOUSE
                               && hero_tx() == HOUSE_EXIT_TX
                               && hero_ty() == HOUSE_EXIT_TY) {
                        scene_load(SCENE_TOWN, DOOR_TX, DOOR_TY + 1,
                                   FACE_DOWN);
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
                    u16 ftx, fty;
                    u8 i, talked = 0;
                    front_tile(&ftx, &fty);
                    if (scene == SCENE_HOUSE) {
                        if (ftx == HOUSE_NPC_TX && fty == HOUSE_NPC_TY) {
                            dialog_open(HOUSE_NPC_LINE);
                            talked = 1;
                        }
                    } else {
                        for (i = 0; i < NPC_COUNT; i++) {
                            if (ftx == npcs[i].tx && fty == npcs[i].ty) {
                                dialog_open(npcs[i].text);
                                talked = 1;
                                break;
                            }
                        }
                    }
                    if (talked || scene == SCENE_HOUSE) {
                        /* done */
                    } else if ((hero_tx() == CHEST_TX && hero_ty() == CHEST_TY)
                               || (ftx == CHEST_TX && fty == CHEST_TY)) {
                        if (chest_opened) {
                            dialog_open("THE CHEST IS EMPTY.");
                        } else {
                            chest_opened = 1;
                            gold = (u16)(gold + 10);
                            hud_text();
                            dialog_open("YOU FOUND 10 GOLD!");
                        }
                    }
                }
            }
        } else {                            /* ST_DIALOG */
            if (padPressed(0) & KEY_A) {
                dialog_close();
            }
        }

        if (moving) {
            anim_tick++;
            if (anim_tick >= 8) { anim_tick = 0; walk_phase ^= 1; }
        } else {
            walk_phase = 0;
        }

        /* The interior is exactly one screen, so it does not scroll.
         * The town scrolls with the hero, clamped to the world. */
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

        draw_char(0, hero_x, hero_y, cam_x, cam_y, hero_facing, walk_phase, 0);
        {
            u8 n;
            if (scene == SCENE_HOUSE) {
                draw_char(1, HOUSE_NPC_TX * 8, HOUSE_NPC_TY * 8,
                          cam_x, cam_y, FACE_DOWN, 0, 1);
                for (n = 1; n < NPC_COUNT; n++) {
                    oamHide((u8)(n + 1));
                }
            } else {
                /* the villagers: same tiles as the hero, palette 1 */
                for (n = 0; n < NPC_COUNT; n++) {
                    draw_char((u8)(n + 1), (u16)(npcs[n].tx * 8),
                              (u16)(npcs[n].ty * 8), cam_x, cam_y,
                              FACE_DOWN, 0, 1);
                }
            }
        }
    }

    return 0;
}
