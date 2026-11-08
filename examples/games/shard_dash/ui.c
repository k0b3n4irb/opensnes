/**
 * @file ui.c
 * @brief Text helpers shared by the screens
 * @ingroup examples
 */
#include <snes.h>
#include <snes/text.h>
#include "ui.h"
#include "save.h"

void uiPrintTime(u8 x, u8 y, u16 frames) {
    u16 tenths = frames / 6;                /* 60 frames a second */
    textPrintAt(x, y, "      ");             /* clear first so a shorter number never smears */
    textSetPos(x, y);
    textPrintU16(tenths / 10);
    textPrint(".");
    textPrintU16(tenths % 10);
}

void uiPrintBest(u8 x, u8 y) {
    if (save_best == SAVE_NONE) {
        textPrintAt(x, y, "--    ");
    } else {
        uiPrintTime(x, y, save_best);
    }
}
