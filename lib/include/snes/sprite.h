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

/* Removed on 2026-10-05 (1.0 plan, lot C): oamDrawMeta, oamDrawMetaFlip.
 * The replacements are in docs/UPGRADING.md; `make check-upgrade` names them. */

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
 * @brief Set sprite graphics address — any bank
 *
 * Stores the address AND the bank byte of @p gfx: a C pointer is a far
 * pointer, so the macro reads the bank from it. Until 2026-09-20 it wrote
 * bank 0 (a pre-A6 leftover: "cc65816 passes 16-bit pointers"), which only
 * worked while the linker happened to put the tiles in bank $00 — and read
 * the wrong bank, silently, for tiles declared with ASSET_SECTION.
 *
 * @param id Sprite index (0-127)
 * @param gfx Pointer to graphics data (any bank)
 */
#define OAM_SET_GFX(id, gfx) do { \
    oambuffer[id].oamgfxaddr = (u16)(u32)(const void *)(gfx); \
    oambuffer[id].oamgfxbank = (u8)((u32)(const void *)(gfx) >> 16); \
} while(0)

/* OAM_SET_GFX_BANK(id, gfx, bank) was removed on 2026-10-05: OAM_SET_GFX() reads
 * the bank from its pointer (docs/UPGRADING.md). */

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
 * @param name_base  OBJ name base, a PAGE NUMBER 0-7 — **not** a VRAM
 *                   address. Each page is $2000 WORD addresses (16 KB),
 *                   so tiles DMA'd to word $4000 need base 2. Prefer
 *                   OBJ_NAME_BASE(addr) over computing it by hand; use
 *                   OAM_DEFAULT_TILE_BASE for tiles at VRAM $0000.
 *
 * @warning The value is masked to 3 bits. Passing a VRAM address — the
 *          natural mistake, since every other VRAM parameter in the SDK
 *          takes one — silently yields a wrong base: `0x6000 & 7` is 0,
 *          and the sprites render whatever tiles sit at word 0. There is
 *          no diagnostic; the symptom is a sprite drawn as background
 *          garbage. The doc also once claimed $1000-word steps, which
 *          made fix32_orbit's sprite read blank VRAM (#115).
 */
void oamInit(u16 size, u16 name_base);

/**
 * @brief Convert a VRAM **word** address into an OBJ name base (0-7).
 *
 * OBJSEL stores the sprite tile area as a 3-bit *page number*, not an
 * address: page N means word address `N << 13`. Passing a VRAM address
 * to oamInit() directly is therefore wrong — it is masked to 3 bits, so
 * `0x6000` becomes base 0 and the sprites render whatever tiles happen
 * to live at word 0 (usually the background's). Nothing reports it.
 *
 * Use this macro and the intent survives the call:
 * @code
 * oamInit(OBJ_SIZE8_L16, OBJ_NAME_BASE(0x6000));   // base 3
 * @endcode
 *
 * @param vram_word_addr VRAM word address, a multiple of $2000
 */
#define OBJ_NAME_BASE(vram_word_addr) (((vram_word_addr) >> 13) & 0x07)

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
void oamInitGfxSet(const u8 *tileSource, u16 tileSize, const u8 *tilePalette,
                   u16 paletteSize, u8 paletteEntry, u16 vramAddr, u8 oamSize);

/*============================================================================
 * Sprite Properties
 *============================================================================*/

/* Sprite ids are u16 in every function below. The setters other than oamSet()
 * took a u8 until 2026-09-21: an id of 256 was truncated to 0 BEFORE the
 * `id >= 128` check could refuse it, and silently overwrote sprite 0. Same
 * stack slot either way — no ABI change, and a u8 variable still converts. */

/**
 * @brief Set sprite properties
 *
 * @param id Sprite ID (0-127)
 * @param x X position (0-511, negative wraps)
 * @param y Y position (0-255, use OBJ_HIDE_Y to hide): the picture line of
 *          the sprite's top row, 0 being the first visible line — the same
 *          y as bgSetScroll()'s, so a sprite and the background it stands
 *          on take the same camera value. (Until 2026-10-09 the library
 *          stored y - 1 and every sprite was drawn one line too high.)
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
 * @note Any integer type works for the coordinates you keep on your side
 * (`u16`, `s16`, struct members or plain variables). Until 2026-09-21 this
 * block warned that separate `u16` variables gave jerky movement and that an
 * `s16` struct was mandatory; that was a compiler defect of early 2026, long
 * fixed — `examples/input/move_sprite` uses plain `u16` and its manifest pins
 * the motion to the pixel. Prefer `s16` when a sprite can leave the screen by
 * the left or the top, so the off-screen test is a signed compare.
 * @see @ref perf "Measured frame costs" — what this call costs per frame in five real scenes.
 */
