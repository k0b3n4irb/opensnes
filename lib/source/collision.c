/**
 * @file collision.c
 * @brief OpenSNES Collision Detection Implementation
 *
 * AABB and tile-based collision detection.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>
#include <snes/collision.h>

/*============================================================================
 * Rectangle Collision Functions
 *============================================================================*/

u8 collideRect(Rect *a, Rect *b) {
    s16 aRight, aBottom;
    s16 bRight, bBottom;

    /* Calculate right and bottom edges */
    aRight = a->x + a->width;
    aBottom = a->y + a->height;
    bRight = b->x + b->width;
    bBottom = b->y + b->height;

    /* Check for no overlap (any gap means no collision) */
    if (a->x >= bRight) return 0;   /* A is to the right of B */
    if (aRight <= b->x) return 0;   /* A is to the left of B */
    if (a->y >= bBottom) return 0;  /* A is below B */
    if (aBottom <= b->y) return 0;  /* A is above B */

    /* Rectangles overlap */
    return 1;
}

u8 collidePoint(s16 x, s16 y, Rect *r) {
    s16 rRight, rBottom;

    rRight = r->x + r->width;
    rBottom = r->y + r->height;

    if (x < r->x) return 0;
    if (x >= rRight) return 0;
    if (y < r->y) return 0;
    if (y >= rBottom) return 0;

    return 1;
}

u8 collideRectEx(Rect *a, Rect *b, s16 *overlapX, s16 *overlapY) {
    s16 aRight, aBottom;
    s16 bRight, bBottom;
    s16 dx1, dx2, dy1, dy2;

    /* Calculate edges */
    aRight = a->x + a->width;
    aBottom = a->y + a->height;
    bRight = b->x + b->width;
    bBottom = b->y + b->height;

    /* Check for no overlap */
    if (a->x >= bRight || aRight <= b->x ||
        a->y >= bBottom || aBottom <= b->y) {
        *overlapX = 0;
        *overlapY = 0;
        return 0;
    }

    /* Calculate overlap amounts */
    /* How far A would need to move to exit B on each side */
    dx1 = bRight - a->x;    /* Distance to exit left */
    dx2 = aRight - b->x;    /* Distance to exit right */
    dy1 = bBottom - a->y;   /* Distance to exit up */
    dy2 = aBottom - b->y;   /* Distance to exit down */

    /* Return minimum overlap direction */
    if (dx1 < dx2) {
        *overlapX = dx1;    /* Push right */
    } else {
        *overlapX = 0 - dx2;  /* Push left (negative) */
    }

    if (dy1 < dy2) {
        *overlapY = dy1;    /* Push down */
    } else {
        *overlapY = 0 - dy2;  /* Push up (negative) */
    }

    return 1;
}

/*============================================================================
 * Tile-Based Collision Functions
 *============================================================================*/

u8 collideTile(s16 px, s16 py, u8 *tilemap, u16 mapWidth) {
    u16 tileX, tileY;
    u16 offset;

    /* Negative coordinates = out of bounds */
    if (px < 0 || py < 0) return 0;

    /* Convert pixel coords to tile coords (8x8 tiles) */
    tileX = px >> 3;  /* Divide by 8 */
    tileY = py >> 3;

    /* Calculate offset in tilemap */
    /* offset = tileY * mapWidth + tileX */
    /* Use repeated addition to avoid multiplication */
    offset = 0;
    while (tileY > 0) {
        offset = offset + mapWidth;
        tileY = tileY - 1;
    }
    offset = offset + tileX;

    /* Return tile value */
    return tilemap[offset];
}

u8 collideTileEx(s16 px, s16 py, u8 *tilemap, u16 mapWidth, u8 tileSize) {
    u16 tileX, tileY;
    u16 offset;
    u8 shift;

    if (px < 0 || py < 0) return 0;

    /* Determine shift amount based on tile size */
    /* 8=3, 16=4, 32=5 */
    if (tileSize == 8) {
        shift = 3;
    } else if (tileSize == 16) {
        shift = 4;
    } else if (tileSize == 32) {
        shift = 5;
    } else {
        shift = 3;  /* Default to 8x8 */
    }

    tileX = px >> shift;
    tileY = py >> shift;

    /* Calculate offset */
    offset = 0;
    while (tileY > 0) {
        offset = offset + mapWidth;
        tileY = tileY - 1;
    }
    offset = offset + tileX;

    return tilemap[offset];
}

u8 collideRectTile(Rect *r, u8 *tilemap, u16 mapWidth) {
    s16 right, bottom;

    right = r->x + r->width - 1;
    bottom = r->y + r->height - 1;

    /* Check all four corners */
    if (collideTile(r->x, r->y, tilemap, mapWidth)) return 1;      /* Top-left */
    if (collideTile(right, r->y, tilemap, mapWidth)) return 1;     /* Top-right */
    if (collideTile(r->x, bottom, tilemap, mapWidth)) return 1;    /* Bottom-left */
    if (collideTile(right, bottom, tilemap, mapWidth)) return 1;   /* Bottom-right */

    return 0;
}

/*============================================================================
 * Helper Functions
 *============================================================================*/

void rectInit(Rect *r, s16 x, s16 y, u16 w, u16 h) {
    r->x = x;
    r->y = y;
    r->width = w;
    r->height = h;
}

void rectSetPos(Rect *r, s16 x, s16 y) {
    r->x = x;
    r->y = y;
}

void rectGetCenter(Rect *r, s16 *cx, s16 *cy) {
    *cx = r->x + (r->width >> 1);
    *cy = r->y + (r->height >> 1);
}

u8 rectContains(Rect *inner, Rect *outer) {
    s16 innerRight, innerBottom;
    s16 outerRight, outerBottom;

    innerRight = inner->x + inner->width;
    innerBottom = inner->y + inner->height;
    outerRight = outer->x + outer->width;
    outerBottom = outer->y + outer->height;

    if (inner->x < outer->x) return 0;
    if (inner->y < outer->y) return 0;
    if (innerRight > outerRight) return 0;
    if (innerBottom > outerBottom) return 0;

    return 1;
}
