/**
 * @file play.c
 * @brief The round: walk Kit to GOAL shards before the clock runs out
 * @ingroup examples
 *
 * The shards come in a fixed order (a table), not at random: a round is the
 * same every time, which is what lets a test play it with a scripted pad.
 */
#include <snes.h>
#include <snes/input.h>
#include <snes/text.h>
#include <snes/input.h>
#include <snes/vramqueue.h>
#include <snes/debug.h>
#include "res/field.inc"
#include "res/cast.inc"
#include "game.h"
#include "screen.h"

/** @name Frames of res/cast.png: Kit 32x32 four a row, then the shard 16x16
 *  @{ */
#define FRAME_DOWN  0          /**< Kit walking down: frames 0-4 */
#define FRAME_UP    5          /**< walking up: 5-9 */
#define FRAME_RIGHT 10         /**< walking right: 10-14; left mirrored */
#define TILE_SHARD  256        /**< the shard's frame k is tile 256 + 2k */
/** @} */

#define PLAYER_SPEED 2
/** @name The field Kit's box may cover (inside the kerb, under the HUD) @{ */
#define PX_MIN 16
#define PX_MAX 232
#define PY_MIN 32
#define PY_MAX 200
/** @} */

/** @brief Where the shards appear, in order: x, y of the 8x8 pickup box. */
static const u8 shard_pos[GOAL * 2] = {
    152, 112,
    152,  80,
    120,  80,
    120, 144,
    168, 144
};

/** @brief Kit's pickup box (top-left corner). Named globals: the tests read them. */
u8 play_px, play_py;
/** @brief The current shard's box. */
u8 play_sx, play_sy;
/** @brief Shards collected this round. */
u8 play_shards;
/** @brief Frames since the round began. */
u16 play_time;
/** @brief Seconds left, for the HUD. */
u8 play_secs;

static u8 tick;            /* frames since the last second */
static u8 facing;          /* FRAME_DOWN, FRAME_UP or FRAME_RIGHT */
static u8 facing_left;     /* the right frames, mirrored */
static u8 moving;          /* a direction was held this image */

/** @brief The OAM tile of Kit's frame @p f (32x32 frames are 4x4 tiles, rows 16 tiles wide). */
static u16 kit_tile(u16 f) {
    return (u16)(((f & 3) << 2) + ((f >> 2) << 6));
}

/** @brief Draw Kit and the shard; the box sits at Kit's feet. */
static void drawSprites(void) {
    u16 f = facing;
    if (moving) f += (play_time / 6) % 5;
    oamSet(0, play_px - 12, play_py - 20, kit_tile(f), 0, 3, facing_left ? OBJ_FLIPX : 0);
    oamSet(1, play_sx - 4, play_sy - 4, TILE_SHARD + 2 * ((play_time / 4) % 8), 1, 2, 0);
}

/** @brief Refresh the two HUD numbers (clear the field first so a shorter number never smears). */
static void drawHud(void) {
    textPrintAt(9, 1, "  ");
    textSetPos(9, 1);
    textPrintU16(play_shards);
    textPrintAt(23, 1, "   ");
    textSetPos(23, 1);
    textPrintU16(play_secs);
}

/** @brief Close hook: the sprites and the HUD do not outlive the round. */
static void playClose(void) {
    oamClear();
    textClear();
}

/** @brief Put the next shard of the table on the field. */
static void placeShard(void) {
    play_sx = shard_pos[play_shards * 2];
    play_sy = shard_pos[play_shards * 2 + 1];
}

/** @brief One image of play. Flat u8/u16 only: no call through a pointer in the hot loop. */
static void update(u16 held) {
    u8 dx, dy;
    moving = 0;
    if ((held & KEY_LEFT)  && play_px > PX_MIN) { play_px -= PLAYER_SPEED; facing = FRAME_RIGHT; facing_left = 1; moving = 1; }
    if ((held & KEY_RIGHT) && play_px < PX_MAX) { play_px += PLAYER_SPEED; facing = FRAME_RIGHT; facing_left = 0; moving = 1; }
    if ((held & KEY_UP)    && play_py > PY_MIN) { play_py -= PLAYER_SPEED; facing = FRAME_UP;    facing_left = 0; moving = 1; }
    if ((held & KEY_DOWN)  && play_py < PY_MAX) { play_py += PLAYER_SPEED; facing = FRAME_DOWN;  facing_left = 0; moving = 1; }

    play_time++;
    dx = (u8)((play_px > play_sx) ? play_px - play_sx : play_sx - play_px);
    dy = (u8)((play_py > play_sy) ? play_py - play_sy : play_sy - play_py);
    if (dx < 8 && dy < 8) {
        play_shards++;
        SNES_ASSERT(play_shards <= GOAL);
        if (play_shards < GOAL) placeShard();
        drawHud();
    }
    if (++tick >= 60) {
        tick = 0;
        play_secs--;
        drawHud();
    }
    drawSprites();
}

void playRun(void) {
    game_screen = SCREEN_PLAY;

    /* Compose with the display off */
    oamClear();
    oamSetSize(0, OBJ_LARGE);       /* oamClear() reset the size bits: Kit is the large size */
    textClear();
    setMainScreen(LAYER_BG1 | LAYER_BG3 | LAYER_OBJ);
    screenLoad(field_map, VRAM_BG1_MAP, field_map_end - field_map, VRAM_QUEUE_ROW);
    screenSetClose(playClose);      /* what this screen sets up, it undoes */

    play_px = 120;
    play_py = 112;
    play_shards = 0;
    play_time = 0;
    play_secs = TIME_LIMIT / 60;
    tick = 0;
    facing = FRAME_DOWN;
    facing_left = 0;
    moving = 0;
    placeShard();
    textPrintAt(2, 1, "SHARDS");
    textPrintAt(12, 1, "/ 5");
    textPrintAt(18, 1, "TIME");
    drawHud();
    drawSprites();

    screenShow();
    while (play_shards < GOAL && play_time < TIME_LIMIT) {
        screenFrame();
        update(padHeld(0));
    }
    run_won = (play_shards >= GOAL);
    run_time = play_time;
    screenClose();
}
