/**
 * @file anim.c
 * @brief Declarative animation player implementation
 *
 * Pure C, no globals: every function operates on a caller-owned
 * AnimPlayer. See anim.h for the layout contract and cost notes.
 *
 * License: CC0 (Public Domain)
 */

#include "snes/anim.h"

/* Ticks for frame `idx` of `clip`: per-frame table wins, else the uniform
 * speed; 0 is clamped to 1 so a zero never stalls the countdown. */
static u8 frame_ticks(const AnimClip *clip, u8 idx) {
    u8 t;
    if (clip->durations) {
        t = clip->durations[idx];
    } else {
        t = clip->speed;
    }
    if (t == 0) t = 1;
    return t;
}

void animPlay(AnimPlayer *p, const AnimClip *clip) {
    if (clip == 0 || clip->len == 0) {
        animStop(p);
        return;
    }
    if (clip == p->clip && !(p->flags & ANIM_F_FINISHED)) {
        return;  /* continue-if-same: the animation keeps running */
    }
    /* new clip, or re-trigger of a finished ANIM_ONCE clip */
    p->clip = clip;
    p->frame = 0;
    p->ticks = frame_ticks(clip, 0);
    p->flags = 0;
}

void animRestart(AnimPlayer *p) {
    if (p->clip == 0) return;
    p->frame = 0;
    p->ticks = frame_ticks(p->clip, 0);
    p->flags = 0;
}

u16 animTick(AnimPlayer *p) {
    const AnimClip *clip = p->clip;
    u16 out;
    u8 next;

    if (clip == 0) {
        return ANIM_NONE;
    }

    /* Return the frame belonging to THIS tick, then advance — every frame
     * is visible exactly `duration` ticks, including the first one and
     * including duration-1 frames. */
    out = clip->frames[p->frame];

    if (p->flags & ANIM_F_FINISHED) {
        return out;  /* ANIM_ONCE holds its last frame */
    }

    /* Single load/store via a local. The natural
     * `p->ticks--; if (p->ticks == 0)` form is now correct too — the
     * cc65816 miscompile that motivated this (post-store re-read through an
     * address reloaded in 8-bit accumulator mode) was fixed in qbe w65816
     * (opensnes#99, pinned by the r_rmw_u8 vector in devtools/libtests and
     * the test_rmw_ptr_reread codegen check). This form is kept as-is to
     * avoid rebuilding every anim-linked ROM for a no-op; it is equally
     * correct and one load/store cheaper. */
    {
        u8 t = p->ticks - 1;
        p->ticks = t;
        if (t != 0) {
            return out;
        }
    }
    {
        next = p->frame + 1;
        if (next >= clip->len) {
            if (clip->mode == ANIM_ONCE) {
                /* hold the last frame; ticks stays 0, flag gates the path */
                p->flags |= ANIM_F_FINISHED;
                return out;
            }
            next = 0;  /* ANIM_LOOP wraps */
        }
        p->frame = next;
        p->ticks = frame_ticks(clip, next);
    }
    return out;
}
