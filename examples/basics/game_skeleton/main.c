/**
 * @file main.c
 * @brief Game skeleton — the smallest complete game: title -> play -> over
 * @ingroup examples
 *
 * The capstone the curriculum was missing: not a feature demo but the
 * *shape* of a whole game, small enough to read in one sitting and fork
 * for your own. It recombines the rungs you've climbed — input drives a
 * sprite, a second sprite is a goal, a bounding-box test scores it, text
 * is the HUD, and a hand-rolled state machine ties title, play and
 * game-over together.
 *
 * The one structural idea: **a game is a state machine around one loop.**
 * Every frame you `WaitForVBlank()`, read the pad, then `switch
 * (game_state)` to the code for the screen you're on. Transitions are just
 * assignments to `game_state` plus a one-time setup for the new screen.
 * That is the skeleton under every SNES game; `basics/scene_stack` shows
 * the same idea lifted into the opt-in `scene` framework once the enum +
 * switch starts to sprawl.
 *
 * The game: walk Kit across a square of the Rue basse with the D-pad and
 * pick up the data shard before the timer runs out; each one picked up
 * respawns elsewhere. The title and the game-over screens show the logo.
 *
 * ROM mode: LoROM (project default).
 *
 * @par Controls
 * - Title / Game over: START to begin / play again
 * - Play: D-pad to walk; reach the shard to score
 *
 * @par SNES Concepts
 * - The frame loop: WaitForVBlank -> read input -> update -> the NMI flushes
 * - A plain `enum` + `switch` state machine (title / play / over)
 * - Mode 1: BG1 the play square, BG2 the logo, BG3 the text in front;
 *   each screen picks its layers with setMainScreen()
 * - Sprites (Kit + the shard) via the OAM buffer; text HUD via the auto-flush
 * - Bounding-box collision on a pickup box smaller than the sprites;
 *   rngNext()/rngSeed() seeded for varied shard spawns
 *
 * @par What to Observe
 * Title shows the logo and "PRESS START". In play, Kit faces the way he
 * walks; reaching the turning shard bumps SCORE and respawns it; TIME
 * counts down; at zero the game-over screen shows the final score.
 * `game_state`, `score`, `time_left` are the probe oracles.
 *
 * @par Modules Used
 * console, dma, background, sprite, text, input
 *
 * @see examples/basics/scene_stack — the same idea via the `scene` framework
 */

#include <snes.h>
#include <snes/input.h>
#include <snes/text.h>

#include "res/field.inc"
#include "res/logo.inc"
#include "res/cast.inc"
/* generated: field_, logo_ tiles/_pal/_map, cast_tiles/_pal (each with _end) */

/** @brief Game states — the whole game is a switch over these. */
#define ST_TITLE 0
#define ST_PLAY  1
#define ST_OVER  2

/** @name VRAM (word addresses): three maps, then each layer's tiles on a $1000 boundary
 *  @{ */
#define VRAM_BG1_MAP  0x0000   /**< the play square */
#define VRAM_BG2_MAP  0x0400   /**< the logo */
#define VRAM_BG3_MAP  0x0800   /**< the text */
#define VRAM_BG1_GFX  0x1000
#define VRAM_BG2_GFX  0x2000
#define VRAM_BG3_FONT 0x3000
#define SPR_VRAM      0x4000   /**< sprite tiles */
/** @} */

/** @name Frames of res/cast.png: Kit 32x32 four a row, then the shard 16x16
 *  @{ */
#define FRAME_DOWN  0          /**< Kit walking down: frames 0-4 */
#define FRAME_UP    5          /**< walking up: 5-9 */
#define FRAME_RIGHT 10         /**< walking right: 10-14; left mirrored */
#define TILE_SHARD  256        /**< the shard's frame k is tile 256 + 2k */
/** @} */

#define PLAYER_SPEED 2
#define START_TIME   30     /**< seconds on the clock */
/** @name The field Kit's pickup box may cover (inside the kerb, under the HUD)
 *  @{ */
#define PX_MIN 16
#define PX_MAX 232
#define PY_MIN 32
#define PY_MAX 200
/** @} */

