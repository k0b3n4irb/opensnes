/**
 * @file result.c
 * @brief The result screen: the round's outcome, the best time, then back to the title
 * @ingroup examples
 */
#include <snes.h>
#include <snes/text.h>
#include <snes/input.h>
#include <snes/vramqueue.h>
#include <snes/debug.h>
#include "res/logo.inc"
#include "game.h"
#include "screen.h"
#include "save.h"
#include "ui.h"

void resultRun(void) {
    game_screen = SCREEN_RESULT;
    SNES_ASSERT(run_won <= 1);

    /* A win goes to the cartridge before the screen shows it: what is on screen is what is saved */
    run_new_best = run_won ? saveRecord(run_time) : 0;

    oamClear();
    textClear();
    setMainScreen(LAYER_BG2 | LAYER_BG3);
    screenLoad(logo_map, VRAM_BG2_MAP, logo_map_end - logo_map, VRAM_QUEUE_ROW);
    textPrintAt(11, 2, run_won ? "YOU WIN" : "TIME UP");
    textPrintAt(8, 4, "TIME");
    uiPrintTime(14, 4, run_time);
    textPrintAt(8, 6, "BEST");
    uiPrintBest(14, 6);
    if (run_new_best) textPrintAt(10, 8, "NEW BEST!");
    textPrintAt(10, 25, "PRESS START");

    screenShow();
    screenWait(RESULT_WAIT);        /* START, or the title comes back by itself */
    screenClose();
}
