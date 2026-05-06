/**
 * @file scene.h
 * @brief Opt-in scene/state stack — push/pop game scenes without
 *        rolling your own state machine.
 * @ingroup framework
 *
 * Third "framework opt-in" from `PHILOSOPHY.md` (alongside `gameloop`
 * D.1 and the asset bundle convention D.2). This module gives you a
 * tiny stack of `Scene` callbacks: the topmost scene's `update` runs
 * every VBlank; pushing a new scene suspends the current one and runs
 * the new scene's `init` + `update`; popping restores the suspended
 * scene without re-running its init.
 *
 * @par Before
 * @code{.c}
 *     enum State { TITLE, PLAY, PAUSE };
 *     enum State current = TITLE;
 *     enum State suspend_stack[4]; int suspend_top = 0;
 *
 *     while (1) {
 *         WaitForVBlank();
 *         switch (current) {
 *             case TITLE: title_update(); break;
 *             case PLAY:  play_update();  break;
 *             case PAUSE: pause_update(); break;
 *         }
 *     }
 * @endcode
 *
 * @par After
 * @code{.c}
 *     static const Scene title = { .init = title_init, .update = title_update };
 *     static const Scene play  = { .init = play_init,  .update = play_update  };
 *     static const Scene pause = { .init = pause_init, .update = pause_update };
 *     // somewhere in title_update: scenePush(&play);
 *     // somewhere in play_update:  scenePush(&pause);
 *     // somewhere in pause_update: scenePop();
 *     int main(void) {
 *         sceneRun(&title);
 *         return 0;  // never reached
 *     }
 * @endcode
 *
 * @par Behaviour
 * - `init` runs once on the first push of a scene. It does NOT re-run
 *   on resume after `scenePop`. If a scene needs "did I just resume?"
 *   logic, it owns that bookkeeping in its own state.
 * - `update` runs every VBlank while the scene is on top of the stack.
 *   Suspended scenes (below the top) get no callbacks.
 * - The stack has a fixed capacity (`SCENE_STACK_MAX = 8`). Push beyond
 *   that is a silent no-op — the new scene is dropped, the active
 *   scene continues. Pop on a stack of size 1 is also a silent no-op:
 *   the bottom scene stays active (the stack is never empty after
 *   `sceneRun` is called).
 * - `scenePush` does not run the new scene's `update` synchronously.
 *   The currently-executing `update` finishes its frame; the next
 *   VBlank dispatches to the new top.
 *
 * @par What this module does NOT do
 * - No transitions (fade-in/out, slide). The caller renders those.
 * - No `resume`/`cleanup` callbacks. Most scenes don't need them; if
 *   yours does, the first 1-2 frames of `update` post-pop can do the
 *   work.
 * - No serialization, save-state, or scope substacks. A scene is just
 *   a pair of function pointers; you keep the data.
 * - No NMI integration. Pushing/popping from inside an NMI callback
 *   is undefined; do it from `update` only.
 *
 * @par Performance
 * `sceneRun` adds one indirect call per frame (the top scene's
 * `update`) and one direct call (`WaitForVBlank`) — same shape as
 * `gameLoopRun`. `scenePush` does an array bounds check, an index
 * update, and an optional indirect call to `init`. `scenePop` is a
 * single index decrement. RAM footprint: 8 pointers × 8 bytes per
 * cproc ABI = 64 bytes BSS plus a single byte for the stack depth.
 *
 * @par Modules required
 * `scene` (and `gameloop`'s transitive `WaitForVBlank` from the
 * runtime). No other lib module is pulled in by sceneRun itself.
 *
 * @par Relationship with `gameloop` (D.1)
 * `GameLoopConfig` from `<snes/gameloop.h>` is an alias for `Scene`.
 * The two types are interchangeable; `gameLoopRun(&cfg)` is
 * semantically equivalent to running a single-scene stack with
 * `sceneRun(&cfg)`. Pick whichever name fits the use site:
 * `GameLoopConfig` for "this is just a main loop", `Scene` for "this
 * is one of several swappable screens".
 *
 * @see gameloop.h, system.h (WaitForVBlank, frame_count)
 */

#ifndef SNES_SCENE_H
#define SNES_SCENE_H

#include <snes/types.h>

/**
 * @brief Maximum scene stack depth.
 *
 * `scenePush` beyond this depth is a silent no-op. 8 is enough for
 * realistic SNES game shapes (title → menu → play → pause → over
 * leaves 3 spare slots).
 */
#define SCENE_STACK_MAX 8

/**
 * @brief A single scene's callbacks.
 *
 * Field stability: this struct is part of the public API. New fields
 * may be added at the END only — designated initialisers stay
 * compatible. Position-based initialisation is therefore discouraged.
 */
typedef struct {
    /**
     * @brief Called once when the scene is first pushed onto the stack.
     *
     * NOT called again when the scene is resumed after a pop. May be
     * NULL to skip. A common shape is to load tilesets / palettes /
     * configure backgrounds in `init` so the work happens in force
     * blank or before `setScreenOn`.
     */
    void (*init)(void);

    /**
     * @brief Called every VBlank while the scene is on top of the stack.
     *
     * MUST NOT be NULL. The caller may invoke `scenePush` or
     * `scenePop` from inside `update`; the change takes effect on the
     * next VBlank dispatch.
     */
    void (*update)(void);
} Scene;

/**
 * @brief Run the scene stack with `initial` at the bottom. Never returns.
 *
 * Pushes `initial`, calls its `init` (if non-NULL), and enters the
 * dispatch loop:
 * @code{.c}
 *     while (1) { WaitForVBlank(); top->update(); }
 * @endcode
 *
 * @param initial First scene. Must be non-NULL and have a non-NULL
 *                `update`. The pointer is held for the lifetime of the
 *                scene's stack residency, so the caller MUST keep the
 *                Scene struct alive for at least that long
 *                (typically: place it in `static const`).
 */
void sceneRun(const Scene *initial);

/**
 * @brief Push a scene onto the stack. Suspends the current top.
 *
 * Calls `next->init` if non-NULL. The caller's currently-executing
 * `update` finishes its frame; the next VBlank dispatches to `next`.
 *
 * Silently ignored when the stack is already at `SCENE_STACK_MAX`.
 *
 * @param next Scene to push. Same lifetime contract as `sceneRun`'s
 *             `initial`.
 */
void scenePush(const Scene *next);

/**
 * @brief Pop the top scene. Resumes the scene below.
 *
 * The popped scene gets no callbacks; the resumed scene's `update`
 * runs on the next VBlank. The resumed scene's `init` is NOT
 * re-invoked — it ran once at first push.
 *
 * Silently ignored when the stack contains only the bottom scene
 * (i.e. depth 1) — the stack is never empty after `sceneRun` is
 * called.
 */
void scenePop(void);

#endif /* SNES_SCENE_H */
