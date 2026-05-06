/**
 * @file asset.h
 * @brief Opt-in asset bundle convention — load a (tiles + palette +
 *        optional tilemap) bundle in one statement.
 * @ingroup framework
 *
 * One of the three "framework opt-ins" promised by `PHILOSOPHY.md`
 * (alongside `gameloop` and the planned scene/state stack). This module
 * removes a recurring piece of SNES boilerplate: declaring six `extern`
 * symbols (begin/end pairs for tiles, palette, tilemap), computing
 * three sizes by subtraction at every call site, and threading them
 * through an 8-parameter `bgInitTileSet()` followed by a separate
 * `dmaCopyVram()` for the map.
 *
 * @par Before
 * @code{.c}
 *     extern u8 tiles[],   tiles_end[];
 *     extern u8 palette[], palette_end[];
 *     extern u8 tilemap[], tilemap_end[];
 *
 *     bgSetMapPtr(0, 0x0000, SC_32x32);
 *     bgInitTileSet(0, tiles, palette, 0,
 *                   tiles_end - tiles,
 *                   palette_end - palette,
 *                   BG_16COLORS, 0x4000);
 *     dmaCopyVram(tilemap, 0x0000, tilemap_end - tilemap);
 * @endcode
 *
 * @par After
 * @code{.c}
 *     // data.asm exports bg_tiles, bg_pal, bg_map (each with _end labels).
 *     BG_LOAD(bg, 0, 0, BG_16COLORS, SC_32x32, 0x4000, 0x0000);
 * @endcode
 *
 * The macros expect the symbol naming convention `<name>_tiles`,
 * `<name>_pal`, `<name>_map` plus their `_end` siblings. The user
 * chooses the prefix once in their `data.asm` (one rename, applies
 * across the whole example). Users who do not want to follow the
 * convention can keep calling `bgInitTileSet` + `dmaCopyVram`
 * directly — the macros do not replace the underlying functions.
 *
 * @par Two flavours, same convention
 *
 * The header exposes both a macro form (`BG_LOAD`, `GFX_LOAD`) that
 * inlines into the call site, and a typed-value form (`BgAsset`,
 * `GfxAsset` + `bgLoad()`, `gfxLoad()`) that lets you store an asset
 * in a `static const`, pass it to a function, or build a level table
 * indexed at runtime. Both forms expand to the same hardware calls;
 * pick whichever fits the use site.
 *
 * @par What this module does NOT do
 * - It does not configure the background mode (`setMode`), the main
 *   screen mask (`setMainScreen`), or the screen brightness
 *   (`setScreenOn`). The caller still owns the high-level init order.
 * - It does not handle sprite asset bundles. Sprites have their own
 *   CGRAM offset (128) and OAM conventions; a future macro family
 *   may be added once the patterns settle.
 * - It does not handle the Tiled mapdata pipeline (tilesetdef,
 *   tilesetatt, large levels). Those still go through `mapLoad()`.
 *
 * @par Performance
 * Both macros expand inline to existing library calls
 * (`bgSetMapPtr`, `bgInitTileSet`, `dmaCopyVram`). Zero runtime cost
 * over the long-form code.
 *
 * @par Modules required
 * `dma` and `background` (transitively pulled in by `bgInitTileSet`
 * and `dmaCopyVram`). No new lib module — this header is pure
 * preprocessor.
 *
 * @see background.h, dma.h
 */

#ifndef SNES_ASSET_H
#define SNES_ASSET_H

#include <snes/types.h>
#include <snes/background.h>
#include <snes/dma.h>

/**
 * @brief A tileset bundle: graphics + palette + color depth.
 *
 * Use cases: runtime-built tilemaps, BGs that only ship a tileset
 * without a static map, or any "load tiles + palette" call where you
 * want the asset to be a first-class value (passed to a function,
 * stored in a level table, etc.).
 *
 * The struct stores begin/end pointer pairs rather than precomputed
 * sizes because pointer subtraction between two extern symbols is
 * not a compile-time constant in C — sizes are computed at load time
 * by `gfxLoad()`. This keeps the struct usable in `static const`
 * storage.
 *
 * Field stability: this struct is part of the public API. New fields
 * may be added at the END only — keep the first five fields where
 * they are. Callers SHOULD use designated initialisers.
 */
