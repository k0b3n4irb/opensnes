/**
 * @file asset.c
 * @brief Implementation of gfxLoad() / bgLoad() — the typed-value
 *        variant of the asset bundle convention. See asset.h.
 *
 * Both functions are thin composers on top of `bgInitTileSet`,
 * `bgSetMapPtr`, and `dmaCopyVram`. There is no new DMA path and
 * no new state — the only thing this module owns is the typed
 * struct shape that lets callers stop juggling six externs and
 * eight positional parameters per scene.
 *
 * The DMA helpers take non-const `u8 *` for historical reasons; the
 * public Asset API keeps `const` pointers so users can store assets
 * in `static const` structs. The const cast is contained to this
 * file and is safe because the underlying DMA only reads the source
 * bytes.
 */

#include <snes.h>
#include <snes/asset.h>
#include <snes/background.h>
#include <snes/dma.h>

void gfxLoad(u8 bg, const GfxAsset *asset, u8 palette_slot, u16 tiles_vram) {
    bgInitTileSet(bg,
                  asset->tiles,
                  asset->palette,
                  palette_slot,
                  (u16)(asset->tiles_end   - asset->tiles),
                  (u16)(asset->palette_end - asset->palette),
                  asset->color_mode,
                  tiles_vram);
}

void bgLoad(u8 bg, const BgAsset *asset, u8 palette_slot,
            u16 tiles_vram, u16 map_vram) {
    bgSetMapPtr(bg, map_vram, asset->map_size);
    gfxLoad(bg, &asset->gfx, palette_slot, tiles_vram);
    dmaCopyVram(asset->tilemap, map_vram,
                (u16)(asset->tilemap_end - asset->tilemap));
}