void oamSet(u16 id, u16 x, u16 y, u16 tile, u16 palette, u16 priority, u16 flags);

/**
 * @brief Set sprite X position
 *
 * @param id Sprite ID (0-127)
 * @param x X position
 */
void oamSetX(u16 id, u16 x);

/**
 * @brief Set sprite Y position
 *
 * @param id Sprite ID (0-127)
 * @param y Y position
 */
void oamSetY(u16 id, u16 y);

/**
 * @brief Set sprite position
 *
 * @param id Sprite ID (0-127)
 * @param x X position
 * @param y Y position
 */
void oamSetXY(u16 id, u16 x, u16 y);

/**
 * @brief Read back a sprite's X position
 *
 * What oamSet() / oamSetX() / oamSetXY() last stored, from the OAM shadow:
 * 0-511, the ninth bit included (a value of 256 or more is a sprite partly
 * or wholly off the left edge, as written with a negative X).
 *
 * @param id Sprite ID (0-127)
 * @return X position, 0 for an invalid id
 * @note A hidden sprite reads 257 (see oamHide()).
 */
u16 oamGetX(u16 id);

/**
 * @brief Read back a sprite's Y position
 *
 * The y that was given to oamSet() / oamSetY() / oamSetXY(), which is the
 * OAM byte.
 *
 * @param id Sprite ID (0-127)
 * @return Y position (0-255), 0 for an invalid id
 * @note A hidden sprite reads 240 (see oamHide()).
 */
u8 oamGetY(u16 id);

/**
 * @brief A batch of sprites in world coordinates, for oamPlaceWorld()
 *
 * Structure of arrays: one array per property, one element per sprite,
 * which is what the 65816 indexes cheaply. The arrays may live in ROM, in
 * plain RAM or in `FAR` RAM.
 */
typedef struct {
    const s16 *x;        /**<  0: world x of each sprite's top-left corner */
    const s16 *y;        /**<  4: world y */
    const u8  *tile;     /**<  8: tile number, low byte */
    const u8  *attr;     /**< 12: attribute byte (vhoopppc), as OAM_ATTR() builds it */
    u8  *visible;        /**< 16: out, one byte per sprite: 1 placed, 0 hidden. May be 0 */
    u8   first_id;       /**< 20: OAM id of the first sprite of the batch */
    u8   count;          /**< 21: number of sprites */
    u8   size;           /**< 22: width and height in pixels: 8, 16, 32 or 64 */
} OamWorldBatch;

/* Not under the host's syntax check (clang, 8-byte pointers): the layout
 * that matters is cc65816's, 4 bytes a pointer. */
#ifndef __clang__
_Static_assert(__builtin_offsetof(OamWorldBatch, y) == 4, "OamWorldBatch.y offset (sprite_world.asm)");
_Static_assert(__builtin_offsetof(OamWorldBatch, tile) == 8, "OamWorldBatch.tile offset (sprite_world.asm)");
_Static_assert(__builtin_offsetof(OamWorldBatch, attr) == 12, "OamWorldBatch.attr offset (sprite_world.asm)");
_Static_assert(__builtin_offsetof(OamWorldBatch, visible) == 16, "OamWorldBatch.visible offset (sprite_world.asm)");
_Static_assert(__builtin_offsetof(OamWorldBatch, first_id) == 20, "OamWorldBatch.first_id offset (sprite_world.asm)");
_Static_assert(__builtin_offsetof(OamWorldBatch, count) == 21, "OamWorldBatch.count offset (sprite_world.asm)");
_Static_assert(__builtin_offsetof(OamWorldBatch, size) == 22, "OamWorldBatch.size offset (sprite_world.asm)");
#endif

