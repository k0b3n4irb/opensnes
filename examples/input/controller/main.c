/**
 * @file main.c
 * @brief Both controllers on a terminal's test screen: buttons lit, the edges, a frame counter
 * @ingroup examples
 *
 * The pad test of a city terminal. The two controllers are drawn on screen,
 * and each button held lights up on its pad's drawing, several at once;
 * under each pad, the bitmask padHeld() returns, in hex. Under them, the last
 * button pressed and the last button released on pad 1, and the number of
 * VBlanks since boot. In the test bay at the bottom, each pad's D-pad walks
 * its avatar: Kit for pad 1, the raccoon for pad 2. This is the starting
 * point for understanding how SNES games read player input, and the screen
 * the console check uses.
 *
 * The SNES joypad has 12 buttons: 4 face buttons (A, B, X, Y), 2 shoulder
 * buttons (L, R), a D-pad (4 directions), START, and SELECT. The NMI
 * handler reads both joypads automatically every VBlank. Your code just
 * calls padHeld(pad) for the buttons down, padPressed(pad) for the ones
 * that went down on this frame, and padReleased(pad) for the ones that
 * came up.
 *
 * Ported from PVSnesLib "controller" example by alekmaul; the second pad
 * was the two_players example, the counter the timer example, both merged
 * here.
 *
 * @par SNES Concepts
 * - padHeld(n) — 16-bit bitmask of the buttons down on pad n (0 or 1)
 * - padPressed(n) / padReleased(n) — the edges: down this frame, up this frame
 * - KEY_A, KEY_B, KEY_X, KEY_Y, KEY_L, KEY_R, KEY_UP, KEY_DOWN, KEY_LEFT,
 *   KEY_RIGHT, KEY_START, KEY_SELECT — the bits, in the order the pad
 *   shifts them out (B Y SELECT START UP DOWN LEFT RIGHT A X L R)
 * - frame_count — the VBlanks since boot, counted by the NMI handler
 * - One lamp sprite per button, shown while its bit is set: the screen is
 *   a picture of the bitmask
 *
 * @par What to Observe
 * - Hold buttons on pad 1 or pad 2: they light up on that pad's drawing
 *   (cyan for pad 1, pink for pad 2) while held, several at once, and the
 *   HELD value under it shows their bits
 * - The last button pressed and released on pad 1 stay on screen
 * - Each D-pad walks its own avatar in the test bay
 * - The counter goes up by 60 a second (50 on PAL); L+R together resets it
 *
 * @par Modules Used
 * console, sprite, dma, background, text, input, gameloop
 *
 * @see input.h
 */

#include <snes.h>
#include <snes/text.h>
#include <snes/gameloop.h>

#include "res/screen.inc"
#include "res/cast.inc"

/** @name VRAM (word addresses): textModeInit() keeps $0000-$0FFF (font) and $3800 (its map)
 *  @{ */
#define SCREEN_TILES 0x1000
#define SCREEN_MAP   0x3C00
#define OBJ_VRAM     0x4000
/** @} */

/** @name The sheet's tiles (res/cast.png, 128 px wide: one row of the sheet is one row of OBJ tiles)
 *  @{ */
#define TILE_AVATAR  64        /**< pad p's avatar starts at tile 64 p; + 4 per facing */
#define TILE_LAMP    128       /**< the four 8x8 lamps: disc, square, pill, half a shoulder */
/** @} */

/** @brief Facings, in the sheet's order */
enum { SOUTH, NORTH, EAST, WEST };

/** @brief A lamp: the button it shows, its place on a pad's drawing, its tile */
typedef struct {
    u16 mask;   /**< the KEY_ bit */
    u8 x, y;    /**< from the pad's top-left (PAD_ORIGINS and PAD_LAMPS in build.py) */
    u8 tile;    /**< 0 disc, 1 square, 2 pill, 3 half a shoulder */
} Lamp;

/** @brief Number of lamps on a pad: one per button, two per shoulder */
#define LAMPS 14

/** @brief The lamps of one pad; the drawing has a button under each */
static const Lamp lamps[LAMPS] = {
    { KEY_B, 74, 31, 0 }, { KEY_Y, 65, 22, 0 }, { KEY_SELECT, 42, 30, 2 }, { KEY_START, 54, 30, 2 },
    { KEY_UP, 22, 14, 1 }, { KEY_DOWN, 22, 30, 1 }, { KEY_LEFT, 14, 22, 1 }, { KEY_RIGHT, 30, 22, 1 },
    { KEY_A, 83, 22, 0 }, { KEY_X, 74, 13, 0 },
    { KEY_L, 10, 0, 3 }, { KEY_L, 18, 0, 3 }, { KEY_R, 78, 0, 3 }, { KEY_R, 86, 0, 3 },
};

/** @brief Each pad's top-left on screen */
static const u8 pad_x[2] = { 16, 136 };
/** @brief Both pads' top edge (the shoulders) */
#define PAD_Y 32

/** @name The bay the avatars walk in: their 32x32 sprites stay inside its outline
 *  @{ */
#define BAY_X_MIN 18
#define BAY_X_MAX 206
#define BAY_Y_MIN 154
#define BAY_Y_MAX 174
/** @} */

/** @brief The 12 buttons in the order the pad sends them, for the edges */
static const u16 key_bit[12] = {
    KEY_B, KEY_Y, KEY_SELECT, KEY_START, KEY_UP, KEY_DOWN,
    KEY_LEFT, KEY_RIGHT, KEY_A, KEY_X, KEY_L, KEY_R
};
/** @brief Their names, padded to one width so a shorter one covers a longer */
static const char *const key_name[12] = {
    "B     ", "Y     ", "SELECT", "START ", "UP    ", "DOWN  ",
    "LEFT  ", "RIGHT ", "A     ", "X     ", "L     ", "R     "
};

