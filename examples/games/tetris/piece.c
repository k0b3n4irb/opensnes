#include "piece.h"

/*
 * Combined tetromino shape data: 7 types x 4 rotations x 8 bytes
 * Layout: [type-1][rot][0..3] = row offsets, [type-1][rot][4..7] = col offsets
 * Each PieceShape = 8 bytes (4 row + 4 col), each rotation = 8 bytes.
 * Index: (type-1)*32 + rot*8 + cell_index
 *
 * Type IDs match tile color indices (1-7).
 * Nintendo Rotation System (NRS).
 */
static s8 all_shapes[] = {
    /* === PIECE_I (type 1) === */
    /* rot 0: horizontal ████ */
     0, 0, 0, 0,   0, 1, 2, 3,
    /* rot 1: vertical */
     0, 1, 2, 3,   1, 1, 1, 1,
    /* rot 2: horizontal (offset) */
     1, 1, 1, 1,   0, 1, 2, 3,
    /* rot 3: vertical (offset) */
     0, 1, 2, 3,   2, 2, 2, 2,

    /* === PIECE_O (type 2) === all rotations identical */
     0, 0, 1, 1,   0, 1, 0, 1,
     0, 0, 1, 1,   0, 1, 0, 1,
     0, 0, 1, 1,   0, 1, 0, 1,
     0, 0, 1, 1,   0, 1, 0, 1,

    /* === PIECE_T (type 3) === */
    /* rot 0: .#. / ### */
     0, 1, 1, 1,   1, 0, 1, 2,
    /* rot 1: #. / ## / #. */
     0, 1, 1, 2,   0, 0, 1, 0,
    /* rot 2: ### / .#. */
     0, 0, 0, 1,   0, 1, 2, 1,
    /* rot 3: .# / ## / .# */
     0, 1, 1, 2,   1, 0, 1, 1,

    /* === PIECE_S (type 4) === */
    /* rot 0: .## / ##. */
     0, 0, 1, 1,   1, 2, 0, 1,
    /* rot 1 */
     0, 1, 1, 2,   0, 0, 1, 1,
    /* rot 2 (same as 0) */
     0, 0, 1, 1,   1, 2, 0, 1,
    /* rot 3 (same as 1) */
     0, 1, 1, 2,   0, 0, 1, 1,

    /* === PIECE_Z (type 5) === */
    /* rot 0: ##. / .## */
     0, 0, 1, 1,   0, 1, 1, 2,
    /* rot 1 */
     0, 1, 1, 2,   1, 0, 1, 0,
    /* rot 2 (same as 0) */
     0, 0, 1, 1,   0, 1, 1, 2,
    /* rot 3 (same as 1) */
     0, 1, 1, 2,   1, 0, 1, 0,

    /* === PIECE_L (type 6) === */
    /* rot 0: ..# / ### */
     0, 1, 1, 1,   2, 0, 1, 2,
    /* rot 1: #. / #. / ## */
     0, 1, 2, 2,   0, 0, 0, 1,
    /* rot 2: ### / #.. */
     0, 0, 0, 1,   0, 1, 2, 0,
    /* rot 3: ## / .# / .# */
     0, 0, 1, 2,   0, 1, 1, 1,

    /* === PIECE_J (type 7) === */
    /* rot 0: #.. / ### */
     0, 1, 1, 1,   0, 0, 1, 2,
    /* rot 1: ## / #. / #. */
     0, 0, 1, 2,   0, 1, 0, 0,
    /* rot 2: ### / ..# */
     0, 0, 0, 1,   0, 1, 2, 2,
    /* rot 3: .# / .# / ## */
     0, 1, 2, 2,   1, 1, 0, 1,
};

s8 pieceGetCellRow(u8 type, u8 rot, u8 i) {
    u16 idx;
    idx = (u16)(type - 1) * 32 + (u16)rot * 8 + (u16)i;
    return all_shapes[idx];
}

s8 pieceGetCellCol(u8 type, u8 rot, u8 i) {
    u16 idx;
    idx = (u16)(type - 1) * 32 + (u16)rot * 8 + 4 + (u16)i;
    return all_shapes[idx];
}

s8 pieceSpawnRow(u8 type) {
    (void)type;
    return 3;  /* spawn in hidden area, bottom cells appear at row 4 (VISIBLE_TOP) */
}

s8 pieceSpawnCol(u8 type) {
    (void)type;
    return 3;
}

u8 pieceRandom(void) {
    u16 r = rand();
    u8 val = (u8)(r % 7);
    return val + 1;
}

u8 pieceRotateCW(u8 rot) {
    u8 next = rot + 1;
    if (next >= 4) next = 0;
    return next;
}

u8 pieceRotateCCW(u8 rot) {
    if (rot == 0) return 3;
    return rot - 1;
}