/**
 * @brief Place a batch of world-space sprites: camera, culling, OAM, in one call
 *
 * For each sprite of the batch: screen position = world position - camera.
 * A sprite that is on screen, even partly, gets its x, y, tile, attribute
 * and ninth x bit written; any other is hidden the way oamHide() hides.
 * The loop a scrolling game runs every frame, in assembly: in compiled C it
 * cost a real project about 6,000 master cycles a sprite (issue #165).
 *
 * - **Culling is by `size`, on both axes.** A 32x32 sprite at x = -31 is
 *   still drawn, with its ninth x bit set; at x = -32 it is hidden.
 * - **`visible[]`** tells the game which sprites were placed, so that it
 *   can skip its own work (animation, streaming) for the others without
 *   testing again. Pass 0 if it is not wanted.
 * - **The size bit is not touched**: set it once with oamSetSize(). A batch
 *   has one size; a game with two sizes makes two calls.
 * - **`cam_y` is the y you give bgSetScroll()**: a sprite and the
 *   background it stands on take the same camera value.
 * - A batch that runs past sprite 127 stops there.
 *
 * @param batch The sprites (see OamWorldBatch)
 * @param cam_x World x of the screen's left edge
 * @param cam_y World y of the screen's first line
 *
 * @code
 * static s16 ax[12], ay[12];              // moved by the game
 * static u8  atile[12], aattr[12], aseen[12];
 * static const OamWorldBatch actors = {
 *     ax, ay, atile, aattr, aseen, 0, 12, 32
 * };
 *
 * oamPlaceWorld(&actors, cam_x, cam_y);   // every frame, after the game moved them
 * @endcode
 */
void oamPlaceWorld(const OamWorldBatch *batch, u16 cam_x, u16 cam_y);

/**
 * @brief Set sprite tile
 *
 * @param id Sprite ID (0-127)
 * @param tile Tile number (0-511)
 */
void oamSetTile(u16 id, u16 tile);

/* Note: there is deliberately no oamSetVisible(id, show) — SNES sprite
 * visibility is Y-position-based, so a "show" call can't know which Y to
 * restore and could only be a silent no-op (which the removed v0.x
 * function was). Use oamHide() to hide and oamSetY()/oamSet() with a
 * valid Y to show. */

/**
 * @brief Park a sprite off screen.
 *
 * Use this for **anything outside the camera**. OAM X is 9 bits and Y is
 * 8: a sprite drawn at a position beyond the screen does not disappear,
 * its coordinates WRAP and it reappears somewhere plausible. In a
 * scrolling world that reads as an entity standing where no entity is —
 * a villager inside a wall — which you find by looking at the screen,
 * not by reading the code.
 *
 * @code
 * s16 sx = (s16)world_x - (s16)cam_x;
 * s16 sy = (s16)world_y - (s16)cam_y;
 * if (sx < -16 || sx > 255 || sy < -16 || sy > 223) {
 *     oamHide(id);
 * } else {
 *     oamSet(id, (u16)sx, (u16)sy, tile, pal, prio, 0);
 * }
 * @endcode
 *
 * Sets Y to OBJ_HIDE_Y **and** X's high bit, because Y=240 alone still
 * wraps for sprites taller than 16 px. That is why this is a function
 * and not a `oamSetXY(id, 0, 240)` you write yourself.
 *
 * @param id Sprite ID (0-127)
 *
 * @see examples/games/rpg — culls its villagers this way
 * @see @ref perf "Measured frame costs" — what this call costs per frame in five real scenes.
 */
void oamHide(u16 id);

/**
 * @brief Set sprite size (large/small)
 *
 * @param id Sprite ID (0-127)
 * @param large TRUE for large size, FALSE for small
 * @see @ref perf "Measured frame costs" — what this call costs per frame in five real scenes.
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
 *
 * @par Tile units (tooling-facing contract)
 * `tile` is in **8x8 OAM character-name units** relative to the base tile,
 * for the sprite sheet as it sits in VRAM. It is NOT a block index: a
 * 16x16 sub-sprite one block to the right of another is `+2` (two 8x8
 * columns), and moving one block row down adds the sheet's row stride
 * (16 names for a 128px-wide sheet). gfx4snes `-T` emits these names
 * directly (fixed in issue #100); editors composing their own sheet
 * (Cooper) compute them from their own geometry.
 */
