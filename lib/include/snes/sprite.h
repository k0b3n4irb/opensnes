/**
 * @file sprite.h
 * @brief SNES Sprite (Object) Management
 *
 * Functions for managing hardware sprites (OBJ).
 *
 * ## SNES Sprite Limits
 *
 * - 128 sprites total
 * - 32 sprites per scanline
 * - Sizes: 8x8, 16x16, 32x32, 64x64
 * - 8 palettes (16 colors each, sharing color 0)
 *
 * ## Usage
 *
 * @code
 * // Initialize sprite system
 * oamInit();
 *
 * // Set up a sprite
 * oamSet(0, 100, 80, 0, 0, 0, 0);  // Sprite 0 at (100, 80)
 *
 * // In main loop
 * WaitForVBlank();
 * oamUpdate();  // Copy OAM buffer to hardware
 * @endcode
 *
 * @author OpenSNES Team
 * @copyright MIT License
 *
 * ## Attribution
 *
 * Based on: PVSnesLib sprite system by Alekmaul
 */

#ifndef OPENSNES_SPRITE_H
#define OPENSNES_SPRITE_H

#include <snes/types.h>

/*============================================================================
 * Constants
 *============================================================================*/

/** @brief Maximum number of hardware sprites */
#define MAX_SPRITES 128

/** @brief Sprite size indices (for oamInitGfxSet, oamInitEx) */
#define OBJ_SIZE8_L16   0   /**< Small=8x8, Large=16x16 */
#define OBJ_SIZE8_L32   1   /**< Small=8x8, Large=32x32 */
#define OBJ_SIZE8_L64   2   /**< Small=8x8, Large=64x64 */
#define OBJ_SIZE16_L32  3   /**< Small=16x16, Large=32x32 */
#define OBJ_SIZE16_L64  4   /**< Small=16x16, Large=64x64 */
#define OBJ_SIZE32_L64  5   /**< Small=32x32, Large=64x64 */

/** @brief Macro to convert size index to OBJSEL register value */
#define OBJ_SIZE_TO_REG(size) ((size) << 5)

/** @brief Y position to hide sprite */
#define OBJ_HIDE_Y  240

/** @brief Sprite visibility */
#define OBJ_SHOW    1   /**< Sprite visible */
#define OBJ_HIDE    0   /**< Sprite hidden */

/** @brief Sprite size selection */
#define OBJ_SMALL   0   /**< Use small sprite size */
#define OBJ_LARGE   1   /**< Use large sprite size */

/*============================================================================
 * Dynamic Sprite Engine Constants
 *============================================================================*/

/** @brief Sprite type identifiers for dynamic engine */
#define OBJ_SPRITE32    1   /**< 32x32 sprite identifier */
#define OBJ_SPRITE16    2   /**< 16x16 sprite identifier */
#define OBJ_SPRITE8     4   /**< 8x8 sprite identifier */

/** @brief Maximum sprites in VRAM upload queue */
#define OBJ_QUEUELIST_SIZE  128

/** @brief Maximum sprite transfers per frame (7 sprites * 6 bytes each) */
#define MAXSPRTRF   (7 * 6)

/*============================================================================
 * Sprite Lookup Tables (for dynamic sprite management)
 *============================================================================*/

/**
 * VRAM addressing lookup tables for dynamic sprite engines.
 * These tables convert sprite frame indices to VRAM addresses.
 *
 * Usage:
 *   u16 vramOffset = lkup16oamS[frameId];  // VRAM source offset
 *   u16 tileId = lkup16idT[spriteSlot];    // OAM tile number
 *   u16 vramDest = lkup16idB[spriteSlot];  // VRAM destination
 */

/** @brief VRAM source offsets for 16x16 sprites (64 entries) */
extern u16 lkup16oamS[];

/** @brief OAM tile IDs for 16x16 sprites - small size mode (64 entries) */
extern u16 lkup16idT[];

/** @brief OAM tile IDs for 16x16 sprites - large size mode (64 entries) */
extern u16 lkup16idT0[];

/** @brief VRAM destination addresses for 16x16 sprites (64 entries) */
extern u16 lkup16idB[];

/** @brief VRAM source offsets for 32x32 sprites (16 entries) */
extern u16 lkup32oamS[];

/** @brief OAM tile IDs for 32x32 sprites (16 entries) */
extern u16 lkup32idT[];

/** @brief VRAM destination addresses for 32x32 sprites (16 entries) */
extern u16 lkup32idB[];

