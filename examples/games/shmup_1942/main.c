/**
 * @file main.c
 * @brief shmup_1942 — S3: player ship + D-pad over auto-scrolling terrain
 * @ingroup examples
 *
 * Adds a controllable 32×32 player ship to the S2 vertical-scroll demo.
 * The terrain continues its 1 px/frame auto-scroll while the player
 * moves freely with the D-pad, clamped to the visible window. The
 * ship is `ship_0000` from Kenney's `ships_packed.png` — a blue
 * chasseur — extracted by `res/extract_player.py` and reduced to
 * a 16-colour palette with the dark-blue background at index 0
 * (transparent in SNES sprite rendering).
 *
 * @par SNES Concepts
 * - OAM: `oamInitGfxSet` loads sprite tiles to VRAM and palette to
 *   CGRAM in one call, plus configures OBJSEL
 * - `OBJ_SIZE32_L64`: 32×32 small sprite, 64×64 large
 * - VRAM layout: BG1 tilemap at $0000, BG1 tiles at $4000, sprite
 *   tiles at $6000 (different 8 KB regions so they don't clash)
 * - Sprite palette in CGRAM 128+ (we use sprite palette 0 = 128-143)
 * - `bgSetScroll()` and `oamSet()` defer to the NMI handler via dirty
 *   flags, so update calls are cheap and apply atomically at VBlank
 *
 * @par What to Observe
 * - The grass terrain scrolls downward at 1 px/frame as in S2
 * - The blue ship sits over the scrolling background and responds to
 *   the D-pad: up/down/left/right move the ship by 2 px/frame
 * - The ship is clamped to the visible window (8..224 X, 16..192 Y)
 *   so it can't disappear off-screen
 *
 * @par Modules Used
 * console, dma, background, asset, sprite, input
 *
 * @see background.h, asset.h, sprite.h, input.h
 */

#include <snes.h>
#include <snes/asset.h>
#include <snes/input.h>

/**
 * @brief Composed scene bundle (4bpp, 16 colours, 32×64 tilemap).
 */
DECLARE_BG_ASSET(scene, BG_16COLORS, SC_32x64);

/** Player ship tile + palette data declared in data.asm. */
extern u8 player_tiles[], player_tiles_end[];
extern u8 player_pal[], player_pal_end[];

/* Map height in pixels — full tilemap, used for scroll wraparound. */
#define MAP_HEIGHT_PX 512

/* Ship movement speed in px/frame. */
#define PLAYER_SPEED 2

/* Clamp bounds for the player ship (32×32 sprite, kept fully on screen).
 * Visible window: 256×224. With a 32-px sprite anchored at its top-left,
 * X ∈ [0, 224] and Y ∈ [0, 192] keeps every pixel visible. Pull in by 8
 * to leave a small margin from the screen edge. */
#define PLAYER_MIN_X  8
#define PLAYER_MAX_X  216
#define PLAYER_MIN_Y  16
#define PLAYER_MAX_Y  176

/**
 * @brief Centralized game state.
 *
 * CRITICAL: must use an s16 struct, NOT loose `static u16` globals — the
 * latter pattern is documented in continuous_scroll/main.c as triggering
 * a compiler issue where horizontal movement silently fails.
 */
typedef struct {
    s16 player_x;
    s16 player_y;
    u16 scroll_y;
} GameState;

static GameState game;

int main(void) {
    setScreenOff();

    /* BG1: tilemap at VRAM $0000, tiles at $4000, palette slot 0.
     * SC_32x64 (2 KB tilemap) supports the 256×512 procedural scene. */
    bgLoad(0, &scene, 0, 0x4000, 0x0000);

    /* Sprite tiles at VRAM $6000 (8 KB above BG tiles, no overlap).
     * Sprite palette goes into CGRAM 128 (sprite palette 0).
     * OBJ_SIZE32_L64 → small sprites are 32×32 = our ship. */
    oamInitGfxSet(player_tiles, (u16)(player_tiles_end - player_tiles),
                  player_pal,   (u16)(player_pal_end   - player_pal),
                  0, 0x6000, OBJ_SIZE32_L64);

    setMode(BG_MODE1, 0);
    setMainScreen(TM_BG1 | TM_OBJ);

    /* Initial state: ship bottom-centre, BG at top. */
    game.player_x = 112;
    game.player_y = 160;
    game.scroll_y = 0;

    oamSet(0, game.player_x, game.player_y, 0, 0, 2, 0);
    bgSetScroll(0, 0, 0);

    setScreenOn();

    while (1) {
        WaitForVBlank();

        /* Auto-scroll the terrain downward by 1 px/frame, wrapped to
         * the map height so it cycles indefinitely. */
        game.scroll_y = (game.scroll_y + 1) % MAP_HEIGHT_PX;

        /* D-pad: 4-way movement with screen-edge clamping. */
        u16 pad = padHeld(0);
        if (pad & KEY_LEFT  && game.player_x > PLAYER_MIN_X) game.player_x -= PLAYER_SPEED;
        if (pad & KEY_RIGHT && game.player_x < PLAYER_MAX_X) game.player_x += PLAYER_SPEED;
        if (pad & KEY_UP    && game.player_y > PLAYER_MIN_Y) game.player_y -= PLAYER_SPEED;
        if (pad & KEY_DOWN  && game.player_y < PLAYER_MAX_Y) game.player_y += PLAYER_SPEED;

        oamSet(0, game.player_x, game.player_y, 0, 0, 2, 0);
        bgSetScroll(0, 0, game.scroll_y);
    }

    return 0;
}