typedef struct {
    s16 dx;         /**< X offset from metasprite origin */
    s16 dy;         /**< Y offset from metasprite origin */
    u16 tile;       /**< Character name offset from base, 8x8 units (see above) */
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
 * @brief What stays the same from one frame of a metasprite to the next
 *
 * Declare one per character, usually `static const`; the frame, the
 * position and the flip are given at each call.
 */
typedef struct {
    u16 baseTile;     /**< Tile number added to each item's tile offset */
    u8  basePalette;  /**< Palette (0-7) for the items that name none */
    u8  size;         /**< OBJ_SMALL or OBJ_LARGE, for every piece */
    u8  pieceSize;    /**< Pixels of one piece (8, 16, 32 or 64): the size
                       *   `size` selects in your OBJSEL mode. Read only
                       *   when flipping */
    u8  width;        /**< Width of the whole metasprite in pixels. Read
                       *   only when flipping */
    u8  height;       /**< Height of the whole metasprite in pixels. Read
                       *   only when flipping */
} MetaspriteStyle;

/**
 * @brief Draw one frame of a metasprite
 *
 * Draws a multi-tile sprite composed of multiple hardware sprites. The
 * frame is an array of MetaspriteItem terminated by METASPR_TERM — what
 * animTickMeta() returns, or a table of your own.
 *
 * @param startId First sprite ID to use (0-127)
 * @param x X position of the metasprite origin
 * @param y Y position of the metasprite origin
 * @param frame The frame to draw (MetaspriteItem[])
 * @param style Base tile, palette, size and extent (see MetaspriteStyle)
 * @param flip 0, or OBJ_FLIPX and / or OBJ_FLIPY to mirror the whole
 *             metasprite inside its `width` x `height` box. Each piece is
 *             moved to the mirrored place and its own flip bit toggled.
 *
 * @return The next free sprite id (startId + the number of sprites drawn) —
 *         chain calls with it, as examples/sprites/metasprite does. Items
 *         that fall off screen are skipped WITHOUT consuming an id, so the
 *         count varies per frame: hide the ids you used last frame and no
 *         longer use.
 *
 * @code
 * // A 32x32 character made of four 16x16 sprites
 * const MetaspriteItem hero_frame0[] = {
 *     METASPR_ITEM(0,  0,  0, OBJ_PRIO(2)),   // Top-left
 *     METASPR_ITEM(16, 0,  1, OBJ_PRIO(2)),   // Top-right
 *     METASPR_ITEM(0,  16, 2, OBJ_PRIO(2)),   // Bottom-left
 *     METASPR_ITEM(16, 16, 3, OBJ_PRIO(2)),   // Bottom-right
 *     METASPR_TERM
 * };
 * static const MetaspriteStyle hero_style = {
 *     .baseTile = 0, .basePalette = 0, .size = OBJ_LARGE,
 *     .pieceSize = 16, .width = 32, .height = 32,
 * };
 *
 * oamDrawMetasprite(0, 100, 80, hero_frame0, &hero_style, 0);          // facing right
 * oamDrawMetasprite(0, 100, 80, hero_frame0, &hero_style, OBJ_FLIPX);  // facing left
 * @endcode
 * @see @ref perf "Measured frame costs" — what this call costs per frame in five real scenes.
 */
u16 oamDrawMetasprite(u16 startId, s16 x, s16 y, const MetaspriteItem *frame,
                      const MetaspriteStyle *style, u8 flip);

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
    u16 vramLarge;      /**< VRAM base for large-size tile pool (was gfxsp0adr).
                         *   Only 0x0000 is honoured today: OBJSEL's name base is
                         *   written as 0 and the tile-number tables are fixed, so
                         *   another value sends the tiles one way and OAM the other */
    u16 vramSmall;      /**< VRAM base for small-size tile pool (was gfxsp1adr).
                         *   Only 0x1000 is honoured today, for the same reason */
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
 * @see @ref perf "Measured frame costs" — what this call costs per frame in five real scenes.
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
                    const MetaspriteItem *meta, const u8 *gfxptr, u8 size_class);

/*============================================================================
 * Fast Macro Sprite API
 *
 * Zero-overhead alternatives to oamSet/oamSetXY for performance-critical code.
 * These write directly to oamMemory[] without function call overhead.
 *
 * oamSet() is a call with seven arguments pushed and a stack frame; for a
 * handful of sprites updated every frame the macros below write the four
 * OAM bytes in place. (A byte count of an earlier oamSet's frame and a
 * "visible jitter" claim stood here until 2026-10-04; the cost is in
 * docs/PERF.md, measured, not here.)
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
    oamMemory[_off + 1] = (u8)((_y) & 0xFF); \
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
    oamMemory[_off + 1] = (u8)((_y) & 0xFF); \
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
