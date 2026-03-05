/**
 * Metasprite data for 8x8 hero sprites.
 * Each frame is a 16x16 character composed of 4 OBJ_SMALL (8x8) hardware sprites.
 * Tile indices reference 8x8 tiles in the transposed .pic layout (16 tiles in one row).
 */

#ifndef HERO8_META_H
#define HERO8_META_H

static const MetaspriteItem hero8_frame0[] = {
    METASPR_ITEM(0, 0, 0, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_ITEM(8, 0, 1, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_ITEM(0, 8, 8, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_ITEM(8, 8, 9, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_TERM
};

static const MetaspriteItem hero8_frame1[] = {
    METASPR_ITEM(0, 0, 2, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_ITEM(8, 0, 3, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_ITEM(0, 8, 10, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_ITEM(8, 8, 11, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_TERM
};

static const MetaspriteItem hero8_frame2[] = {
    METASPR_ITEM(0, 0, 4, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_ITEM(8, 0, 5, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_ITEM(0, 8, 12, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_ITEM(8, 8, 13, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_TERM
};

static const MetaspriteItem hero8_frame3[] = {
    METASPR_ITEM(0, 0, 6, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_ITEM(8, 0, 7, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_ITEM(0, 8, 14, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_ITEM(8, 8, 15, OBJ_PAL(0) | OBJ_PRIO(2)),
    METASPR_TERM
};

#endif
