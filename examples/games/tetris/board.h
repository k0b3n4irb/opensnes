#ifndef TETRIS_BOARD_H
#define TETRIS_BOARD_H

#include <snes.h>

#define BOARD_W   10
#define BOARD_ROWS 24  /* full board: rows 0-23 (0-3 hidden spawn, 4-23 visible) */
#define VISIBLE_TOP 4  /* first visible row (standard 20 visible rows) */

typedef struct {
    u8 count;
    u8 rows[4];
} LineClearResult;

void boardClear(void);
u8   boardCheckCollision(u8 type, u8 rot, s8 row, s8 col);
void boardLockPiece(u8 type, u8 rot, s8 row, s8 col);
u8   boardFindFullLines(LineClearResult *result);
void boardRemoveLines(LineClearResult *result);
u8   boardGetCell(u8 row, u8 col);

#endif /* TETRIS_BOARD_H */
