/**
 * @file collision.h
 * @brief SNES Collision Detection
 *
 * Simple collision detection routines for game objects.
 *
 * ## Overview
 *
 * This module provides:
 * - Rectangle vs rectangle (AABB) collision
 * - Point vs rectangle collision
 * - Tile-based collision for platformers
 * - Bounding box helpers
 *
 * ## Usage Example
 *
 * @code
 * // Define collision boxes for player and enemy
 * Rect playerBox = { player_x, player_y, 16, 16 };
 * Rect enemyBox = { enemy_x, enemy_y, 16, 16 };
 *
 * // Check collision
 * if (collideRect(&playerBox, &enemyBox)) {
 *     // Handle collision
 *     player_hit();
 * }
 *
 * // Tile-based collision (for platformers)
 * if (collideTile(player_x + 8, player_y + 16, tilemap, 32)) {
 *     // Player is standing on solid ground
 *     on_ground = 1;
 * }
 * @endcode
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_COLLISION_H
#define OPENSNES_COLLISION_H

#include <snes/types.h>

/*============================================================================
 * Types
 *============================================================================*/

/**
 * @brief Axis-aligned bounding box (rectangle)
 *
 * Represents a collision rectangle in screen coordinates.
 * Origin is top-left corner.
 */
typedef struct {
    s16 x;          /**< Left edge X coordinate */
    s16 y;          /**< Top edge Y coordinate */
    u16 width;      /**< Width in pixels */
    u16 height;     /**< Height in pixels */
} Rect;

/*============================================================================
 * Rectangle Collision Functions
 *============================================================================*/

/**
 * @brief Check rectangle vs rectangle collision (AABB)
 *
 * Tests if two axis-aligned bounding boxes overlap.
 *
 * @param a First rectangle
 * @param b Second rectangle
 * @return 1 if rectangles overlap, 0 otherwise
 *
 * @code
 * Rect player = { px, py, 16, 16 };
 * Rect enemy = { ex, ey, 16, 16 };
 * if (collideRect(&player, &enemy)) {
 *     // Collision detected
 * }
 * @endcode
 */
u8 collideRect(Rect *a, Rect *b);

/**
 * @brief Check point vs rectangle collision
 *
 * Tests if a point is inside a rectangle.
 *
 * @param x Point X coordinate
 * @param y Point Y coordinate
 * @param r Rectangle to test against
 * @return 1 if point is inside rectangle, 0 otherwise
 *
 * @code
 * // Check if bullet hits target
 * if (collidePoint(bullet_x, bullet_y, &target)) {
 *     target_hit();
 * }
 * @endcode
 */
u8 collidePoint(s16 x, s16 y, Rect *r);

/**
 * @brief Check if two rectangles overlap and return overlap amount
 *
 * Tests collision and returns the overlap distance for each axis.
 * Useful for collision response (pushing objects apart).
 *
 * @param a First rectangle
 * @param b Second rectangle
 * @param overlapX Pointer to store X overlap (negative = left, positive = right)
 * @param overlapY Pointer to store Y overlap (negative = up, positive = down)
 * @return 1 if rectangles overlap, 0 otherwise
 *
 * @code
 * s16 dx, dy;
 * if (collideRectEx(&player, &wall, &dx, &dy)) {
 *     // Push player out of wall
 *     player_x = player_x - dx;
 *     player_y = player_y - dy;
 * }
 * @endcode
 */
u8 collideRectEx(Rect *a, Rect *b, s16 *overlapX, s16 *overlapY);

/*============================================================================
 * Tile-Based Collision Functions
 *============================================================================*/

/**
 * @brief Check collision with tile at pixel coordinates
 *
 * Looks up the tile at a given pixel position and returns whether
 * it's solid. Tiles are assumed to be 8x8 pixels.
 *
 * @param px Pixel X coordinate
 * @param py Pixel Y coordinate
 * @param tilemap Pointer to collision tilemap (1 byte per tile, 0=empty, nonzero=solid)
 * @param mapWidth Width of tilemap in tiles
 * @return Tile value at position (0 if empty/out of bounds)
 *
 * @code
 * // Check if player would hit ground
 * if (collideTile(player_x + 8, player_y + 16, collision_map, 32)) {
 *     player_vy = 0;  // Stop falling
 *     on_ground = 1;
 * }
 * @endcode
 */
u8 collideTile(s16 px, s16 py, u8 *tilemap, u16 mapWidth);

/**
 * @brief Check collision with tile using custom tile size
 *
 * Like collideTile() but allows custom tile sizes (8, 16, etc).
 *
 * @param px Pixel X coordinate
 * @param py Pixel Y coordinate
 * @param tilemap Pointer to collision tilemap
 * @param mapWidth Width of tilemap in tiles
 * @param tileSize Size of tiles in pixels (must be power of 2: 8, 16, 32)
 * @return Tile value at position (0 if empty/out of bounds)
 */
u8 collideTileEx(s16 px, s16 py, u8 *tilemap, u16 mapWidth, u8 tileSize);

/**
 * @brief Check collision between rectangle and tilemap
 *
 * Tests if any corner of the rectangle touches a solid tile.
 * Useful for checking if a moving object would collide with the world.
 *
 * @param r Rectangle to test
 * @param tilemap Pointer to collision tilemap
 * @param mapWidth Width of tilemap in tiles
 * @return 1 if any corner touches solid tile, 0 otherwise
 *
 * @code
 * Rect playerBox = { player_x, player_y, 16, 16 };
 * if (collideRectTile(&playerBox, collision_map, 32)) {
 *     // Player hit solid tiles
 * }
 * @endcode
 */
u8 collideRectTile(Rect *r, u8 *tilemap, u16 mapWidth);

/*============================================================================
 * Helper Functions
 *============================================================================*/

/**
 * @brief Initialize a rectangle
 *
 * @param r Rectangle to initialize
 * @param x Left edge X
 * @param y Top edge Y
 * @param w Width
 * @param h Height
 */
void rectInit(Rect *r, s16 x, s16 y, u16 w, u16 h);

/**
 * @brief Set rectangle position
 *
 * @param r Rectangle to modify
 * @param x New X position
 * @param y New Y position
 */
void rectSetPos(Rect *r, s16 x, s16 y);

/**
 * @brief Get rectangle center point
 *
 * @param r Rectangle
 * @param cx Pointer to store center X
 * @param cy Pointer to store center Y
 */
void rectGetCenter(Rect *r, s16 *cx, s16 *cy);

/**
 * @brief Check if rectangle is completely inside another
 *
 * @param inner Inner rectangle (potentially contained)
 * @param outer Outer rectangle (container)
 * @return 1 if inner is completely inside outer, 0 otherwise
 */
u8 rectContains(Rect *inner, Rect *outer);

#endif /* OPENSNES_COLLISION_H */
