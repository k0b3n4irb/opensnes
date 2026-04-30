/**
 * @file sprite_dynamic_meta.c
 * @brief Dynamic metasprite engine — C implementation
 *
 * One-stop metasprite iterator: walks a `MetaspriteItem` array, sets up
 * `oambuffer[id]` for each sub-sprite, and dispatches to the size-specific
 * dynamic draw routine via the engine's per-mode size resolution.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>

/* Internal entry points: removed from the public header in B.6 but kept
 * as the underlying ASM mechanism behind the unified iterator below. */
extern void oamDynamic8Draw(u16 id);
extern void oamDynamic16Draw(u16 id);
extern void oamDynamic32Draw(u16 id);

/* Owned by sprite_dynamic_dispatch.c — current size pair (OBJ_SIZE_*). */
extern u8 oam_dynamic_size_mode;

/* Inlined macros (not functions) to avoid the cc65816 "JSL inside an
 * `if` branch followed by `cmp` dispatch" codegen issue documented at
 * memory/cc65816_conditional_jsl_codegen_bug.md. */
#define MODE_LARGE_SIZE(mode) \
    ((u8)((mode) == 0 ? 16 : \
          (mode) == 1 ? 32 : \
          (mode) == 2 ? 64 : \
          (mode) == 3 ? 32 : \
          (mode) == 4 ? 64 : 64))

#define MODE_SMALL_SIZE(mode) \
    ((u8)((mode) <= 2 ? 8 : \
          (mode) <= 4 ? 16 : 32))

/**
 * @brief Walk a metasprite, set up oambuffer entries, dispatch dynamic draws.
 *
 * Replaces the trio `oamMetaDrawDyn{8,16,32}`. The pixel size of each
 * sub-sprite is resolved from the engine's current mode (set at init) plus
 * the caller-provided `size_class` (OBJ_SMALL / OBJ_LARGE):
 *
 *   - OBJ_SMALL → small half of the size pair (and OBJ_NAMETABLE_HIGH on
 *     the attribute byte so the tile id addresses the second name table).
 *   - OBJ_LARGE → large half of the size pair, no attribute override.
 *
 * @param id          First oambuffer slot (incremented per sub-sprite).
 * @param x,y         Origin in screen coordinates.
 * @param meta        Metasprite array, terminated by metasprite_end.
 * @param gfxptr      ROM source for the dynamic tile data.
 * @param size_class  OBJ_SMALL (0) or OBJ_LARGE (1).
 */
void oamMetaDrawDyn(u16 id, s16 x, s16 y,
                    const MetaspriteItem *meta, u8 *gfxptr, u8 size_class) {
    u8 refresh = oambuffer[id].oamrefresh;
    u8 pixel_size;
    u8 attr_or;

    if (size_class == OBJ_SMALL) {
        pixel_size = MODE_SMALL_SIZE(oam_dynamic_size_mode);
        attr_or = OBJ_NAMETABLE_HIGH;
    } else {
        pixel_size = MODE_LARGE_SIZE(oam_dynamic_size_mode);
        attr_or = 0;
    }

    while (meta->dx != metasprite_end) {
        oambuffer[id].oamx = x + meta->dx;
        oambuffer[id].oamy = y + meta->dy;
        oambuffer[id].oamframeid = meta->tile;
        oambuffer[id].oamattribute = meta->attr | attr_or;
        oambuffer[id].oamrefresh = refresh;
        OAM_SET_GFX(id, gfxptr);

        if (pixel_size == 8) {
            oamDynamic8Draw(id);
        } else if (pixel_size == 16) {
            oamDynamic16Draw(id);
        } else if (pixel_size == 32) {
            oamDynamic32Draw(id);
        }
        /* pixel_size == 64: 64x64 dynamic streaming unsupported, skip. */

        meta++;
        id++;
    }
}
