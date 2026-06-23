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
 * // Initialize sprite system (defaults: 8x8/16x16 sprites, tile base 0)
 * oamInit(OAM_DEFAULT_SIZE, OAM_DEFAULT_TILE_BASE);
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

/** @brief Sprite size indices (for oamInit, oamInitGfxSet) */
#define OBJ_SIZE8_L16   0   /**< Small=8x8, Large=16x16 */
#define OBJ_SIZE8_L32   1   /**< Small=8x8, Large=32x32 */
#define OBJ_SIZE8_L64   2   /**< Small=8x8, Large=64x64 */
#define OBJ_SIZE16_L32  3   /**< Small=16x16, Large=32x32 */
#define OBJ_SIZE16_L64  4   /**< Small=16x16, Large=64x64 */
#define OBJ_SIZE32_L64  5   /**< Small=32x32, Large=64x64 */

/** @brief Macro to convert size index to OBJSEL register value */
#define OBJ_SIZE_TO_REG(size) ((size) << 5)

/** @brief Convert VRAM word address to OBJSEL name base bits (0-2) */
#define OBJ_BASE(vram_addr) (((vram_addr) >> 13) & 0x07)

/**
 * @brief Build OBJSEL register value from size constant + VRAM base address
 * @param size One of OBJ_SIZE8_L16 .. OBJ_SIZE32_L64
 * @param vram_addr VRAM word address for sprite tiles (must be 8KB-aligned)
 *
 * @code
 * REG_OBJSEL = OBJSEL(OBJ_SIZE16_L32, 0x4000);  // = 0x62
 * REG_OBJSEL = OBJSEL(OBJ_SIZE8_L16,  0x4000);  // = 0x02
 * @endcode
 */
#define OBJSEL(size, vram_addr) ((u8)(OBJ_SIZE_TO_REG(size) | OBJ_BASE(vram_addr)))

/** @brief CGRAM base offset for sprite palettes (sprites use colors 128-255) */
#define OBJ_CGRAM_BASE  128

/** @brief CGRAM color index for sprite palette n (0-7) */
#define OBJ_CGRAM_PAL(n)  (OBJ_CGRAM_BASE + (n) * 16)

/** @brief Size of one 16-color palette in bytes (16 colors x 2 bytes per BGR555) */
#define PALETTE_16_SIZE  32

/** @brief Y coordinate that hides a sprite below the NTSC visible area */
#define OAM_Y_OFFSCREEN  0xE0

/** @brief OAM extension table offset in bytes (starts after 128*4 main entries) */
#define OAM_EXT_OFFSET  512

/** @brief OAM extension table size in bytes (2 bits per sprite, 128 sprites) */
#define OAM_EXT_SIZE    32

/** @brief Total OAM buffer size in bytes (512 main + 32 extension) */
#define OAM_BUFFER_SIZE 544

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
 * oamDynamicDraw(0);  // Draw + queue VRAM upload (NMI auto-flushes)
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

/*============================================================================
 * Compile-time struct layout assertions
 * Must match the oambuffer layout in sprite_dynamic.asm exactly.
 *============================================================================*/

_Static_assert(sizeof(t_sprites) == 16, "t_sprites must be 16 bytes");
_Static_assert(__builtin_offsetof(t_sprites, oamx) == 0, "oamx offset mismatch");
_Static_assert(__builtin_offsetof(t_sprites, oamy) == 2, "oamy offset mismatch");
_Static_assert(__builtin_offsetof(t_sprites, oamframeid) == 4, "oamframeid offset mismatch");
_Static_assert(__builtin_offsetof(t_sprites, oamattribute) == 6, "oamattribute offset mismatch");
_Static_assert(__builtin_offsetof(t_sprites, oamrefresh) == 7, "oamrefresh offset mismatch");
_Static_assert(__builtin_offsetof(t_sprites, oamgfxaddr) == 8, "oamgfxaddr offset mismatch");
_Static_assert(__builtin_offsetof(t_sprites, oamgfxbank) == 10, "oamgfxbank offset mismatch");

/**
 * @brief Set sprite graphics address (bank $00 only)
 *
 * Sets the 16-bit graphics address with bank byte = 0.
 * cc65816 passes 16-bit pointers, so the bank byte is always lost
 * before this macro runs. Use OAM_SET_GFX_BANK() for data in other banks.
 *
 * @param id Sprite index (0-127)
 * @param gfx Pointer to graphics data in bank $00
 */
#define OAM_SET_GFX(id, gfx) do { \
    oambuffer[id].oamgfxaddr = (u16)(gfx); \
    oambuffer[id].oamgfxbank = 0; \
} while(0)

/**
 * @brief Set sprite graphics address with explicit bank byte
 *
 * Use this when sprite data is in a bank other than $00.
 *
 * @param id Sprite index (0-127)
 * @param gfx Pointer to graphics data
 * @param bank ROM bank where graphics data is located (0-255)
 */
