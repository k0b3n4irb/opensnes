/**
 * @file main.c
 * @brief shard_dash: a small game in several files, with a test for each screen
 * @ingroup examples
 *
 * The other examples are one file because they teach one thing. This one
 * shows how a game is put together so that it stays readable and testable:
 *
 * | File       | Role |
 * |------------|------|
 * | main.c     | boot, the pump, and the sequence: title, round, result, again |
 * | screen.c/h | the screen layer: VBlank, queue flush, fades, pad edges, pump |
 * | title.c    | the title screen, one function |
 * | play.c     | the round, one function |
 * | result.c   | the result screen, one function |
 * | save.c/h   | the save: a versioned, checksummed record, defaults when it is off |
 * | ui.c/h     | text helpers the screens share |
 *
 * The game: grab five shards in thirty seconds. A win is saved if it is the
 * best time; the result screen shows it and the title returns.
 *
 * Design rules shown here (each is commented where it is applied):
 * - nothing freezes the pump: fades use fadeStart()/fadeStep(), never fadeIn()/fadeOut();
 * - a screen is a function that sets up, shows, loops and closes, and undoes what it set up in a close hook;
 * - VRAM uploads go through the queue, in pieces that keep to a budget the game chose;
 * - the save has a magic, a version and a checksum; absent or corrupt means defaults;
 * - state lives in named globals, so a test can read it by name;
 * - a key held over a screen change is not a press on the next screen.
 *
 * @par Controls
 * - Title / result: START (either pad)
 * - Round: D-pad
 *
 * @par SNES Concepts
 * - A game as functions in files, not one switch in one loop
 * - The VRAM queue and its VBlank budget
 * - Non-blocking fades, battery-backed SRAM with a checked record
 * - `luna test` manifests that play each screen, the real fade and a save round trip
 *
 * @par What to Observe
 * Title with the best time (`--` at first), a fade to the round, five shards
 * in a fixed order, then the result and the title again. Win once, power
 * cycle, and the best time is still there.
 *
 * @par Modules Used
 * console, dma, background, sprite, text, input, vramqueue, debug, sram
 *
 * @see examples/basics/game_skeleton, examples/memory/save_game, examples/transitions/fading
 */

#include <snes.h>
#include <snes/text.h>
#include <snes/sprite.h>
#include "res/field.inc"
#include "res/logo.inc"
#include "res/cast.inc"
/* generated: field_, logo_ tiles/_pal/_map, cast_tiles/_pal (each with _end) */
#include "game.h"
#include "screen.h"
#include "save.h"

u8 game_screen;
u16 pump_frames;
u8 run_won;
u16 run_time;
u8 run_new_best;

/** @brief The pump: what the game does on every image, in every screen, fades and loads included. */
static void gamePump(void) {
    pump_frames++;
}

/** @brief Load what every screen shares, once, with the display off. The maps come per screen. */
static void boot(void) {
    consoleInit();
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);

    bgInitTileSet(0, field_tiles, field_pal, 0, field_tiles_end - field_tiles,
                  field_pal_end - field_pal, BG_16COLORS, VRAM_BG1_GFX);
    bgInitTileSet(1, logo_tiles, logo_pal, 1, logo_tiles_end - logo_tiles,
                  logo_pal_end - logo_pal, BG_16COLORS, VRAM_BG2_GFX);
    bgSetMapPtr(0, VRAM_BG1_MAP, SC_32x32);
    bgSetMapPtr(1, VRAM_BG2_MAP, SC_32x32);
    setColor(0, RGB(2, 3, 5));      /* the backdrop: bgInitTileSet loaded the square's colour 0 there */

    textLoadFont(VRAM_BG3_FONT);
    textInit(VRAM_BG3_MAP, 0, 7);
    text_config.priority = 1;
    setColor(29, RGB(31, 31, 31));
    bgSetGfxPtr(2, VRAM_BG3_FONT);
    bgSetMapPtr(2, VRAM_BG3_MAP, SC_32x32);

    oamInitGfxSet(cast_tiles, cast_tiles_end - cast_tiles,
                  cast_pal, cast_pal_end - cast_pal,
                  0, SPR_VRAM, OBJ_SIZE16_L32);
    oamClear();

    screenSetPump(gamePump);
    screenFadeSkip(0);              /* the real fade runs; a test may poke screen_fade_skip */
    saveLoad();
}

int main(void) {
    boot();
    while (1) {
        titleRun();
        playRun();
        resultRun();
    }
    return 0;
}
