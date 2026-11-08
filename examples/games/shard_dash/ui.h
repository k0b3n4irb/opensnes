/**
 * @file ui.h
 * @brief Text helpers shared by the screens
 * @ingroup examples
 */
#ifndef SHARD_DASH_UI_H
#define SHARD_DASH_UI_H

#include <snes.h>

/** @brief Print a time as seconds and tenths ("12.3") at a text cell; blanks first. */
void uiPrintTime(u8 x, u8 y, u16 frames);
/** @brief Print the saved best time at a cell, or "--" when there is none. */
void uiPrintBest(u8 x, u8 y);

#endif
