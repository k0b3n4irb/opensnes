/**
 * Metasprite data for 32x32 hero sprites.
 * Each frame is a 64x64 character composed of 4 hardware sprites (32x32 each).
 *
 * Tile indices are 8x8-based OAM tile numbers matching the -T transposed VRAM layout.
 * The transposed .pic has 192 tiles in a 16x12 grid (4x3 of 32x32 tiles).
 *
 * Mapping: transposed 32x32 tile at (col, row) in the 4x3 grid
 *          -> OAM 8x8 tile = col*4 + row*64
 *
 * Tile arrangement (transposed linear order -> content):
 *   0=F0_TL 1=F0_TR 2=F1_TL 3=F1_TR 4=F2_TL 5=F2_TR
 *   6=F0_BL 7=F0_BR 8=F1_BL 9=F1_BR 10=F2_BL 11=F2_BR
 */

#ifndef HERO32_META_H
#define HERO32_META_H

/* Frame 0: TL at (0,0), TR at (1,0), BL at (2,1), BR at (3,1) */
static const MetaspriteItem hero32_frame0[] = {
    METASPR_ITEM(0,  0,  0,  OBJ_PAL(0) | OBJ_PRIO(2)),  /* (0,0)->0*4=0 */
    METASPR_ITEM(32, 0,  4,  OBJ_PAL(0) | OBJ_PRIO(2)),  /* (1,0)->1*4=4 */
    METASPR_ITEM(0,  32, 72, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (2,1)->2*4+64=72 */
    METASPR_ITEM(32, 32, 76, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (3,1)->3*4+64=76 */
    METASPR_TERM
};

/* Frame 1: TL at (2,0), TR at (3,0), BL at (0,2), BR at (1,2) */
static const MetaspriteItem hero32_frame1[] = {
    METASPR_ITEM(0,  0,  8,   OBJ_PAL(0) | OBJ_PRIO(2)),  /* (2,0)->2*4=8 */
    METASPR_ITEM(32, 0,  12,  OBJ_PAL(0) | OBJ_PRIO(2)),  /* (3,0)->3*4=12 */
    METASPR_ITEM(0,  32, 128, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (0,2)->0+128=128 */
    METASPR_ITEM(32, 32, 132, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (1,2)->4+128=132 */
    METASPR_TERM
};

/* Frame 2: TL at (0,1), TR at (1,1), BL at (2,2), BR at (3,2) */
static const MetaspriteItem hero32_frame2[] = {
    METASPR_ITEM(0,  0,  64,  OBJ_PAL(0) | OBJ_PRIO(2)),  /* (0,1)->0+64=64 */
    METASPR_ITEM(32, 0,  68,  OBJ_PAL(0) | OBJ_PRIO(2)),  /* (1,1)->4+64=68 */
    METASPR_ITEM(0,  32, 136, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (2,2)->8+128=136 */
    METASPR_ITEM(32, 32, 140, OBJ_PAL(0) | OBJ_PRIO(2)),  /* (3,2)->12+128=140 */
    METASPR_TERM
};

#endif