/** @brief Probe oracle: current state (0 title, 1 play, 2 over). */
u8 game_state;
/** @brief Probe oracle: shards collected. */
u16 score;
/** @brief Probe oracle: seconds remaining in a round. */
u8 time_left;

/* Kit's and the shard's pickup boxes are 8x8, top-left corner here; the
 * sprites are drawn round them (Kit 32x32, the shard 16x16) */
static u8 px, py;          /* Kit's box */
static u8 cx, cy;          /* the shard's box */
static u8 tick;            /* frame counter for the 1-second timer */
static u8 anim;            /* frame counter for the walk and the shard's turn */
static u8 facing;          /* FRAME_DOWN, FRAME_UP or FRAME_RIGHT */
static u8 facing_left;     /* his right frames, mirrored */
static u8 moving;          /* a direction was held this frame */
static u16 seed_ctr;       /* advances on the title so each run differs */

/**
 * @brief The OAM tile of Kit's frame @p f: 32x32 frames are 4x4 tiles, and
 * a row of OBJ tiles is 16 wide, so frame f starts at column (f % 4) * 4 of
 * tile row (f / 4) * 4.
 */
static u16 kit_tile(u16 f) {
    return (u16)(((f & 3) << 2) + ((f >> 2) << 6));
}

/** @brief Move the shard to a fresh pseudo-random spot. */
static void spawn_shard(void) {
    cx = (u8)(16 + (rngNext() % 216));
    cy = (u8)(PY_MIN + (rngNext() % (PY_MAX - PY_MIN)));
}

/**
 * @brief Draw the two gameplay sprites into the OAM buffer
 *
 * Kit's 32x32 frame is drawn 12 px left of and 20 px above his box, so
 * the box sits at his feet; the shard's 16x16 frame is centred on its box.
 * Kit walks through five frames while he moves (six vblanks each); the
 * shard turns through eight (four vblanks each).
 */
static void draw_sprites(void) {
    u16 f = facing;
    if (moving)
        f += (anim / 6) % 5;
    oamSet(0, px - 12, py - 20, kit_tile(f), 0, 3, facing_left ? OBJ_FLIPX : 0);
    oamSet(1, cx - 4, cy - 4, TILE_SHARD + 2 * ((anim / 4) % 8), 1, 2, 0);
}

/** @brief Show the logo screen: BG2 and the text, no square, no sprites. */
static void logo_screen(void) {
    textClear();
    oamClear();
    setMainScreen(LAYER_BG2 | LAYER_BG3);
}

/** @brief Title screen: the logo, the game's name and the prompt. */
static void show_title(void) {
    logo_screen();
    textPrintAt(11, 2, "SHARD RUN");
    textPrintAt(10, 25, "PRESS START");
}

/** @brief Print the fixed HUD labels once (numbers refreshed as they change). */
static void draw_hud_labels(void) {
    textClear();
    setMainScreen(LAYER_BG1 | LAYER_BG3 | LAYER_OBJ);
    textPrintAt(2, 1, "SCORE");
    textPrintAt(18, 1, "TIME");
}

/** @brief Refresh the SCORE number (clear the field first so it never smears). */
static void draw_score(void) {
    textPrintAt(8, 1, "     ");
    textSetPos(8, 1);
    textPrintU16(score);
}

/** @brief Refresh the TIME number. */
static void draw_time(void) {
    textPrintAt(23, 1, "   ");
    textSetPos(23, 1);
    textPrintU16(time_left);
}

/** @brief Enter the play state: reset score/timer, centre the player. */
static void start_game(void) {
    rngSeed(seed_ctr);          /* varied shard spawns per run */
    score = 0;
    time_left = START_TIME;
    tick = 0;
    px = 120;
    py = 112;
    facing = FRAME_DOWN;
    facing_left = 0;
    moving = 0;
    oamSetSize(0, OBJ_LARGE);   /* the title's oamClear() reset the size bits */
    spawn_shard();
    draw_hud_labels();
    draw_score();
    draw_time();
    draw_sprites();
    game_state = ST_PLAY;
}

/** @brief Game-over screen. */
static void show_over(void) {
    logo_screen();
    textPrintAt(11, 2, "GAME OVER");
    textPrintAt(11, 4, "SCORE");
    textSetPos(17, 4);
    textPrintU16(score);
    textPrintAt(10, 25, "PRESS START");
    game_state = ST_OVER;
}

