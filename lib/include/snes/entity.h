/**
 * @file entity.h
 * @brief SNES Entity/Object Management System
 *
 * Simple entity system for managing game objects (players, enemies, bullets, items).
 *
 * ## Overview
 *
 * This module provides:
 * - Pool of reusable entity slots (up to 64 entities)
 * - Position and velocity tracking
 * - Entity types for game logic
 * - Sprite and animation integration
 * - Collision box support
 * - Batch update and draw functions
 *
 * ## Usage Example
 *
 * @code
 * // Define entity types
 * #define ENT_PLAYER  1
 * #define ENT_ENEMY   2
 * #define ENT_BULLET  3
 *
 * // Spawn player at start
 * Entity *player = entitySpawn(ENT_PLAYER, FIX(100), FIX(100));
 * player->width = 16;
 * player->height = 16;
 *
 * // In main loop:
 * entityUpdateAll();   // Update positions, call update callbacks
 * entityDrawAll();     // Update OAM from entity positions
 * oamUpdate();         // Send to hardware
 *
 * // Spawn enemy
 * Entity *enemy = entitySpawn(ENT_ENEMY, FIX(200), FIX(50));
 * if (enemy) {
 *     enemy->vx = FIX(-1);  // Move left
 * }
 *
 * // Check collisions
 * if (entityCollide(player, enemy)) {
 *     player->health--;
 * }
 *
 * // Destroy entity when done
 * entityDestroy(enemy);
 * @endcode
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_ENTITY_H
#define OPENSNES_ENTITY_H

#include <snes/types.h>

/*============================================================================
 * Constants
 *============================================================================*/

/** @brief Maximum number of entities */
#define ENTITY_MAX 16

/** @brief Entity type: unused slot */
#define ENT_NONE 0

/*============================================================================
 * Entity Structure
 *============================================================================*/

/**
 * @brief Entity structure
 *
 * Represents a game object with position, velocity, and properties.
 * Positions and velocities use 8.8 fixed-point format.
 */
typedef struct Entity {
    /* State */
    u8 active;          /**< 1 if entity is active, 0 if free */
    u8 type;            /**< Entity type (game-defined, 0 = unused) */
    u8 flags;           /**< User-defined flags */
    u8 state;           /**< User-defined state */

    /* Position (8.8 fixed-point) */
    s16 x;              /**< X position (fixed-point) */
    s16 y;              /**< Y position (fixed-point) */

    /* Velocity (8.8 fixed-point) */
    s16 vx;             /**< X velocity (fixed-point) */
    s16 vy;             /**< Y velocity (fixed-point) */

    /* Size (for collision) */
    u8 width;           /**< Width in pixels */
    u8 height;          /**< Height in pixels */

    /* Graphics */
    u8 spriteId;        /**< OAM sprite ID (0-127) */
    u8 tile;            /**< Current tile index */
    u8 palette;         /**< Palette number (0-7) */
    u8 priority;        /**< BG priority (0-3) */

    /* Game data */
    u8 health;          /**< Health/HP */
    u8 timer;           /**< General-purpose timer */
} Entity;

/*============================================================================
 * Entity Flags (user can define more)
 *============================================================================*/

#define ENT_FLAG_VISIBLE    0x01    /**< Entity is visible */
#define ENT_FLAG_SOLID      0x02    /**< Entity blocks movement */
#define ENT_FLAG_FLIP_X     0x04    /**< Flip sprite horizontally */
#define ENT_FLAG_FLIP_Y     0x08    /**< Flip sprite vertically */

/*============================================================================
 * Entity Management Functions
 *============================================================================*/

/**
 * @brief Initialize the entity system
 *
 * Clears all entity slots. Call once at game start.
 */
void entityInit(void);

/**
 * @brief Spawn a new entity
 *
 * Finds a free entity slot and initializes it.
 *
 * @param type Entity type (game-defined, non-zero)
 * @param x Initial X position (fixed-point)
 * @param y Initial Y position (fixed-point)
 * @return Pointer to entity, or NULL if no free slots
 *
 * @code
 * Entity *bullet = entitySpawn(ENT_BULLET, player->x, player->y);
 * if (bullet) {
 *     bullet->vx = FIX(8);  // Fast horizontal
 *     bullet->tile = TILE_BULLET;
 * }
 * @endcode
 */
