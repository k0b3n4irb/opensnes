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
 * @par Why macros, not a typed `BgAsset` struct
 *
 * The natural design here would be a `static const BgAsset bg = {...}`
 * holding pointer fields, with a `bgLoad(&bg, ...)` runtime function.
 * That design is currently broken by a cproc/QBE/WLA-DX integration
 * bug: cproc lays out struct pointer fields with an 8-byte stride,
 * but WLA-DX's `.dl <symbol>` directive (used by QBE for static
 * pointer initialisers) emits only 3 bytes for a 24-bit address. The
 * resulting ROM layout shifts every field after the first pointer,
 * and every read at runtime returns garbage.
 *
 * Until that toolchain bug is fixed (separate compiler chantier), the
 * asset bundle ships as a header-only macro. The call-site ergonomics
 * are unchanged; only the "asset as a first-class value you can pass
 * to functions or store in a level table" use case is deferred.
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
