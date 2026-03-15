#ifndef PIECE_H
#define PIECE_H

#include <snes.h>

/* Piece types (1-7, matching tile colors) */
#define PIECE_I  1
#define PIECE_O  2
#define PIECE_T  3
#define PIECE_S  4
#define PIECE_Z  5
#define PIECE_L  6
#define PIECE_J  7

#define NUM_PIECE_TYPES  7
#define NUM_ROTATIONS    4
#define CELLS_PER_PIECE  4

/* Direct cell accessors — return row/col offset for cell i of (type, rot) */
s8         pieceGetCellRow(u8 type, u8 rot, u8 i);
s8         pieceGetCellCol(u8 type, u8 rot, u8 i);
s8         pieceSpawnRow(u8 type);
s8         pieceSpawnCol(u8 type);
u8         pieceRandom(void);
u8         pieceRotateCW(u8 rot);
u8         pieceRotateCCW(u8 rot);

#endif
