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
 * The game: steer the arrow with the D-pad, touch the coin to score, before
 * the timer runs out. Zero assets — both sprites are built in C.
 *
 * ROM mode: LoROM (project default).
 *
 * @par Controls
 * - Title / Game over: START to begin / play again
 * - Play: D-pad to move; touch the coin to score
 *
 * @par SNES Concepts
 * - The frame loop: WaitForVBlank -> read input -> update -> the NMI flushes
 * - A plain `enum` + `switch` state machine (title / play / over)
 * - Sprites (player + coin) via the OAM buffer; text HUD via the auto-flush
 * - Bounding-box collision; rand()/srand() seeded for varied coin spawns
 *
 * @par What to Observe
 * Title shows "PRESS START". In play, moving onto the coin bumps SCORE and
 * respawns it; TIME counts down; at zero the game-over screen shows the
 * final score. `game_state`, `score`, `time_left` are the probe oracles.
 *
 * @par Modules Used
 * console, dma, background, sprite, text, input
 *
 * @see examples/basics/scene_stack — the same idea via the `scene` framework
 */

#include <snes.h>
#include <snes/input.h>

/** @brief Game states — the whole game is a switch over these. */
#define ST_TITLE 0
#define ST_PLAY  1
#define ST_OVER  2

#define SPR_VRAM   0x4000   /**< sprite tiles (text font is at $0000) */
#define TILE_PLAYER 0
#define TILE_COIN   1

#define PLAYER_SPEED 2
#define START_TIME   30     /**< seconds on the clock */
#define PX_MIN 8
#define PX_MAX 240
#define PY_MIN 24
#define PY_MAX 208

/** @brief Probe oracle: current state (0 title, 1 play, 2 over). */
u8 game_state;
/** @brief Probe oracle: coins collected. */
u16 score;
/** @brief Probe oracle: seconds remaining in a round. */
u8 time_left;

static u8 px, py;          /* player position */
static u8 cx, cy;          /* coin position   */
static u8 tick;            /* frame counter for the 1-second timer */
static u16 seed_ctr;       /* advances on the title so each run differs */

static u8 pixbuf[64];
static u8 tilebuf[32];

/** @brief Pack pixbuf[] (8x8 palette indices) into a 4bpp planar tile. */
static void encode_4bpp(void) {
    u8 pair, row, col;
    u16 o = 0;
    for (pair = 0; pair < 4; pair += 2) {
        for (row = 0; row < 8; row++) {
            u8 lo = 0, hi = 0;
            for (col = 0; col < 8; col++) {
                u8 v = pixbuf[row * 8 + col];
                if (v & (1 << pair))       lo |= (u8)(0x80 >> col);
                if (v & (1 << (pair + 1))) hi |= (u8)(0x80 >> col);
            }
            tilebuf[o++] = lo;
            tilebuf[o++] = hi;
        }
    }
}

/** @brief Build an 8x8 tile from an 8-row bitmap, using palette index @p idx. */
static void build_tile(const u8 *rows, u8 idx, u16 tile) {
    u8 r, c;
    for (r = 0; r < 8; r++)
        for (c = 0; c < 8; c++)
            pixbuf[r * 8 + c] = (u8)(((rows[r] >> (7 - c)) & 1) ? idx : 0);
    encode_4bpp();
    dmaCopyVram(tilebuf, (u16)(SPR_VRAM + tile * 16), 32);
}

/** @brief Move the coin to a fresh pseudo-random spot. */
static void spawn_coin(void) {
    cx = (u8)(16 + (rand() % 216));
    cy = (u8)(PY_MIN + (rand() % (PY_MAX - PY_MIN)));
}

/** @brief Draw the two gameplay sprites into the OAM buffer. */
static void draw_sprites(void) {
    oamSetFast(0, px, py, TILE_PLAYER, 0, 3, 0);
    oamSetFast(1, cx, cy, TILE_COIN,   1, 3, 0);
}

/** @brief Title screen text. */
static void show_title(void) {
    textClear();
    textPrintAt(10, 10, "TINY QUEST");
    textPrintAt(8, 14, "PRESS START");
    oamClear();               /* no gameplay sprites on the title */
}

/** @brief Print the fixed HUD labels once (numbers refreshed as they change). */
static void draw_hud_labels(void) {
    textClear();
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
    srand(seed_ctr);          /* varied coin spawns per run */
    score = 0;
    time_left = START_TIME;
    tick = 0;
    px = 120;
    py = 112;
    spawn_coin();
    draw_hud_labels();
    draw_score();
    draw_time();
    draw_sprites();
    game_state = ST_PLAY;
}

/** @brief Game-over screen. */
static void show_over(void) {
    textClear();
    textPrintAt(9, 10, "GAME OVER");
    textPrintAt(8, 13, "SCORE");
    textSetPos(14, 13);
    textPrintU16(score);
    textPrintAt(8, 16, "PRESS START");
    oamClear();
    game_state = ST_OVER;
}

/** @brief One frame of gameplay. */
static void update_play(u16 held) {
    u8 dx, dy;

    /* Move the player, clamped to the field. */
    if ((held & KEY_LEFT)  && px > PX_MIN) px -= PLAYER_SPEED;
    if ((held & KEY_RIGHT) && px < PX_MAX) px += PLAYER_SPEED;
    if ((held & KEY_UP)    && py > PY_MIN) py -= PLAYER_SPEED;
    if ((held & KEY_DOWN)  && py < PY_MAX) py += PLAYER_SPEED;

    /* Bounding-box overlap with the coin (both 8x8). */
    dx = (u8)((px > cx) ? px - cx : cx - px);
    dy = (u8)((py > cy) ? py - cy : cy - py);
    if (dx < 8 && dy < 8) {
        score++;
        spawn_coin();
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
    textModeInit();                 /* BG_MODE0, text on BG1, font at $0000 */
    setColor(0, RGB(1, 2, 6));      /* dark blue backdrop */

    /* Build the two sprite tiles (arrow player, round coin). */
    {
        static const u8 player[8] = {0x18,0x3C,0x7E,0xFF,0x3C,0x3C,0x3C,0x00};
        static const u8 coin[8]   = {0x00,0x3C,0x7E,0x7E,0x7E,0x3C,0x00,0x00};
        build_tile(player, 1, TILE_PLAYER);
        build_tile(coin,   1, TILE_COIN);
    }
    setColor(OBJ_CGRAM_BASE + 0 * 16 + 1, RGB(10, 28, 31));  /* player: cyan   */
    setColor(OBJ_CGRAM_BASE + 1 * 16 + 1, RGB(31, 28,  6));  /* coin:   yellow */

    oamInit(OBJ_SIZE8_L16, SPR_VRAM >> 13);
    oamClear();

    setMainScreen(LAYER_BG1 | LAYER_OBJ);

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
