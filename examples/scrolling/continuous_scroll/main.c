/**
 * @file main.c
 * @brief A threshold camera over two layers in parallax: Kit runs on the roofs
 * @ingroup examples
 *
 * Kit runs along a stretch of roofs 512 px wide (BG1, a 64x32 map) in front
 * of the far towers (BG2). The camera follows him without a map engine:
 * while he is between two thresholds of the screen he moves and the world
 * stays; what he goes past one, the world scrolls instead and he is put
 * back on it, so he keeps his place on screen while the roofs go by. BG2 scrolls
 * at half BG1's speed, which is what makes the towers look far away.
 * Scroll registers are written via bgSetScroll(), which defers the actual
 * PPU register write to the NMI handler via a dirty flag.
 *
 * @par SNES Concepts
 * - Multi-layer parallax scrolling (BG1 + BG2 at independent speeds)
 * - A 64x32 background map: two 32x32 pages side by side, 512 px of world
 * - Deferred scroll register writes via NMI dirty flags
 * - Multiple tile sets and palette slots in VRAM/CGRAM
 * - A 32x32 sprite, animated and mirrored by its attribute's H-flip bit
 *
 * @par What to Observe
 * - Hold RIGHT: Kit runs to x 140 and stays there; the roofs scroll by
 *   under him and the towers behind them follow at half the speed
 * - At the end of the roofs the camera stops and Kit runs on to the edge
 * - Release: Kit stands and breathes; LEFT runs back
 *
 * @par Modules Used
 * console, sprite, input, background, dma
 *
 * @see background.h, sprite.h, input.h, dma.h
 */

#include <snes.h>

/*============================================================================
 * Graphics Data (converted by the build from res/, one settings file beside each picture)
 *============================================================================*/

#include "res/roofs.inc"
#include "res/skyline.inc"
#include "res/kit.inc"
/* generated: roofs_tiles/_pal/_map, skyline_tiles/_pal/_map, kit_tiles/_pal (each with _end) */

/*============================================================================
 * Game State
 *============================================================================*/

/** @name Scroll Configuration
 *  @brief Constants controlling the camera
 *  @{
 */
#define MAX_SCROLL_X 256            /**< BG1's last scroll: 512 px of roofs minus one 256 px screen */
#define SCROLL_THRESHOLD_RIGHT 140  /**< When Kit's x > this, scroll right */
#define SCROLL_THRESHOLD_LEFT  80   /**< When Kit's x < this, scroll left */
/** @} */

/** @brief Kit's sprite y: his feet (row 30 of the frame) on the roofs (picture row 144, scrolled by 8) */
#define KIT_Y        105
/** @brief The frames in res/kit.png, 32x32, four a row: idle 0-3, run 4-11 */
#define FRAME_IDLE   0
#define FRAME_RUN    4

/**
 * @struct GameState
 * @brief Centralized game state structure
 *
 * Coordinates are `s16` because the scroll thresholds subtract and compare
 * them: a signed type keeps "left of the threshold" a plain `<`.
 */
typedef struct {
    s16 player_x;       /**< Kit's X position (screen coords) */
    s16 player_y;       /**< Kit's Y position (screen coords) */
    s16 bg1_scroll_x;   /**< BG1 horizontal scroll offset */
    s16 bg1_scroll_y;   /**< BG1 vertical scroll offset */
    s16 bg2_scroll_x;   /**< BG2 horizontal scroll offset (parallax: half of BG1's) */
    s16 bg2_scroll_y;   /**< BG2 vertical scroll offset */
} GameState;

/** @brief Global game state instance with initial values */
GameState game = {20, KIT_Y, 0, 8, 0, 240};   /* BG2 at y 240 = -16: the towers 16 px lower */

/** @brief Kit faces left (his frames face right; the sprite is mirrored) */
static u8 facing_left;

/**
 * @brief The OAM tile number of frame @p f of the sheet
 *
 * A 32x32 frame is 4x4 tiles. VRAM rows of sprite tiles are 16 tiles wide,
 * so a frame row of four is one VRAM row of tiles: frame f starts at
 * column (f % 4) * 4 of tile row (f / 4) * 4.
 */
static u16 frame_tile(u16 f) {
    return (u16)(((f & 3) << 2) + ((f >> 2) << 6));
}

/*============================================================================
 * Main Program
 *============================================================================*/

/**
 * @brief Entry point -- a threshold camera over two layers in parallax.
 *
 * Loads the roofs (BG1, 64x32), the far towers (BG2, 64x32) and Kit's
 * frames, each in its own VRAM and CGRAM slot. Each frame, the D-pad moves
 * Kit; past a threshold the camera scrolls instead, BG2 at half speed.
 *
 * @return Does not return (infinite loop).
 */