typedef struct {
    /** @brief Pointer to tile graphics data. */
    u8 *tiles;
    /** @brief One past the end of tile graphics. Size is computed at
     *         load time as `tiles_end - tiles`. */
    u8 *tiles_end;
    /** @brief Pointer to BGR555 palette data. */
    u8 *palette;
    /** @brief One past the end of the palette data. */
    u8 *palette_end;
    /**
     * @brief Color depth selector. Use one of:
     *        `BG_4COLORS`, `BG_4COLORS0` (Mode 0), `BG_16COLORS`,
     *        `BG_256COLORS`. Determines how the runtime computes the
     *        CGRAM destination from `palette_slot`.
     */
    u16 color_mode;
} GfxAsset;

/**
 * @brief A full background bundle: tileset + tilemap + map size.
 *
 * Wraps a `GfxAsset` with a tilemap blob and the BG layout constant
 * (`SC_32x32`, `SC_64x32`, `SC_32x64`, `SC_64x64`).
 */
typedef struct {
    /** @brief Tileset (tiles + palette + color mode). */
    GfxAsset gfx;
    /** @brief Pointer to tilemap data (gfx4snes `.map`). */
    u8 *tilemap;
    /** @brief One past the end of the tilemap data. */
    u8 *tilemap_end;
    /**
     * @brief Tilemap layout. Use one of `SC_32x32`, `SC_64x32`,
     *        `SC_32x64`, `SC_64x64`.
     */
    u8 map_size;
} BgAsset;

/**
 * @brief Load a tileset (tiles + palette) and configure a BG's tile
 *        graphics pointer — typed-value variant of `GFX_LOAD`.
 *
 * @param bg            Background number 0-3.
 * @param asset         Bundle to load. Must be non-NULL.
 * @param palette_slot  Palette slot index (0-7 for 16-color, etc.).
 * @param tiles_vram    VRAM word address for the tile graphics
 *                      (4 KB aligned).
 */
void gfxLoad(u8 bg, const GfxAsset *asset, u8 palette_slot, u16 tiles_vram);

/**
 * @brief Load a full background bundle — typed-value variant of `BG_LOAD`.
 *
 * Composes `gfxLoad()` with `bgSetMapPtr()` and a `dmaCopyVram()` of
 * the tilemap. After the call the BG is fully addressable; the caller
 * still controls when to enable it on the screen.
 *
 * @param bg            Background number 0-3.
 * @param asset         Bundle to load. Must be non-NULL.
 * @param palette_slot  Palette slot index (see `gfxLoad`).
 * @param tiles_vram    VRAM word address for the tile graphics.
 * @param map_vram      VRAM word address for the tilemap.
 */
void bgLoad(u8 bg, const BgAsset *asset, u8 palette_slot,
            u16 tiles_vram, u16 map_vram);

/**
 * @brief Declare a `static const GfxAsset` from gfx4snes-style symbol pairs.
 *
 * Expands to extern declarations for `<name>_tiles`/`<name>_tiles_end`
 * and `<name>_pal`/`<name>_pal_end`, plus a `static const GfxAsset`
 * populated with the right pointers and color mode.
 *
 * @code{.c}
 *     DECLARE_GFX_ASSET(player, BG_16COLORS);
 *     gfxLoad(0, &player, 0, 0x4000);
 * @endcode
 */
#define DECLARE_GFX_ASSET(name, color_mode_)                                  \
    extern u8 name##_tiles[], name##_tiles_end[];                             \
    extern u8 name##_pal[],   name##_pal_end[];                               \
    static const GfxAsset name = {                                            \
        .tiles       = name##_tiles,                                          \
        .tiles_end   = name##_tiles_end,                                      \
        .palette     = name##_pal,                                            \
        .palette_end = name##_pal_end,                                        \
        .color_mode  = (color_mode_),                                         \
    }

/**
 * @brief Declare a `static const BgAsset` from gfx4snes-style symbol pairs.
 *
 * Same convention as `DECLARE_GFX_ASSET`, plus `<name>_map` /
 * `<name>_map_end` for the tilemap blob.
 *
 * @code{.c}
 *     DECLARE_BG_ASSET(scene, BG_16COLORS, SC_32x32);
 *     bgLoad(0, &scene, 0, 0x4000, 0x0000);
 * @endcode
 */
#define DECLARE_BG_ASSET(name, color_mode_, map_size_)                        \
    extern u8 name##_tiles[], name##_tiles_end[];                             \
    extern u8 name##_pal[],   name##_pal_end[];                               \
    extern u8 name##_map[],   name##_map_end[];                               \
    static const BgAsset name = {                                             \
        .gfx = {                                                              \
            .tiles       = name##_tiles,                                      \
            .tiles_end   = name##_tiles_end,                                  \
            .palette     = name##_pal,                                        \
            .palette_end = name##_pal_end,                                    \
            .color_mode  = (color_mode_),                                     \
        },                                                                    \
        .tilemap     = name##_map,                                            \
        .tilemap_end = name##_map_end,                                        \
        .map_size    = (map_size_),                                           \
    }