#define OAM_SET_GFX_BANK(id, gfx, bank) do { \
    oambuffer[id].oamgfxaddr = (u16)(gfx); \
    oambuffer[id].oamgfxbank = (u8)(bank); \
} while(0)

/* --- Bank $00 SLOT 1 (C-accessible, < $2000) --- */

/**
 * @brief Dynamic sprite buffer (128 entries, 2048 bytes)
 *
 * Game-level sprite state for the dynamic sprite engine.
 * Each entry tracks a sprite's position, animation frame, and graphics pointer.
 * Separate from oamMemory (hardware OAM buffer at $0300-$051F).
 */
extern t_sprites oambuffer[128];

/*============================================================================
 * Initialization
 *============================================================================*/

/**
 * @brief Default sprite size mode — small=8×8 / large=16×16.
 *
 * Same value as OBJ_SIZE8_L16 (the most common configuration: 8×8 tiles
 * for HUD elements and 16×16 for characters/projectiles).
 */
#define OAM_DEFAULT_SIZE        OBJ_SIZE8_L16
/** @brief Default sprite tile base — 0 = tiles at VRAM word $0000. */
#define OAM_DEFAULT_TILE_BASE   0

/**
 * @brief Initialize the sprite (OAM) system.
 *
 * Sets the OBJSEL register (sprite size mode + tile base) and clears the
 * OAM shadow buffer so all sprites start hidden. Must be called before
 * any other oam* function. Replaces the v1 oamInit/oamInitEx pair —
 * pass OAM_DEFAULT_* constants for the previous oamInit(void) defaults.
 *
 * @param size       Sprite size mode (OBJ_SIZE_*; use OAM_DEFAULT_SIZE for
 *                   the standard 8×8/16×16 layout)
 * @param tile_base  VRAM tile-base index 0-7 (each step = $1000 word
 *                   addresses; use OAM_DEFAULT_TILE_BASE for tiles at
 *                   VRAM $0000)
 */
void oamInit(u16 size, u16 tile_base);

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
 * @note **Performance**: the old framesize=158 cliff (≈3 calls/frame caused
 * jitter) was RESOLVED by an ASM rewrite — see KNOWN_LIMITATIONS.md
 * ("oamSet() framesize cliff — RESOLVED"). `oamSet()` is now cheap; use it
 * freely. For extreme sprite counts the `oamSetFast()` / `oamSetXYFast()` macros
 * (see "Fast Macro Sprite API" below) trim a little more overhead.
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

/** @brief OAM attribute bit 0: second name table select (tile numbers 256+) */
#define OBJ_NAMETABLE_HIGH  0x01

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

/*============================================================================
 * Dynamic Sprite Engine
 *============================================================================*/

/**
 * @brief Configuration for the dynamic sprite engine.
 *
 * Pass this to `oamDynamicInit` instead of the 5 positional arguments of
 * `oamInitDynamicSprite`. Equivalent at runtime; clearer at the call site
 * and easier to evolve without breaking existing callers.
 */
typedef struct {
    u16 vramLarge;      /**< VRAM base for large-size tile pool (was gfxsp0adr) */
    u16 vramSmall;      /**< VRAM base for small-size tile pool (was gfxsp1adr) */
    u16 slotLargeInit;  /**< Initial OAM slot for large sprites (was oamsp0init) */
    u16 slotSmallInit;  /**< Initial OAM slot for small sprites (was oamsp1init) */
    u8  sizeMode;       /**< OBJ_SIZE_* — defines the small/large pixel sizes */
} OamDynamicConfig;

/**
 * @brief Initialize the dynamic sprite engine from a config struct.
 *
 * Preferred over `oamInitDynamicSprite` — the struct-based form makes the
 * intent of each parameter visible at the call site and survives future
 * additions without a signature break.
 *
 * @param cfg Configuration (caller-owned; the engine reads it once).
 *
 * @code
 * static const OamDynamicConfig dyn = {
 *     .vramLarge      = 0x0000,
 *     .vramSmall      = 0x1000,
 *     .slotLargeInit  = 0,
 *     .slotSmallInit  = 0,
 *     .sizeMode       = OBJ_SIZE16_L32,
 * };
 * oamDynamicInit(&dyn);
 * @endcode
 */
void oamDynamicInit(const OamDynamicConfig *cfg);

/**
 * @brief Override the dispatched pixel size for a dynamic sprite slot.
 *
 * Optional companion to `oamDynamicDraw`. By default each slot dispatches
 * to the "large" half of the size pair set at init. Call this to make a
 * specific slot use a different pixel size (8, 16, or 32) — typical when
 * mixing small and large dynamic sprites under the same mode pair.
 *
 * Pass 0 to clear the override and revert to the mode default.
 *
 * @param id   Sprite slot id (0-127)
 * @param size Pixel size: 8, 16, or 32 (or 0 to clear)
 */