/** @brief VRAM source offsets for 8x8 sprites (128 entries) */
extern u16 lkup8oamS[];

/** @brief OAM tile IDs for 8x8 sprites (128 entries) */
extern u16 lkup8idT[];

/** @brief VRAM destination addresses for 8x8 sprites (128 entries) */
extern u16 lkup8idB[];

/*============================================================================
 * Dynamic Sprite Structure
 *============================================================================*/

/**
 * @brief Dynamic sprite state structure (16 bytes, PVSnesLib compatible)
 *
 * Used by the dynamic sprite engine to track per-sprite state including
 * position, animation frame, and graphics pointer for VRAM uploads.
 *
 * @code
 * // Set up sprite 0
 * oambuffer[0].oamx = 100;
 * oambuffer[0].oamy = 80;
 * oambuffer[0].oamframeid = 0;
 * oambuffer[0].oamattribute = OBJ_PRIO(2) | OBJ_PAL(0);
 * oambuffer[0].oamrefresh = 1;  // Request VRAM upload
 * OAM_SET_GFX(0, sprite_tiles);  // Set 24-bit graphics address
 *
 * // In game loop
 * oamDynamic16Draw(0);  // Draw and queue VRAM upload if needed
 * @endcode
 */
typedef struct {
    s16 oamx;           /**< 0-1: X position on screen */
    s16 oamy;           /**< 2-3: Y position on screen */
    u16 oamframeid;     /**< 4-5: Frame index in sprite sheet */
    u8 oamattribute;    /**< 6: Attributes (vhoopppc) - flip, priority, palette, tile high bit */
    u8 oamrefresh;      /**< 7: Set to 1 to request VRAM upload of graphics */
    u16 oamgfxaddr;     /**< 8-9: Low 16-bit address of graphics data */
    u8 oamgfxbank;      /**< 10: Bank byte of graphics address */
    u8 _pad;            /**< 11: Padding byte */
    u16 _reserved1;     /**< 12-13: Padding for 16-byte alignment */
    u16 _reserved2;     /**< 14-15: Padding for 16-byte alignment */
} t_sprites;

/**
 * @brief Macro to set sprite graphics address from a pointer
 *
 * Properly stores the 24-bit address by splitting it into 16-bit address + bank byte.
 * This ensures correct structure layout matching the assembly expectations.
 *
 * @param id Sprite index (0-127)
 * @param gfx Pointer to graphics data (will be split into addr16 + bank)
 */
#define OAM_SET_GFX(id, gfx) do { \
    oambuffer[id].oamgfxaddr = (u16)(unsigned long)(gfx); \
    oambuffer[id].oamgfxbank = (u8)((unsigned long)(gfx) >> 16); \
} while(0)

/**
 * @brief Dynamic sprite buffer (128 entries)
 *
 * Game-level sprite state for the dynamic sprite engine.
 * Each entry tracks a sprite's position, animation frame, and graphics pointer.
 * Separate from oamMemory (hardware OAM buffer).
 */
extern t_sprites oambuffer[128];

/*============================================================================
 * Initialization
 *============================================================================*/

/**
 * @brief Initialize sprite system
 *
 * Clears OAM buffer and sets up default configuration.
 * Must be called before using any sprite functions.
 */
void oamInit(void);

/**
 * @brief Initialize sprite system with configuration
 *
 * @param size Sprite size configuration (OBJ_SIZE_*)
 * @param tileBase Base address for sprite tiles in VRAM (word address >> 13)
 */
void oamInitEx(u16 size, u16 tileBase);

/**
 * @brief Initialize sprite graphics and palette (PVSnesLib compatible)
 *
 * Loads sprite tiles to VRAM and palette to CGRAM, and configures
 * the sprite tile base address and sizes.
 *
 * @param tileSource Address of sprite tile graphics
 * @param tileSize Size of tile data in bytes
 * @param tilePalette Address of sprite palette data
 * @param paletteSize Size of palette data in bytes
 * @param paletteEntry Palette number (0-7, placed at color 128+entry*16)
 * @param vramAddr VRAM address for tiles (must be 8KB aligned)
 * @param oamSize Sprite size configuration (OBJ_SIZE_*)
 *
 * @code
 * extern char sprite_tiles[], sprite_tiles_end[];
 * extern char sprite_pal[];
 * oamInitGfxSet(sprite_tiles, sprite_tiles_end - sprite_tiles,
 *               sprite_pal, 32, 0, 0x6000, OBJ_SIZE16_L32);
 * @endcode
 */
