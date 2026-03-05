/**
 * @file object.h
 * @brief Game object engine with physics and collision
 *
 * Provides managed game objects with:
 * - Linked-list management (up to 80 objects, 64 types)
 * - Type-based function dispatch (init, update, refresh callbacks)
 * - Map collision detection (with optional slope support)
 * - Fixed-point position and velocity
 * - Object-to-object collision detection
 *
 * ## Workspace Pattern
 *
 * Because the object buffer resides in Bank $7E (not directly accessible from C),
 * a workspace pattern is used. Before each C callback, the engine copies the
 * current object's data to `objWorkspace` (Bank $00). After the callback returns,
 * changes are copied back.
 *
 * @code
 * #include <snes.h>
 * #include <snes/object.h>
 *
 * void enemy_update(u16 idx) {
 *     // objWorkspace is pre-populated with current object data
 *     if (objWorkspace.action == ACT_WALK) {
 *         objWorkspace.xvel = 0x0100;
 *     }
 *     // Call engine functions (they sync workspace automatically)
 *     objCollidMap(idx);
 *     objUpdateXY(idx);
 *     // Read updated position from workspace
 *     u16 screenx = objWorkspace.xpos[1] | (objWorkspace.xpos[2] << 8);
 * }
 * @endcode
 *
 * ## Attribution
 *
 * Based on: PVSnesLib object engine by Alekmaul
 * Slope management: Nub1604
 * License: zlib (compatible with MIT)
 */

#ifndef OPENSNES_OBJECT_H
#define OPENSNES_OBJECT_H

#include <snes/types.h>

/*============================================================================
 * Constants
 *============================================================================*/

/** @brief Maximum number of objects in the game */
#define OB_MAX              80

/** @brief Maximum number of object types */
#define OB_TYPE_MAX         64

/** @brief Object size in bytes */
#define OB_SIZE             64

/*============================================================================
 * Object Structure (64 bytes, packed)
 *============================================================================*/

/**
 * @brief Game object data structure
 *
 * Each object is 64 bytes. Fields are accessed through the objWorkspace
 * variable during C callbacks.
 */
typedef struct {
    u16 prev;           /**<  0: Previous object in linked list */
    u16 next;           /**<  2: Next object in linked list */
    u8 type;            /**<  4: Object type (game-defined) */
    u8 nID;             /**<  5: Unique ID in linked list */
    u16 sprnum;         /**<  6: Sprite OAM number (multiple of 4) */
    u16 sprid3216;      /**<  8: Sprite OAM id in sprite memory */
    u16 sprblk3216;     /**< 10: Sprite OAM VRAM block address */
    u8 sprflip;         /**< 12: Sprite flip attribute */
    u8 sprpal;          /**< 13: Sprite palette */
    u16 sprframe;       /**< 14: Current animation frame */
    u8 xpos[3];         /**< 16: X position (24-bit fixed point) */
    u8 ypos[3];         /**< 19: Y position (24-bit fixed point) */
    u16 xofs;           /**< 22: X offset from collision box origin */
    u16 yofs;           /**< 24: Y offset from collision box origin */
    u16 width;          /**< 26: Collision box width */
    u16 height;         /**< 28: Collision box height */
    u16 xmin;           /**< 30: Minimum X boundary */
    u16 xmax;           /**< 32: Maximum X boundary */
    s16 xvel;           /**< 34: X velocity (fixed point) */
    s16 yvel;           /**< 36: Y velocity (fixed point) */
    u16 tilestand;      /**< 38: Tile standing on (from collision) */
    u16 tileabove;      /**< 40: Tile above (from collision) */
    u16 tilesprop;      /**< 42: Tile property standing on */
    u16 tilebprop;      /**< 44: Tile property on left/right side */
    u16 action;         /**< 46: Current action (ACT_STAND, ACT_WALK, etc.) */
    u8 status;          /**< 48: Collision status */
    u8 tempo;           /**< 49: Object tempo counter */
    u8 count;           /**< 50: General-purpose counter */
    u8 dir;             /**< 51: Direction (game-defined) */
    u16 parentID;       /**< 52: Parent object ID (for projectiles) */
    u8 hitpoints;       /**< 54: Hit points */
    u8 sprrefresh;      /**< 55: Set to 1 to refresh sprite */
    u8 onscreen;        /**< 56: 1 if object is visible on screen */
    u8 objnotused[7];   /**< 57-63: Reserved for future use */
} t_objs;

/*============================================================================
 * Compile-time struct layout assertions
 * Must match the .STRUCT t_objs in object.asm exactly.
 *============================================================================*/

_Static_assert(sizeof(t_objs) == 64, "t_objs must be 64 bytes");
_Static_assert(__builtin_offsetof(t_objs, xpos) == 16, "xpos offset mismatch");
_Static_assert(__builtin_offsetof(t_objs, ypos) == 19, "ypos offset mismatch");
_Static_assert(__builtin_offsetof(t_objs, xvel) == 34, "xvel offset mismatch");
_Static_assert(__builtin_offsetof(t_objs, yvel) == 36, "yvel offset mismatch");
_Static_assert(__builtin_offsetof(t_objs, action) == 46, "action offset mismatch");
_Static_assert(__builtin_offsetof(t_objs, type) == 4, "type offset mismatch");
_Static_assert(__builtin_offsetof(t_objs, width) == 26, "width offset mismatch");
_Static_assert(__builtin_offsetof(t_objs, height) == 28, "height offset mismatch");
_Static_assert(__builtin_offsetof(t_objs, status) == 48, "status offset mismatch");
_Static_assert(__builtin_offsetof(t_objs, onscreen) == 56, "onscreen offset mismatch");