/** @brief Probe oracle: each avatar's top-left, x (pad 1, pad 2) */
u16 avatar_x[2];
/** @brief Probe oracle: each avatar's top-left, y (pad 1, pad 2) */
u16 avatar_y[2];
/** @brief Each avatar's facing (SOUTH, NORTH, EAST, WEST) */
static u8 facing[2];

/** @brief Light pad n's lamps for the buttons held, and print its bitmask. */
static void show_pad(u8 pad) {
    u16 held = padHeld(pad);
    u8 i, id;

    for (i = 0; i < LAMPS; i++) {
        id = 2 + pad * LAMPS + i;           /* sprites 0 and 1 are the avatars */
        if (held & lamps[i].mask)
            oamSet(id, pad_x[pad] + lamps[i].x, PAD_Y + lamps[i].y,
                   TILE_LAMP + lamps[i].tile, 2 + pad, 3, 0);
        else
            oamHide(id);
    }
    textSetPos(pad ? 23 : 8, 11);
    textPrintHex(held, 4);
}

/** @brief Walk pad n's avatar with its D-pad, inside the bay. */
static void walk(u8 pad) {
    u16 held = padHeld(pad);

    if ((held & KEY_LEFT) && avatar_x[pad] > BAY_X_MIN) {
        avatar_x[pad]--;
        facing[pad] = WEST;
    }
    if ((held & KEY_RIGHT) && avatar_x[pad] < BAY_X_MAX) {
        avatar_x[pad]++;
        facing[pad] = EAST;
    }
    if ((held & KEY_UP) && avatar_y[pad] > BAY_Y_MIN) {
        avatar_y[pad]--;
        facing[pad] = NORTH;
    }
    if ((held & KEY_DOWN) && avatar_y[pad] < BAY_Y_MAX) {
        avatar_y[pad]++;
        facing[pad] = SOUTH;
    }
    oamSet(pad, avatar_x[pad], avatar_y[pad],
           TILE_AVATAR * pad + 4 * facing[pad], pad, 2, 0);
}

/** @brief The name of the first button set in a mask, for the edges. */
static const char *first_name(u16 mask) {
    u8 i;
    for (i = 0; i < 12; i++)
        if (mask & key_bit[i])
            return key_name[i];
    return 0;
}

/** @brief Once, in force blank: the screen, the sheet, the labels. */
static void on_init(void) {
    u8 i;

    textModeInit();                 /* Mode 0, the text on BG1, font at $0000 */

    /* BG2: the terminal and its two pads, four colours at CGRAM 32 (Mode 0
     * gives each BG its own palettes: BG_4COLORS0 puts palette 0 of BG2 there) */
    bgInitTileSet(1, screen_tiles, screen_pal, 0,
                  screen_tiles_end - screen_tiles,
                  screen_pal_end - screen_pal,
                  BG_4COLORS0, SCREEN_TILES);
    bgSetMapPtr(1, SCREEN_MAP, SC_32x32);
    dmaCopyVram(screen_map, SCREEN_MAP, screen_map_end - screen_map);
    setColor(0, RGB(1, 2, 3));      /* the backdrop, round the bezel */

    /* The avatars are 32x32 (LARGE), the lamps 8x8 (small); four OBJ palettes:
     * Kit, the raccoon, pad 1's glow, pad 2's glow */
    oamInitGfxSet(cast_tiles, cast_tiles_end - cast_tiles,
                  cast_pal, cast_pal_end - cast_pal,
                  0, OBJ_VRAM, OBJ_SIZE8_L32);
    for (i = 0; i < 2; i++) {
        oamSetSize(i, OBJ_LARGE);
        avatar_x[i] = i ? 168 : 56;
        avatar_y[i] = BAY_Y_MIN + 10;
        facing[i] = SOUTH;
    }

    setMainScreen(LAYER_BG1 | LAYER_BG2 | LAYER_OBJ);

    textPrintAt(8, 1, "PAD TEST  RUE BASSE");
    textPrintAt(2, 3, "PAD 1");
    textPrintAt(17, 3, "PAD 2");
    textPrintAt(2, 11, "HELD $");
    textPrintAt(17, 11, "HELD $");
    textPrintAt(4, 13, "PRESSED  -");
    textPrintAt(4, 14, "RELEASED -");
    textPrintAt(4, 16, "FRAMES");
    textPrintAt(4, 17, "L+R RESETS THE COUNT");
    WaitForVBlank();
    setScreenOn();
}

/** @brief Every frame: the lamps, the edges, the counter, the avatars. */
static void on_update(void) {
    const char *name;

    show_pad(0);
    show_pad(1);
    walk(0);
    walk(1);

    name = first_name(padPressed(0));
    if (name)
        textPrintAt(15, 13, name);
    name = first_name(padReleased(0));
    if (name)
        textPrintAt(15, 14, name);

    /* L+R resets once, on the frame the pair is completed (one of the two
     * just went down while the other is held), not on every frame held */
    if ((padHeld(0) & (KEY_L | KEY_R)) == (KEY_L | KEY_R)
            && (padPressed(0) & (KEY_L | KEY_R)))
        frame_count = 0;
    textSetPos(15, 16);
    textPrintU16(frame_count);
    textPrint("     ");
}

/**
 * @brief Entry point — the game loop runs on_init once, then on_update each frame.
 * @return Never returns
 */
int main(void) {
    GameLoopConfig cfg = {
        .init = on_init,
        .update = on_update,
    };
    gameLoopRun(&cfg);
    return 0;
}