void oamInitGfxSet(u8 *tileSource, u16 tileSize, u8 *tilePalette,
                   u16 paletteSize, u8 paletteEntry, u16 vramAddr, u8 oamSize);

/**
 * @brief Set sprite extended attributes
 *
 * @param id Sprite ID (0-127)
 * @param size Size selection (OBJ_SMALL or OBJ_LARGE)
 * @param visible Visibility (OBJ_SHOW or OBJ_HIDE)
 */
void oamSetEx(u8 id, u8 size, u8 visible);

/*============================================================================
 * Sprite Properties
 *============================================================================*/

/**
 * @brief Set sprite properties
 *
 * @param id Sprite ID (0-127)
 * @param x X position (0-511, negative wraps)
 * @param y Y position (0-255, use OBJ_HIDE_Y to hide)
 * @param tile Tile number (0-511)
 * @param palette Palette (0-7)
 * @param priority Priority (0-3, 3=highest)
 * @param flags Flip flags (bit 6 = H flip, bit 7 = V flip)
 *
 * @warning **Coordinate Variable Pattern**: Due to a compiler quirk, sprite
 * coordinates MUST be stored in a struct with s16 members for correct behavior.
 * Using separate u16 variables causes jerky horizontal movement.
 *
 * **CORRECT** (use struct with s16):
 * @code
 * typedef struct { s16 x, y; } Position;
 * Position player = {100, 100};
 * oamSet(0, player.x, player.y, 0, 0, 3, 0);
 * player.x += 1;  // Smooth movement
 * @endcode
 *
 * **WRONG** (separate u16 variables - causes bugs):
 * @code
 * u16 player_x = 100;  // DON'T do this!
 * u16 player_y = 100;
 * oamSet(0, player_x, player_y, 0, 0, 3, 0);
 * player_x += 1;  // Jerky movement!
 * @endcode
 *
 * See examples/graphics/backgrounds/continuous_scroll for details on this pattern.
 */
void oamSet(u16 id, u16 x, u16 y, u16 tile, u16 palette, u16 priority, u16 flags);

/**
 * @brief Set sprite X position
 *
 * @param id Sprite ID (0-127)
 * @param x X position
 */
void oamSetX(u8 id, u16 x);

/**
 * @brief Set sprite Y position
 *
 * @param id Sprite ID (0-127)
 * @param y Y position
 */
void oamSetY(u8 id, u8 y);

/**
 * @brief Set sprite position
 *
 * @param id Sprite ID (0-127)
 * @param x X position
 * @param y Y position
 */
void oamSetXY(u8 id, u16 x, u8 y);

/**
 * @brief Set sprite tile
 *
 * @param id Sprite ID (0-127)
 * @param tile Tile number (0-511)
 */
void oamSetTile(u8 id, u16 tile);

/**
 * @brief Set sprite visibility
 *
 * @param id Sprite ID (0-127)
 * @param visible OBJ_SHOW (1) or OBJ_HIDE (0)
 *
 * @note **Important**: SNES sprite visibility is controlled by Y position.
 * Setting Y to 240 (OBJ_HIDE_Y) hides the sprite below the visible screen.
 * This function only handles HIDING (sets Y=240). Passing OBJ_SHOW does
 * nothing - to show a sprite, set a valid Y coordinate using oamSetY() or
 * oamSet(). For explicit hiding, use oamHide() which is clearer.
 *
 * @code
 * oamSetVisible(0, OBJ_HIDE);  // Hides sprite 0 (sets Y=240)
 * oamSetVisible(0, OBJ_SHOW);  // Does nothing! Sprite stays hidden.
 * oamSetY(0, 100);             // This shows the sprite at Y=100
 * @endcode
 */
void oamSetVisible(u8 id, u8 visible);

/**
 * @brief Hide sprite
 *
 * @param id Sprite ID (0-127)
 */
void oamHide(u8 id);

/**
 * @brief Set sprite size (large/small)
 *
 * @param id Sprite ID (0-127)
 * @param large TRUE for large size, FALSE for small
 */
void oamSetSize(u16 id, u16 large);

/*============================================================================
 * OAM Update
 *============================================================================*/

/**
 * @brief Copy OAM buffer to hardware
 *
 * Call during VBlank to update sprite display.
 *
 * @code
 * WaitForVBlank();
 * oamUpdate();
 * @endcode
 */
void oamUpdate(void);

