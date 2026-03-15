#include "board.h"
#include "piece.h"

/* 24 rows x 10 columns. Rows 0-3 are hidden spawn area. */
static u8 board[BOARD_ROWS][BOARD_W];

void boardClear(void) {
    u8 r, c;
    for (r = 0; r < BOARD_ROWS; r++) {
        for (c = 0; c < BOARD_W; c++) {
            board[r][c] = 0;
        }
    }
}

u8 boardGetCell(u8 row, u8 col) {
    if (row >= BOARD_ROWS || col >= BOARD_W) return 1;
    return board[row][col];
}

u8 boardCheckCollision(u8 type, u8 rot, s8 row, s8 col) {
    u8 i;
    s8 r, c;

    for (i = 0; i < CELLS_PER_PIECE; i++) {
        r = row + pieceGetCellRow(type, rot, i);
        c = col + pieceGetCellCol(type, rot, i);

        /* Out of bounds */
        if (c < 0 || c >= BOARD_W) return 1;
        if (r < 0) return 1;
        if (r >= BOARD_ROWS) return 1;

        /* Cell occupied */
        if (board[r][c] != 0) return 1;
    }

    return 0;
}

void boardLockPiece(u8 type, u8 rot, s8 row, s8 col) {
    u8 i;
    s8 r, c;

    for (i = 0; i < CELLS_PER_PIECE; i++) {
        r = row + pieceGetCellRow(type, rot, i);
        c = col + pieceGetCellCol(type, rot, i);

        if (r >= 0 && r < BOARD_ROWS && c >= 0 && c < BOARD_W) {
            board[r][c] = type;
        }
    }
}

u8 boardFindFullLines(LineClearResult *result) {
    u8 r, c, full;
    u8 cnt;

    cnt = 0;

    /* Only scan visible rows — hidden spawn rows (0-3) can never be full */
    for (r = VISIBLE_TOP; r < BOARD_ROWS; r++) {
        full = 1;
        for (c = 0; c < BOARD_W; c++) {
            if (boardGetCell(r, c) == 0) {
                full = 0;
                break;
            }
        }
        if (full && cnt < 4) {
            result->rows[cnt] = r;
            cnt++;
        }
    }

    result->count = cnt;
    return cnt;
}

void boardRemoveLines(LineClearResult *result) {
    u8 i, r, c, line_row;
    s8 dst;

    for (i = 0; i < result->count; i++) {
        line_row = result->rows[i];

        /* Shift all rows above down by one */
        for (dst = line_row; dst > 0; dst--) {
            for (c = 0; c < BOARD_W; c++) {
                board[dst][c] = board[dst - 1][c];
            }
        }
        /* Clear top row */
        for (c = 0; c < BOARD_W; c++) {
            board[0][c] = 0;
        }

        /* Adjust remaining line indices (they shifted down) */
        {
            u8 j;
            for (j = i + 1; j < result->count; j++) {
                if (result->rows[j] < line_row) {
                    result->rows[j]++;
                }
            }
        }
    }
}