Entity *entitySpawn(u8 type, s16 x, s16 y);

/**
 * @brief Destroy an entity
 *
 * Marks entity as inactive and returns it to the pool.
 *
 * @param e Entity to destroy
 */
void entityDestroy(Entity *e);

/**
 * @brief Get entity by index
 *
 * @param index Entity index (0 to ENTITY_MAX-1)
 * @return Pointer to entity (may be inactive)
 */
Entity *entityGet(u8 index);

/**
 * @brief Find first entity of given type
 *
 * @param type Entity type to find
 * @return Pointer to first matching entity, or NULL if none
 */
Entity *entityFindType(u8 type);

/**
 * @brief Count active entities
 *
 * @return Number of active entities
 */
u8 entityCount(void);

/**
 * @brief Count entities of given type
 *
 * @param type Entity type to count
 * @return Number of active entities of that type
 */
u8 entityCountType(u8 type);

/*============================================================================
 * Entity Update Functions
 *============================================================================*/

/**
 * @brief Update all active entities
 *
 * For each active entity:
 * 1. Applies velocity to position (x += vx, y += vy)
 * 2. Decrements timer if non-zero
 *
 * Call once per frame.
 */
void entityUpdateAll(void);

/**
 * @brief Update entity positions only (no callbacks)
 *
 * Applies velocity to all active entities without calling update callbacks.
 */
void entityMoveAll(void);

/*============================================================================
 * Entity Drawing Functions
 *============================================================================*/

/**
 * @brief Draw all active entities to OAM
 *
 * Updates OAM entries for all visible entities based on their
 * position, tile, palette, and flags.
 *
 * Call after entityUpdateAll(), then call oamUpdate().
 *
 * @note Requires sprite.h functions (oamSet)
 */
void entityDrawAll(void);

/**
 * @brief Hide all entity sprites
 *
 * Sets all entity sprite Y positions to 240 (off-screen).
 */
void entityHideAll(void);

/*============================================================================
 * Entity Collision Functions
 *============================================================================*/

/**
 * @brief Check collision between two entities
 *
 * Uses entity position, width, and height for AABB collision.
 *
 * @param a First entity
 * @param b Second entity
 * @return 1 if colliding, 0 otherwise
 */
u8 entityCollide(Entity *a, Entity *b);

/**
 * @brief Check if entity collides with any entity of given type
 *
 * @param e Entity to check
 * @param type Type to check against
 * @return Pointer to first colliding entity, or NULL
 *
 * @code
 * Entity *hit = entityCollideType(bullet, ENT_ENEMY);
 * if (hit) {
 *     hit->health--;
 *     entityDestroy(bullet);
 * }
 * @endcode
 */
Entity *entityCollideType(Entity *e, u8 type);

/**
 * @brief Check if point is inside entity
 *
 * @param e Entity to check
 * @param px Point X (fixed-point)
 * @param py Point Y (fixed-point)
 * @return 1 if point is inside entity bounds, 0 otherwise
 */
u8 entityContainsPoint(Entity *e, s16 px, s16 py);

/*============================================================================
 * Entity Helper Functions
 *============================================================================*/

/**
 * @brief Set entity position
 *
 * @param e Entity
 * @param x New X position (fixed-point)
 * @param y New Y position (fixed-point)
 */
void entitySetPos(Entity *e, s16 x, s16 y);

/**
 * @brief Set entity velocity
 *
 * @param e Entity
 * @param vx New X velocity (fixed-point)
 * @param vy New Y velocity (fixed-point)
 */
void entitySetVel(Entity *e, s16 vx, s16 vy);

/**
 * @brief Get entity screen X (integer)
 *
 * @param e Entity
 * @return Screen X position (truncated from fixed-point)
 */
s16 entityScreenX(Entity *e);

/**
 * @brief Get entity screen Y (integer)
 *
 * @param e Entity
 * @return Screen Y position (truncated from fixed-point)
 */
s16 entityScreenY(Entity *e);

/**
 * @brief Set entity sprite properties
 *
 * @param e Entity
 * @param spriteId OAM sprite ID
 * @param tile Tile index
 * @param palette Palette number
 */
void entitySetSprite(Entity *e, u8 spriteId, u8 tile, u8 palette);

#endif /* OPENSNES_ENTITY_H */