/**
 * @brief Load a tileset (tiles + palette) and configure a BG's tile
 *        graphics pointer.
 *
 * Expands to two `extern u8` declarations for the symbol pair
 * `<name>_tiles` / `<name>_tiles_end`, `<name>_pal` / `<name>_pal_end`,
 * and a single call to `bgInitTileSet`. Use this when the BG's
 * tilemap is built at runtime (dynamic streaming, programmatically
 * generated maps) and only the tileset is shipped as data.
 *
 * Caller still owns: force-blank around the call (or being inside
 * VBlank), `setMode()`, `setMainScreen()`, `setScreenOn()`, the
 * BG's tilemap configuration if any.
 *
 * @code{.c}
 *     // data.asm exports player_tiles / player_tiles_end /
 *     //                  player_pal   / player_pal_end
 *     GFX_LOAD(player, 0, 0, BG_16COLORS, 0x4000);
 * @endcode
 *
 * @param name           Symbol-prefix root (must be a valid C identifier).
 * @param bg_            Background number 0-3.
 * @param palette_slot_  Palette slot index (0-7 for 16-color, etc.).
 * @param color_mode_    `BG_4COLORS` / `BG_16COLORS` / `BG_256COLORS`
 *                       / `BG_4COLORS0` (Mode 0).
 * @param tiles_vram_    VRAM word address for the tile graphics
 *                       (4 KB aligned).
 */
#define GFX_LOAD(name, bg_, palette_slot_, color_mode_, tiles_vram_)         \
    do {                                                                      \
        extern u8 name##_tiles[], name##_tiles_end[];                         \
        extern u8 name##_pal[],   name##_pal_end[];                           \
        bgInitTileSet((bg_), name##_tiles, name##_pal, (palette_slot_),       \
                      (u16)(name##_tiles_end - name##_tiles),                 \
                      (u16)(name##_pal_end   - name##_pal),                   \
                      (color_mode_), (tiles_vram_));                          \
    } while (0)

/**
 * @brief Load a full background bundle (tiles + palette + tilemap)
 *        and configure all BG pointers.
 *
 * Expands to six `extern u8` declarations for the symbol pairs
 * `<name>_tiles`, `<name>_pal`, `<name>_map` (each with an `_end`
 * sibling), then `bgSetMapPtr` + `bgInitTileSet` + `dmaCopyVram` —
 * the canonical "show this static background" sequence in one line.
 *
 * @code{.c}
 *     // data.asm exports bg_tiles / bg_pal / bg_map (with _end labels)
 *     BG_LOAD(bg, 0, 0, BG_16COLORS, SC_32x32, 0x4000, 0x0000);
 * @endcode
 *
 * @param name           Symbol-prefix root (must be a valid C identifier).
 * @param bg_            Background number 0-3.
 * @param palette_slot_  Palette slot index.
 * @param color_mode_    Color depth (`BG_*COLORS*`).
 * @param map_size_      `SC_32x32` / `SC_64x32` / `SC_32x64` / `SC_64x64`.
 * @param tiles_vram_    VRAM word address for the tile graphics
 *                       (4 KB aligned).
 * @param map_vram_      VRAM word address for the tilemap (1 KB aligned).
 */
#define BG_LOAD(name, bg_, palette_slot_, color_mode_, map_size_,            \
                tiles_vram_, map_vram_)                                       \
    do {                                                                      \
        extern u8 name##_tiles[], name##_tiles_end[];                         \
        extern u8 name##_pal[],   name##_pal_end[];                           \
        extern u8 name##_map[],   name##_map_end[];                           \
        bgSetMapPtr((bg_), (map_vram_), (map_size_));                         \
        bgInitTileSet((bg_), name##_tiles, name##_pal, (palette_slot_),       \
                      (u16)(name##_tiles_end - name##_tiles),                 \
                      (u16)(name##_pal_end   - name##_pal),                   \
                      (color_mode_), (tiles_vram_));                          \
        dmaCopyVram(name##_map, (map_vram_),                                  \
                    (u16)(name##_map_end - name##_map));                      \
    } while (0)

#endif /* SNES_ASSET_H */