/**
 * @brief Clear all sprites
 *
 * Hides all sprites by moving them off-screen.
 */
void oamClear(void);

/*============================================================================
 * Metasprites
 *============================================================================*/

/**
 * @brief Metasprite item structure (PVSnesLib compatible)
 *
 * Each item represents one hardware sprite within a metasprite.
 * Uses 8-byte alignment for efficient access.
 */
typedef struct {
    s16 dx;         /**< X offset from metasprite origin */
    s16 dy;         /**< Y offset from metasprite origin */
    u16 tile;       /**< Tile number offset from base */
    u8 attr;        /**< Attributes: flip flags, palette offset, priority */
    u8 reserved;    /**< Padding for 8-byte alignment */
} MetaspriteItem;

/** @brief PVSnesLib compatibility typedef */
typedef MetaspriteItem t_metasprite;

/** @brief Metasprite item macro (PVSnesLib compatible) */
#define METASPR_ITEM(dx, dy, tile, attr) { (dx), (dy), (tile), (attr), 0 }

/** @brief End marker for metasprite data (dx = -128) */
#define METASPR_TERM { -128, 0, 0, 0, 0 }

/** @brief Legacy end marker value */
#define metasprite_end (-128)

/** @brief Metasprite palette attribute macro */
#define OBJ_PAL(pal)    ((pal) << 1)

/** @brief Metasprite priority attribute macro */
#define OBJ_PRIO(prio)  ((prio) << 4)

/** @brief Metasprite horizontal flip flag */
#define OBJ_FLIPX       0x40

/** @brief Metasprite vertical flip flag */
#define OBJ_FLIPY       0x80

/**
 * @brief Draw a metasprite (PVSnesLib compatible)
 *
 * Draws a multi-tile sprite composed of multiple hardware sprites.
 * The metasprite data is an array of MetaspriteItem structures
 * terminated by METASPR_TERM.
 *
 * @param startId First sprite ID to use (0-127)
 * @param x X position of metasprite origin
 * @param y Y position of metasprite origin
 * @param meta Pointer to metasprite data array (MetaspriteItem[])
 * @param baseTile Base tile number to add to each item's tile offset
 * @param basePalette Base palette (0-7) when item doesn't specify one
 * @param size Size selection for all sprites (OBJ_SMALL or OBJ_LARGE)
 *
 * @return Number of hardware sprites used
 *
 * @code
 * // Define a 32x32 metasprite using 4 16x16 sprites
 * const MetaspriteItem hero_frame0[] = {
 *     METASPR_ITEM(0,  0,  0, OBJ_PRIO(2)),   // Top-left
 *     METASPR_ITEM(16, 0,  1, OBJ_PRIO(2)),   // Top-right
 *     METASPR_ITEM(0,  16, 2, OBJ_PRIO(2)),   // Bottom-left
 *     METASPR_ITEM(16, 16, 3, OBJ_PRIO(2)),   // Bottom-right
 *     METASPR_TERM
 * };
 *
 * // Draw at position (100, 80)
 * oamDrawMeta(0, 100, 80, hero_frame0, 0, 0, OBJ_LARGE);
 * @endcode
 */
u8 oamDrawMeta(u8 startId, s16 x, s16 y, const MetaspriteItem *meta,
               u16 baseTile, u8 basePalette, u8 size);

/**
 * @brief Draw a metasprite with flip support
 *
 * Like oamDrawMeta but supports horizontal and vertical flipping
 * of the entire metasprite.
 *
 * @param startId First sprite ID to use
 * @param x X position
 * @param y Y position
 * @param meta Metasprite data
 * @param baseTile Base tile number
 * @param basePalette Base palette
 * @param size Size selection (OBJ_SMALL or OBJ_LARGE)
 * @param flipX Flip horizontally if non-zero
 * @param flipY Flip vertically if non-zero
 * @param width Metasprite width for flip calculations
 * @param height Metasprite height for flip calculations
 *
 * @return Number of hardware sprites used
 */
u8 oamDrawMetaFlip(u8 startId, s16 x, s16 y, const MetaspriteItem *meta,
                   u16 baseTile, u8 basePalette, u8 size,
                   u8 flipX, u8 flipY, u8 width, u8 height);

/**
 * @brief Draw a metasprite (legacy simple interface)
 *
 * Simplified interface for basic metasprite drawing.
 *
 * @param startId First sprite ID to use
 * @param x X position
 * @param y Y position
 * @param data Metasprite data (cast to const u8*)
 * @param palette Palette to use
 *
 * @return Number of hardware sprites used
 */
