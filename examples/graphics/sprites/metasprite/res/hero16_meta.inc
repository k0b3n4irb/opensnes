/**
 * Metasprite data for 16x16 hero sprites.
 * Each frame is a 32x48 character composed of 6 hardware sprites (16x16 each).
 *
 * Tile indices are 8x8-based OAM tile numbers matching the -T transposed VRAM layout.
 * The transposed .pic has 96 tiles in a 16x6 grid (8x3 of 16x16 tiles).
 *
 * Mapping: transposed 16x16 tile at (col, row) in the 8x3 grid
 *          -> OAM 8x8 tile = col*2 + row*32
 *
 * Tile arrangement (transposed linear order -> content):
 *   0=F0(0,0) 1=F0(1,0) 2=F0(0,1) 3=F0(1,1) 4=F0(0,2) 5=F0(1,2)
 *   6=F1(0,0) 7=F1(1,0) ...
 */

#ifndef HERO16_META_H
#define HERO16_META_H

/* Frame 0: transposed tiles 0-5 at grid (0,0)-(5,0) */
static const MetaspriteItem hero16_frame0[] = {
    METASPR_ITEM(0,  0,  0,  OBJ_PAL(0) | OBJ_PRIO(2)),  /* (0,0)->0*2=0 */
    METASPR_ITEM(16, 0,  2,  OBJ_PAL(0) | OBJ_PRIO(2)),  /* (1,0)->1*2=2 */
    METASPR_ITEM(0,  16, 4,  OBJ_PAL(0) | OBJ_PRIO(2)),  /* (2,0)->2*2=4 */
    METASPR_ITEM(16, 16, 6,  OBJ_PAL(0) | OBJ_PRIO(2)),  /* (3,0)->3*2=6 */
    METASPR_ITEM(0,  32, 8,  OBJ_PAL(0) | OBJ_PRIO(2)),  /* (4,0)->4*2=8 */
    METASPR_ITEM(16, 32, 10, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (5,0)->5*2=10 */
    METASPR_TERM
};

/* Frame 1: transposed tiles 6-11 at grid (6,0)-(7,0) then (0,1)-(3,1) */
static const MetaspriteItem hero16_frame1[] = {
    METASPR_ITEM(0,  0,  12, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (6,0)->6*2=12 */
    METASPR_ITEM(16, 0,  14, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (7,0)->7*2=14 */
    METASPR_ITEM(0,  16, 32, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (0,1)->0+32=32 */
    METASPR_ITEM(16, 16, 34, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (1,1)->2+32=34 */
    METASPR_ITEM(0,  32, 36, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (2,1)->4+32=36 */
    METASPR_ITEM(16, 32, 38, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (3,1)->6+32=38 */
    METASPR_TERM
};

/* Frame 2: transposed tiles 12-17 at grid (4,1)-(7,1) then (0,2)-(1,2) */
static const MetaspriteItem hero16_frame2[] = {
    METASPR_ITEM(0,  0,  40, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (4,1)->8+32=40 */
    METASPR_ITEM(16, 0,  42, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (5,1)->10+32=42 */
    METASPR_ITEM(0,  16, 44, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (6,1)->12+32=44 */
    METASPR_ITEM(16, 16, 46, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (7,1)->14+32=46 */
    METASPR_ITEM(0,  32, 64, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (0,2)->0+64=64 */
    METASPR_ITEM(16, 32, 66, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (1,2)->2+64=66 */
    METASPR_TERM
};

/* Frame 3: transposed tiles 18-23 at grid (2,2)-(7,2) */
static const MetaspriteItem hero16_frame3[] = {
    METASPR_ITEM(0,  0,  68, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (2,2)->4+64=68 */
    METASPR_ITEM(16, 0,  70, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (3,2)->6+64=70 */
    METASPR_ITEM(0,  16, 72, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (4,2)->8+64=72 */
    METASPR_ITEM(16, 16, 74, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (5,2)->10+64=74 */
    METASPR_ITEM(0,  32, 76, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (6,2)->12+64=76 */
    METASPR_ITEM(16, 32, 78, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (7,2)->14+64=78 */
    METASPR_TERM
};

#endif
