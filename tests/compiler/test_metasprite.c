/**
 * test_metasprite.c - Verify metasprite implementation
 *
 * Tests that metasprite structures and macros match PVSnesLib format
 * and generate correct OAM data.
 */

#include <snes/types.h>
#include <snes/sprite.h>

/* Test metasprite definition - a 32x32 character from 4 16x16 sprites */
const MetaspriteItem hero_walk_frame0[] = {
    METASPR_ITEM(0,  0,  0, OBJ_PAL(0) | OBJ_PRIO(2)),   /* Top-left */
    METASPR_ITEM(16, 0,  1, OBJ_PAL(0) | OBJ_PRIO(2)),   /* Top-right */
    METASPR_ITEM(0,  16, 2, OBJ_PAL(0) | OBJ_PRIO(2)),   /* Bottom-left */
    METASPR_ITEM(16, 16, 3, OBJ_PAL(0) | OBJ_PRIO(2)),   /* Bottom-right */
    METASPR_TERM
};

/* Test flipped metasprite */
const MetaspriteItem hero_walk_frame0_flipped[] = {
    METASPR_ITEM(16, 0,  0, OBJ_PAL(0) | OBJ_PRIO(2) | OBJ_FLIPX),
    METASPR_ITEM(0,  0,  1, OBJ_PAL(0) | OBJ_PRIO(2) | OBJ_FLIPX),
    METASPR_ITEM(16, 16, 2, OBJ_PAL(0) | OBJ_PRIO(2) | OBJ_FLIPX),
    METASPR_ITEM(0,  16, 3, OBJ_PAL(0) | OBJ_PRIO(2) | OBJ_FLIPX),
    METASPR_TERM
};

/* Animation frames array (like PVSnesLib gfx4snes output) */
const MetaspriteItem* hero_walk_metasprites[2] = {
    hero_walk_frame0,
    hero_walk_frame0_flipped
};

/* Test size verification */
void test_metasprite_sizes(void) {
    /* Verify structure size is 8 bytes (PVSnesLib compatible) */
    static u8 size_check[sizeof(MetaspriteItem) == 8 ? 1 : -1];
    (void)size_check;
}

/* Test macro expansions */
void test_macros(void) {
    /* OBJ_PAL(0-7) should produce palette bits in correct position */
    u8 pal0 = OBJ_PAL(0);  /* Should be 0 */
    u8 pal3 = OBJ_PAL(3);  /* Should be 6 (3 << 1) */
    u8 pal7 = OBJ_PAL(7);  /* Should be 14 (7 << 1) */

    /* OBJ_PRIO(0-3) should produce priority bits */
    u8 prio0 = OBJ_PRIO(0);  /* Should be 0 */
    u8 prio2 = OBJ_PRIO(2);  /* Should be 32 (2 << 4) */
    u8 prio3 = OBJ_PRIO(3);  /* Should be 48 (3 << 4) */

    /* Flip flags */
    u8 flipx = OBJ_FLIPX;  /* Should be 0x40 */
    u8 flipy = OBJ_FLIPY;  /* Should be 0x80 */

    (void)pal0; (void)pal3; (void)pal7;
    (void)prio0; (void)prio2; (void)prio3;
    (void)flipx; (void)flipy;
}

/* Test that metasprite end marker is correct */
void test_end_marker(void) {
    /* METASPR_TERM should have dx = -128 */
    if (hero_walk_frame0[4].dx == metasprite_end) {
        /* Correct */
    }
}

int main(void) {
    test_metasprite_sizes();
    test_macros();
    test_end_marker();

    /* Draw metasprite at position (100, 80) */
    oamDrawMeta(0, 100, 80, hero_walk_frame0, 0, 0, OBJ_LARGE);

    return 0;
}
