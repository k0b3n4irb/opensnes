/**
 * @file sprite_dynamic_dispatch.c
 * @brief Modern entry points for the dynamic sprite engine
 *
 * Hosts the size-aware dispatch glue (`oamDynamicDraw`,
 * `oamDynamicSetSize`) and the struct-based init wrapper
 * (`oamDynamicInit` over the legacy `oamInitDynamicSprite`).
 *
 * Lives in its own translation unit so `sprite.o` stays free of
 * dependencies on the dynamic engine — examples that don't use
 * dynamic sprites only link `sprite_oamset` + `sprite` and don't drag
 * `oamInitDynamicSprite` symbols. Examples that list `sprite_dynamic`
 * pull this file in via the dependency rule in make/common.mk.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>

/**
 * Global dynamic-sprite size pair (0..5, OBJ_SIZE_*).
 *
 * Written by `oamInitDynamicSprite` (see sprite_dynamic.asm) at engine
 * init. Read by `oamDynamicDraw` to compute the fallback pixel size
 * when no per-sprite override is set. Defaults to 0 = OBJ_SIZE8_L16.
 */
u8 oam_dynamic_size_mode;

/* Macro: pixel size of the "large" half of an OBJ_SIZE_* pair.
 *
 * Implemented as a macro (not a function) to dodge a cc65816 codegen issue
 * where a JSL inside the conditional fallback path of `oamDynamicDraw`
 * produced a stale A-register read on the subsequent `cmp #16`, silently
 * skipping the dispatch. Inlining keeps everything in registers / locals.
 */
#define MODE_LARGE_SIZE(mode) \
    ((u8)((mode) == 0 ? 16 : \
          (mode) == 1 ? 32 : \
          (mode) == 2 ? 64 : \
          (mode) == 3 ? 32 : \
          (mode) == 4 ? 64 : 64))

/**
 * Per-sprite explicit pixel size override for the dynamic engine.
 * Indexed by sprite id. 0 = "use mode default" (resolved at draw time
 * to the current mode's large size). Set via oamDynamicSetSize when a
 * sprite needs a different size from the mode default (e.g. mixing
 * 8x8 small and 16x16 large sprites under OBJ_SIZE8_L16).
 *
 * BSS-initialized to 0, so callers that never touch this still get the
 * mode-default — which matches what every existing dynamic example
 * needs (they all draw "large" sprites). oamDynamicInit also resets
 * this array so re-initing the engine clears stale per-sprite overrides.
 */
static u8 oam_dyn_sprite_size[MAX_SPRITES];

void oamDynamicInit(const OamDynamicConfig *cfg) {
    u8 i;

    oamInitDynamicSprite(cfg->vramLarge,
                         cfg->vramSmall,
                         cfg->slotLargeInit,
                         cfg->slotSmallInit,
                         cfg->sizeMode);

    /* Clear per-sprite size overrides — re-init implies a fresh dispatch
     * table. Sprites then default to the mode's large pixel size unless
     * the caller overrides via oamDynamicSetSize. */
    for (i = 0; i < MAX_SPRITES; i++) {
        oam_dyn_sprite_size[i] = 0;
    }
}

void oamDynamicSetSize(u16 id, u8 size) {
    if (id >= MAX_SPRITES) return;
    oam_dyn_sprite_size[id] = size;
}

void oamDynamicDraw(u16 id) {
    u8 sz;

    if (id >= MAX_SPRITES) return;

    sz = oam_dyn_sprite_size[id];
    if (sz == 0) {
        /* Fallback: large pixel size of the size pair set at init.
         * MODE_LARGE_SIZE is a macro, not a function — see its definition for
         * the cc65816 codegen reason. */
        sz = MODE_LARGE_SIZE(oam_dynamic_size_mode);
    }

    if (sz == 8) {
        oamDynamic8Draw(id);
    } else if (sz == 16) {
        oamDynamic16Draw(id);
    } else if (sz == 32) {
        oamDynamic32Draw(id);
    }
    /* sz == 64: 64x64 dynamic streaming is unsupported, silently skip. */
}