/*============================================================================
 * Exported Variables
 *============================================================================*/

/* --- Bank $00 SLOT 1 (C-accessible, < $2000) --- */

/**
 * @brief Object workspace (Bank $00)
 *
 * Single 64-byte workspace populated before each C callback.
 * Read/write this in init, update, and refresh callbacks.
 * Changes are automatically copied back to the object buffer.
 */
extern t_objs objWorkspace;

/** @brief Current object pointer (byte offset, for internal use) */
extern u16 objptr;

/** @brief Set to 1 inside a callback to kill the current object */
extern u8 objtokill;

/** @brief Handle of the last object created by objNew() */
extern u16 objgetid;

/* --- Bank $7E SLOT 2 (NOT C-accessible, ASM-only) --- */
/* The actual object buffer array lives in Bank $7E:
 *   objbuffers[OB_MAX]  — 80 x 64 = 5120 bytes
 *   objfctinit/upd/ref  — function pointer tables (256 bytes each)
 * Access these ONLY through the workspace pattern:
 *   objGetPointer(handle) copies objbuffers[N] → objWorkspace
 *   Engine functions copy objWorkspace → objbuffers[N] after callbacks
 */

/*============================================================================
 * Functions
 *============================================================================*/

/**
 * @brief Initialize the object engine
 *
 * Clears all object buffers, resets linked lists, and sets default
 * gravity/friction values. Call once at game start.
 */
void objInitEngine(void);

/**
 * @brief Set gravity and friction values
 *
 * @param objgravity Gravity acceleration per frame (default: 41)
 * @param objfriction Horizontal friction deceleration (default: 16)
 */
void objInitGravity(u16 objgravity, u16 objfriction);

/**
 * @brief Register callback functions for an object type
 *
 * @param objtype Object type index (0-63)
 * @param initfct Init callback: void init(u16 xp, u16 yp, u16 type, u16 minx, u16 maxx)
 * @param updfct  Update callback: void update(u16 idx) — called per frame
 * @param reffct  Refresh callback: void refresh(u16 idx) — called for on-screen objects (or NULL)
 */
void objInitFunctions(u8 objtype, void *initfct, void *updfct, void *reffct);

/**
 * @brief Create a new object
 *
 * @param objtype Object type (0-63)
 * @param x Initial X position in map pixels
 * @param y Initial Y position in map pixels
 * @return Object handle (0 if no space available). Also stored in objgetid.
 *
 * @note After objNew, the new object data is copied to objWorkspace.
 *       Set width, height, and other fields on objWorkspace before returning
 *       from the init callback.
 */
u16 objNew(u8 objtype, u16 x, u16 y);

/**
 * @brief Get pointer to an object from its handle
 *
 * Validates the handle and populates objWorkspace with the object's data.
 * Sets objptr to the buffer offset (1-based), or 0 if invalid.
 *
 * @param objhandle Object handle (from objNew/objgetid)
 */
void objGetPointer(u16 objhandle);

/**
 * @brief Kill an object
 *
 * @param objhandle Object handle
 */
void objKill(u16 objhandle);

/**
 * @brief Kill all active objects
 */
void objKillAll(void);

/**
 * @brief Load objects from a data table
 *
 * Table format: [x:u16, y:u16, type:u16, minx:u16, maxx:u16]... terminated by 0xFFFF.
 * Calls each type's init function for every object loaded.
 *
 * @param sourceO Pointer to object data table
 */
void objLoadObjects(u8 *sourceO);

/**
 * @brief Update all active objects
 *
 * Iterates through all active objects. For each object within the
 * "virtual screen" (-64 to 320 X, -64 to 288 Y), calls the type's
 * update function with objWorkspace pre-populated.
 */
void objUpdateAll(void);

/**
 * @brief Refresh sprites for all on-screen objects
 *
 * Calls the refresh function for objects that are on screen
 * (-32 to 256 X, -32 to 224 Y). Useful to avoid flickering
 * in scrolling games.
 */
void objRefreshAll(void);

/**
 * @brief Check object collision with map tiles
 *
 * Tests collision in Y (gravity/jumping) then X (walls).
 * Updates tilestand, tileabove, tilesprop, tilebprop in objWorkspace.
 * Applies friction to X velocity. Applies gravity if airborne.
 *
 * @param objhandle Object index (as received in update callback)
 *
 * @note Syncs objWorkspace before and after — safe to call from C callbacks.
 */
void objCollidMap(u16 objhandle);

/**
 * @brief Check object collision with map tiles including slopes
 *
 * Like objCollidMap but also handles slope tiles (T_SLOPEU1..T_SLOPEUD2).
 *
 * @param objhandle Object index
 */
void objCollidMapWithSlopes(u16 objhandle);

/**
 * @brief Check object collision with map (no gravity)
 *
 * For top-down movement without gravity.
 *
 * @param objhandle Object index
 */
void objCollidMap1D(u16 objhandle);

/**
 * @brief Test collision between two objects
 *
 * Uses AABB (axis-aligned bounding box) collision.
 *
 * @param objhandle1 First object index
 * @param objhandle2 Second object index
 * @return 1 if collision detected, 0 otherwise
 */
u16 objCollidObj(u16 objhandle1, u16 objhandle2);

/**
 * @brief Update object position from velocity
 *
 * Adds xvel/yvel to xpos/ypos (24-bit fixed point).
 *
 * @param objhandle Object index
 *
 * @note Syncs objWorkspace before and after.
 */
void objUpdateXY(u16 objhandle);

#endif /* OPENSNES_OBJECT_H */