void oamDynamicSetSize(u16 id, u8 size);

/**
 * @brief Draw a dynamic sprite — engine picks the size routine.
 *
 * Preferred entry point — replaces the manual choice between
 * `oamDynamic8Draw` / `oamDynamic16Draw` / `oamDynamic32Draw`. The engine
 * uses the per-sprite size set via `oamDynamicSetSize` if any; otherwise
 * it falls back to the "large" pixel size of the size pair selected at
 * init by `oamInitDynamicSprite` (or `oamDynamicInit`).
 *
 * 64x64 dynamic streaming is not currently supported; calls that would
 * resolve to 64x64 are silently skipped.
 *
 * @param id Index into oambuffer array (0-127)
 */
void oamDynamicDraw(u16 id);

/**
 * @brief Block until the dynamic-sprite VRAM tile queue is empty.
 *
 * Called once during init, after queueing the starting frame via
 * `oamDynamicDraw` / `oamMetaDrawDyn`, and **before** `setScreenOn`. The
 * NMI auto-flush hook drains up to 7 queue entries per VBlank, so init
 * sequences that enqueue more than that (typical for metasprites with
 * many sub-sprites) need several VBlanks to complete. This helper loops
 * `WaitForVBlank()` until the queue is empty and tells the NMI hook to
 * skip the end-of-frame "hide stale sprites" step during the drain so
 * the just-drawn sprites are not pushed off-screen between waits.
 *
 * Safe to call only while the screen is in force blank — VRAM writes
 * during active display are silently dropped by the PPU.
 *
 * @code
 * setScreenOff();                  // force blank
 * oamDynamicInit(&dyn);
 * drawAllSprites();                // queues many tile uploads
 * oamDynamicDrainQueue();          // wait until VRAM matches OAM
 * setScreenOn();                   // first frame renders correctly
 * @endcode
 */
void oamDynamicDrainQueue(void);

/*============================================================================
 * Dynamic Metasprite Engine
 *============================================================================*/

/**
 * @brief Draw a dynamic metasprite — engine picks the size routine.
 *
 * Iterates `meta` (a MetaspriteItem array terminated by METASPR_TERM),
 * sets up `oambuffer[id..]` for each sub-sprite, and dispatches to the
 * matching `oamDynamic{8,16,32}Draw` based on the engine's current size
 * pair (set at init via `oamDynamicInit`) and the caller-supplied
 * `size_class`:
 *
 *   - `OBJ_SMALL` → small half of the size pair, with `OBJ_NAMETABLE_HIGH`
 *     ORed into the attribute byte so the tile id reaches the second name
 *     table (where small dynamic tiles live in modes 0/1/3).
 *   - `OBJ_LARGE` → large half of the size pair.
 *
 * Replaces the legacy trio `oamMetaDrawDyn{8,16,32}`. The pixel size is
 * resolved from the engine state, so callers no longer pick a function
 * by sprite size — they just say which half of the pair to use.
 *
 * @param id          Starting oambuffer index (each sub-sprite uses
 *                    `id`, `id+1`, ...).
 * @param x,y         Metasprite origin in screen coordinates.
 * @param meta        MetaspriteItem array, terminated by METASPR_TERM.
 * @param gfxptr      ROM source for the dynamic tile data
 *                    (bank $00 — cc65816 passes 16-bit pointers only).
 * @param size_class  `OBJ_SMALL` (0) or `OBJ_LARGE` (1).
 *
 * @code
 * static const OamDynamicConfig dyn = {
 *     .vramLarge = 0x0000, .vramSmall = 0x1000,
 *     .slotLargeInit = 0,  .slotSmallInit = 0,
 *     .sizeMode = OBJ_SIZE16_L32,
 * };
 * oamDynamicInit(&dyn);
 *
 * const MetaspriteItem hero[] = {
 *     METASPR_ITEM(0,  0,  0, OBJ_PRIO(2)),
 *     METASPR_ITEM(16, 0,  1, OBJ_PRIO(2)),
 *     METASPR_ITEM(0,  16, 2, OBJ_PRIO(2)),
 *     METASPR_ITEM(16, 16, 3, OBJ_PRIO(2)),
 *     METASPR_TERM
 * };
 * oambuffer[0].oamrefresh = 1;
 * oamMetaDrawDyn(0, 100, 80, hero, hero_tiles, OBJ_LARGE);
 * @endcode
 */
void oamMetaDrawDyn(u16 id, s16 x, s16 y,
                    const MetaspriteItem *meta, u8 *gfxptr, u8 size_class);

