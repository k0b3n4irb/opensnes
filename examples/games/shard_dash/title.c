/**
 * @file title.c
 * @brief The title screen: the logo, the rules, the best time
 * @ingroup examples
 */
#include <snes.h>
#include <snes/text.h>
#include <snes/input.h>
#include <snes/vramqueue.h>
#include "res/logo.inc"
#include "game.h"
#include "screen.h"
#include "save.h"
#include "ui.h"

void titleRun(void) {
    game_screen = SCREEN_TITLE;

    /* Compose with the display off: layers, then the map through the queue, then text */
    oamClear();
    textClear();
    setMainScreen(LAYER_BG2 | LAYER_BG3);
    screenLoad(logo_map, VRAM_BG2_MAP, logo_map_end - logo_map, VRAM_QUEUE_ROW);
    textPrintAt(11, 2, "SHARD DASH");
    textPrintAt(7, 4, "GRAB 5 SHARDS IN 30 S");
    textPrintAt(9, 23, "BEST");
    uiPrintBest(14, 23);
    textPrintAt(10, 25, "PRESS START");

    screenShow();
    do {
        screenFrame();
    } while (!screenPressed(KEY_START, 1));     /* either pad starts; a key held from before never does */
    screenClose();
}