/** @brief One frame of gameplay. */
static void update_play(u16 held) {
    u8 dx, dy;

    /* Move Kit, clamped to the field; he faces the way he walks. */
    moving = 0;
    if ((held & KEY_LEFT)  && px > PX_MIN) { px -= PLAYER_SPEED; facing = FRAME_RIGHT; facing_left = 1; moving = 1; }
    if ((held & KEY_RIGHT) && px < PX_MAX) { px += PLAYER_SPEED; facing = FRAME_RIGHT; facing_left = 0; moving = 1; }
    if ((held & KEY_UP)    && py > PY_MIN) { py -= PLAYER_SPEED; facing = FRAME_UP;    facing_left = 0; moving = 1; }
    if ((held & KEY_DOWN)  && py < PY_MAX) { py += PLAYER_SPEED; facing = FRAME_DOWN;  facing_left = 0; moving = 1; }

    /* Bounding-box overlap of the two pickup boxes (both 8x8). */
    dx = (u8)((px > cx) ? px - cx : cx - px);
    dy = (u8)((py > cy) ? py - cy : cy - py);
    if (dx < 8 && dy < 8) {
        score++;
        spawn_shard();
        draw_score();
    }

    /* One-second tick. */
    if (++tick >= 60) {
        tick = 0;
        time_left--;
        draw_time();
        if (time_left == 0) {
            show_over();
            return;
        }
    }

    draw_sprites();
}

int main(void) {
    consoleInit();
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);
    setColor(0, RGB(2, 3, 5));      /* the backdrop: the night behind the logo */

    /* BG1: the play square, palette slot 0; BG2: the logo, slot 1 */
    bgInitTileSet(0, field_tiles, field_pal, 0, field_tiles_end - field_tiles,
                  field_pal_end - field_pal, BG_16COLORS, VRAM_BG1_GFX);
    bgInitTileSet(1, logo_tiles, logo_pal, 1, logo_tiles_end - logo_tiles,
                  logo_pal_end - logo_pal, BG_16COLORS, VRAM_BG2_GFX);
    bgSetMapPtr(0, VRAM_BG1_MAP, SC_32x32);
    bgSetMapPtr(1, VRAM_BG2_MAP, SC_32x32);
    dmaCopyVram(field_map, VRAM_BG1_MAP, field_map_end - field_map);
    dmaCopyVram(logo_map, VRAM_BG2_MAP, logo_map_end - logo_map);
    setColor(0, RGB(2, 3, 5));      /* bgInitTileSet loaded the square's colour 0 there */

    /* BG3: the text module's 2bpp font, palette slot 7, in front of both */
    textLoadFont(VRAM_BG3_FONT);
    textInit(VRAM_BG3_MAP, 0, 7);
    text_config.priority = 1;
    setColor(29, RGB(31, 31, 31));  /* slot 7, colour 1: white */
    bgSetGfxPtr(2, VRAM_BG3_FONT);
    bgSetMapPtr(2, VRAM_BG3_MAP, SC_32x32);

    /* Sprites: Kit's walk and the shard, OBJ palettes 0 and 1; Kit is the
     * LARGE size (32x32) of OBJ_SIZE16_L32, set by start_game(), the shard
     * the small one */
    oamInitGfxSet(cast_tiles, cast_tiles_end - cast_tiles,
                  cast_pal, cast_pal_end - cast_pal,
                  0, SPR_VRAM, OBJ_SIZE16_L32);
    oamClear();

    game_state = ST_TITLE;
    seed_ctr = 0;
    show_title();

    WaitForVBlank();
    setScreenOn();

    while (1) {
        u16 pressed, held;
        WaitForVBlank();
        pressed = padPressed(0);
        held    = padHeld(0);
        anim++;

        switch (game_state) {
        case ST_TITLE:
            seed_ctr++;                        /* entropy: when did you press? */
            if (pressed & KEY_START) start_game();
            break;
        case ST_PLAY:
            update_play(held);
            break;
        case ST_OVER:
            if (pressed & KEY_START) { game_state = ST_TITLE; show_title(); }
            break;
        }
    }

    return 0;
}