int main(void) {
    u16 pad;              /**< Current joypad button state (read from hardware) */
    u16 frame;            /**< Kit's current frame */
    u16 tick = 0;         /**< Frames since the animation started */
    u8 running;
    s16 d;                /**< How far past a threshold Kit went this frame */

    consoleInit();

    /* Force blank during setup */
    setScreenOff();

    /*------------------------------------------------------------------------
     * Configure Background Tilemaps (where tilemap data goes in VRAM)
     *------------------------------------------------------------------------*/
    /* BG1 and BG2 maps, 64x32 each: two 32x32 pages (1024 words) side by side */
    bgSetMapPtr(0, 0x0000, SC_64x32);
    bgSetMapPtr(1, 0x0800, SC_64x32);

    /*------------------------------------------------------------------------
     * Load Background Tiles and Palettes
     *------------------------------------------------------------------------*/
    /* BG1: the roofs' 600 tiles at $1000 (19 KB, up to $357F), palette slot 2 */
    bgInitTileSet(0, roofs_tiles, roofs_pal, 2,
                  roofs_tiles_end - roofs_tiles,
                  roofs_pal_end - roofs_pal,
                  BG_16COLORS, 0x1000);

    /* BG2: the towers at $4000, palette slot 4. Must not overlap with BG1! */
    bgInitTileSet(1, skyline_tiles, skyline_pal, 4,
                  skyline_tiles_end - skyline_tiles,
                  skyline_pal_end - skyline_pal,
                  BG_16COLORS, 0x4000);

    /* The backdrop: the night behind the towers */
    setColor(0, RGB(2, 3, 5));

    /*------------------------------------------------------------------------
     * Load Tilemap Data (both pages of each map, 4 KB each)
     *------------------------------------------------------------------------*/
    dmaCopyVram(roofs_map, 0x0000, roofs_map_end - roofs_map);
    dmaCopyVram(skyline_map, 0x0800, skyline_map_end - skyline_map);

    /*------------------------------------------------------------------------
     * Load Sprite Graphics
     *------------------------------------------------------------------------*/
    /* Kit's 12 frames at $6000, palette 0 (colors 128-143), 32x32 large sprites */
    oamInitGfxSet(kit_tiles, kit_tiles_end - kit_tiles,
                  kit_pal, kit_pal_end - kit_pal,
                  0, 0x6000, OBJ_SIZE16_L32);

    /*------------------------------------------------------------------------
     * Configure Video Mode
     *------------------------------------------------------------------------*/
    setMode(BG_MODE1, 0);
    setMainScreen(LAYER_BG1 | LAYER_BG2 | LAYER_OBJ);

    oamSet(0, game.player_x, game.player_y, frame_tile(FRAME_IDLE), 0, 2, 0);
    oamSetSize(0, OBJ_LARGE);
    oamUpdate();

    /* Set the initial scroll BEFORE enabling display, so the first frame
     * already shows the right rows. */
    bgSetScroll(0, game.bg1_scroll_x, game.bg1_scroll_y);
    bgSetScroll(1, game.bg2_scroll_x, game.bg2_scroll_y);

    setScreenOn();

    /*------------------------------------------------------------------------
     * Main Loop
     *------------------------------------------------------------------------*/
    while (1) {
        WaitForVBlank();

        pad = padHeld(0);
        running = 0;

        /* Kit always moves when the D-pad is held */
        if (pad & KEY_LEFT) {
            if (game.player_x > 8) game.player_x -= 2;
            facing_left = 1;
            running = 1;
        }
        if (pad & KEY_RIGHT) {
            if (game.player_x < 216) game.player_x += 2;
            facing_left = 0;
            running = 1;
        }

        /* The threshold camera: what Kit went past a threshold, the world
         * scrolls instead, and he is put back on the threshold — so he keeps
         * his place on screen while there are roofs to scroll to */
        if (game.player_x > SCROLL_THRESHOLD_RIGHT && game.bg1_scroll_x < MAX_SCROLL_X) {
            d = game.player_x - SCROLL_THRESHOLD_RIGHT;
            if (d > MAX_SCROLL_X - game.bg1_scroll_x) d = MAX_SCROLL_X - game.bg1_scroll_x;
            game.bg1_scroll_x += d;
            game.player_x -= d;
        }
        if (game.player_x < SCROLL_THRESHOLD_LEFT && game.bg1_scroll_x > 0) {
            d = SCROLL_THRESHOLD_LEFT - game.player_x;
            if (d > game.bg1_scroll_x) d = game.bg1_scroll_x;
            game.bg1_scroll_x -= d;
            game.player_x += d;
        }
        /* Parallax: the far layer moves half as fast */
        game.bg2_scroll_x = game.bg1_scroll_x >> 1;

        /* Run: eight frames, five vblanks each; stand: four, twelve each */
        tick++;
        if (running)
            frame = FRAME_RUN + (tick / 5) % 8;
        else
            frame = FRAME_IDLE + (tick / 12) % 4;

        oamSet(0, game.player_x, game.player_y, frame_tile(frame), 0, 2,
               facing_left ? OBJ_FLIPX : 0);

        /* Update scroll — bgSetScroll defers to NMI via dirty flag */
        bgSetScroll(0, game.bg1_scroll_x, game.bg1_scroll_y);
        bgSetScroll(1, game.bg2_scroll_x, game.bg2_scroll_y);
    }

    return 0;
}