/*============================================================================
 * Fast Macro Sprite API
 *
 * Zero-overhead alternatives to oamSet/oamSetXY for performance-critical code.
 * These write directly to oamMemory[] without function call overhead.
 *
 * oamSet() has framesize=158 per call due to SSA temporaries. With >2-3
 * sprites/frame in the main loop, the stack manipulation causes visible
 * jitter. These macros eliminate that overhead entirely.
 *
 * Note: cc65816 does not truly inline 'static inline' functions — they
 * become separate SUPERFREE sections with global labels that conflict
 * across translation units. Macros are the only zero-overhead option.
 *
 * oamMemory[] and oam_update_flag are declared in <snes/system.h>
 * (included automatically via <snes.h>).
 *
 * Usage:
 *   // Pre-compute attribute byte once at init
 *   u8 attr = OAM_ATTR(tile, palette, priority, flags);
 *
 *   // Per-frame: fast full update
 *   oamSetFast(id, x, y, tile, palette, priority, flags);
 *
 *   // Per-frame: position-only update (most common)
 *   oamSetXYFast(id, x, y);
 *============================================================================*/

/**
 * @brief Pre-compute OAM attribute byte (vhoopppc)
 *
 * @param _tile Tile number (0-511, only bit 8 used)
 * @param _pal Palette (0-7)
 * @param _prio Priority (0-3)
 * @param _fl Flip flags (bit 6 = H flip, bit 7 = V flip)
 * @return Packed attribute byte
 */
#define OAM_ATTR(_tile, _pal, _prio, _fl) \
    ((u8)(((_fl) & 0xC0) | \
          (((_prio) & 0x03) << 4) | \
          (((_pal) & 0x07) << 1) | \
          (((_tile) >> 8) & 0x01)))

/**
 * @brief X-high-bit mask for a sprite slot (0-3 within a high-table byte)
 *
 * Each high-table byte covers 4 sprites. Bit 0 of each 2-bit pair is the
 * X high bit. This macro returns the mask for the X-high bit of the given slot.
 */
#define OAM_XHI_MASK(_slot) \
    ((u8)((_slot) == 0 ? 0x01 : (_slot) == 1 ? 0x04 : \
          (_slot) == 2 ? 0x10 : 0x40))

/**
 * @brief Set sprite properties with zero function-call overhead
 *
 * Drop-in replacement for oamSet() that writes directly to oamMemory[].
 * Same parameters, same behavior, but compiles to direct memory writes
 * instead of a function call with framesize=158.
 *
 * @param _id Sprite ID (0-127)
 * @param _x X position (0-511)
 * @param _y Y position (0-255)
 * @param _tile Tile number (0-511)
 * @param _pal Palette (0-7)
 * @param _prio Priority (0-3)
 * @param _fl Flip flags (bit 6 = H flip, bit 7 = V flip)
 */
#define oamSetFast(_id, _x, _y, _tile, _pal, _prio, _fl) do { \
    u16 _off = (u16)(_id) << 2; \
    oamMemory[_off + 0] = (u8)((_x) & 0xFF); \
    oamMemory[_off + 1] = (u8)(((_y) - 1) & 0xFF); /* compensate +1 PPU scanline quirk */ \
    oamMemory[_off + 2] = (u8)((_tile) & 0xFF); \
    oamMemory[_off + 3] = OAM_ATTR(_tile, _pal, _prio, _fl); \
    u16 _ext = 512 + ((u16)(_id) >> 2); \
    u16 _sl = (u16)(_id) & 0x03; \
    u8 _xhi = OAM_XHI_MASK(_sl); \
    if ((_x) & 0x100) \
        oamMemory[_ext] |= _xhi; \
    else \
        oamMemory[_ext] &= ~_xhi; \
    oam_update_flag = 1; \
} while(0)

/**
 * @brief Update sprite position only (most common per-frame operation)
 *
 * Fastest possible sprite update — only writes X, Y, and X high bit.
 * Use when tile/palette/priority/flags don't change between frames.
 *
 * @param _id Sprite ID (0-127)
 * @param _x X position (0-511)
 * @param _y Y position (0-255)
 */
#define oamSetXYFast(_id, _x, _y) do { \
    u16 _off = (u16)(_id) << 2; \
    oamMemory[_off + 0] = (u8)((_x) & 0xFF); \
    oamMemory[_off + 1] = (u8)(((_y) - 1) & 0xFF); /* compensate +1 PPU scanline quirk */ \
    u16 _ext = 512 + ((u16)(_id) >> 2); \
    u16 _sl = (u16)(_id) & 0x03; \
    u8 _xhi = OAM_XHI_MASK(_sl); \
    if ((_x) & 0x100) \
        oamMemory[_ext] |= _xhi; \
    else \
        oamMemory[_ext] &= ~_xhi; \
    oam_update_flag = 1; \
} while(0)

#endif /* OPENSNES_SPRITE_H */