u8 oamDrawMetasprite(u8 startId, u16 x, u8 y, const u8 *data, u8 palette);

/*============================================================================
 * Dynamic Sprite Engine
 *============================================================================*/

/**
 * @brief Initialize the dynamic sprite engine
 *
 * Sets up VRAM regions for sprite graphics and initializes the VRAM upload
 * queue. Must be called before using oamDynamic*Draw functions.
 *
 * @param gfxsp0adr VRAM address for large sprites (e.g., 0x0000)
 * @param gfxsp1adr VRAM address for small sprites (e.g., 0x1000)
 * @param oamsp0init Starting OAM slot for large sprites (0-127, multiple of 4)
 * @param oamsp1init Starting OAM slot for small sprites (0-127, multiple of 4)
 * @param oamsize Sprite size configuration (OBJ_SIZE_*)
 *
 * @code
 * // Initialize for 16x16 small, 32x32 large sprites
 * // Large sprites at VRAM $0000, small at $1000
 * oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE16_L32);
 * @endcode
 */
void oamInitDynamicSprite(u16 gfxsp0adr, u16 gfxsp1adr,
                          u16 oamsp0init, u16 oamsp1init, u8 oamsize);

/**
 * @brief End frame processing for dynamic sprites
 *
 * Call after drawing all sprites each frame. Hides sprites that were
 * visible last frame but not drawn this frame, and resets per-frame counters.
 *
 * @code
 * // Game loop
 * while (1) {
 *     for (i = 0; i < numSprites; i++) {
 *         oamDynamic16Draw(i);
 *     }
 *     oamInitDynamicSpriteEndFrame();  // Hide unused sprites
 *     WaitForVBlank();
 *     oamVramQueueUpdate();  // Upload graphics to VRAM
 * }
 * @endcode
 */
void oamInitDynamicSpriteEndFrame(void);

/**
 * @brief Process VRAM upload queue
 *
 * Call during VBlank to upload queued sprite graphics to VRAM.
 * Transfers up to 7 sprites per frame to stay within VBlank budget.
 * If more sprites are queued, they will be transferred on subsequent frames.
 *
 * @note Must be called after WaitForVBlank() for proper timing.
 */
void oamVramQueueUpdate(void);

/**
 * @brief Draw a 32x32 dynamic sprite
 *
 * Updates OAM buffer with sprite position and attributes from oambuffer[id].
 * If oamrefresh is set, queues graphics for VRAM upload.
 *
 * @param id Index into oambuffer array (0-127)
 *
 * @code
 * oambuffer[0].oamx = 100;
 * oambuffer[0].oamy = 80;
 * oambuffer[0].oamframeid = 0;
 * oambuffer[0].oamattribute = OBJ_PRIO(2) | OBJ_PAL(0);
 * oambuffer[0].oamrefresh = 1;
 * oambuffer[0].oamgraphics = &sprite32_tiles;
 * oamDynamic32Draw(0);
 * @endcode
 */
void oamDynamic32Draw(u16 id);

/**
 * @brief Draw a 16x16 dynamic sprite
 *
 * Updates OAM buffer with sprite position and attributes from oambuffer[id].
 * If oamrefresh is set, queues graphics for VRAM upload.
 *
 * @param id Index into oambuffer array (0-127)
 */
void oamDynamic16Draw(u16 id);

/**
 * @brief Draw an 8x8 dynamic sprite
 *
 * Updates OAM buffer with sprite position and attributes from oambuffer[id].
 * If oamrefresh is set, queues graphics for VRAM upload.
 *
 * @param id Index into oambuffer array (0-127)
 */
void oamDynamic8Draw(u16 id);

/**
 * @brief Set graphics pointer for dynamic sprite (handles 24-bit address)
 *
 * This function properly sets both the 16-bit address and bank byte
 * for sprite graphics data. Use this instead of directly setting
 * oamgraphics/oamgfxbank to ensure correct bank handling.
 *
 * @param id Index into oambuffer array (0-127)
 * @param gfx Pointer to sprite graphics data in ROM
 *
 * @code
 * extern u8 sprite_tiles[];
 * oamSetGfx(0, sprite_tiles);  // Sets oambuffer[0].oamgraphics and .oamgfxbank
 * @endcode
 */
void oamSetGfx(u16 id, u8 *gfx);

#endif /* OPENSNES_SPRITE_H */
