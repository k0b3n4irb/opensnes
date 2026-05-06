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
#include "sprite_dynamic_internal.h"

/* Forward declarations for the internal ASM entry points behind
 * `oamDynamicInit` and `oamDynamicDraw`. Removed from the public header
 * in B.6; user code goes through the struct-based init and the
 * size-aware dispatcher exposed below. */
extern void oamInitDynamicSprite(u16 gfxsp0adr, u16 gfxsp1adr,
                                 u16 oamsp0init, u16 oamsp1init, u8 oamsize);
extern void oamInitDynamicSpriteEndFrame(void);
extern void oamDynamic8Draw(u16 id);
extern void oamDynamic16Draw(u16 id);
extern void oamDynamic32Draw(u16 id);

/* Counter of bytes currently in the VRAM tile upload queue (declared in
 * templates/crt0.asm). Each queued entry is 6 bytes. The NMI auto-flush
 * hook drains up to MAXSPRTRF=42 (= 7 entries) per VBlank. */
extern volatile u16 oamqueuenumber;

/* Init-time queue drain flag (declared in templates/crt0.asm). When
 * non-zero, oamDynamicNmiFlush skips end-of-frame "hide stale sprites"
 * to let oamDynamicDrainQueue wait for queue empty without erasing the
 * very sprites the application just queued. */
extern u8 oam_dynamic_draining;

/**
 * Global dynamic-sprite size pair (0..5, OBJ_SIZE_*).
 *
 * Written by `oamInitDynamicSprite` (see sprite_dynamic.asm) at engine
 * init. Read by `oamDynamicDraw` to compute the fallback pixel size
 * when no per-sprite override is set. Defaults to 0 = OBJ_SIZE8_L16.
 */
u8 oam_dynamic_size_mode;

/* MODE_LARGE_SIZE / MODE_SMALL_SIZE come from sprite_dynamic_internal.h. */

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

void oamDynamicDrainQueue(void) {
    /* Tell the NMI auto-flush hook to skip the end-of-frame hide step
     * for the duration of the wait — see oamDynamicNmiFlush in
     * sprite_dynamic.asm and the comment block on oam_dynamic_draining
     * in templates/crt0.asm for why this is needed. */
    oam_dynamic_draining = 1;
    while (oamqueuenumber != 0) {
        WaitForVBlank();
    }
    oam_dynamic_draining = 0;

    /* Run end-frame once now that the queue is fully flushed. This
     * resets the per-frame slot allocators (oamnumberspr0/1) and
     * snapshots oamnumberperframe into oamnumberperframeold so the
     * NEXT main-loop draw allocates from the start of the slot pool
     * — without it, the slot counters keep growing and subsequent
     * sub-sprites are placed in VRAM regions that were never
     * uploaded, producing visible garbage tiles ("window of noise"
     * around metasprites after a configuration change). */
    oamInitDynamicSpriteEndFrame();
}

void oamDynamicDraw(u16 id) {
    u8 sz;

    if (id >= MAX_SPRITES) return;

    sz = oam_dyn_sprite_size[id];
    if (sz == 0) {
        /* Fallback: large pixel size of the size pair set at init. */
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
