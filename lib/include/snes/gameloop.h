/**
 * @file gameloop.h
 * @brief Opt-in game loop framework — write your update, the engine
 *        runs the VBlank synchronisation.
 * @ingroup framework
 *
 * One of the three "framework opt-ins" promised by `PHILOSOPHY.md`
 * (alongside `scene_2d` and the asset bundle convention). This module
 * exists to remove a single repetitive piece of SNES boilerplate:
 *
 * @code{.c}
 *     while (1) {
 *         WaitForVBlank();
 *         // ... your update logic
 *     }
 * @endcode
 *
 * If that loop is the only thing standing between your `main()` and
 * the work you actually want to write, drop in the framework:
 *
 * @code{.c}
 *     static void onInit(void) {
 *         consoleInit();
 *         setMode(BG_MODE0, 0);
 *         // ... rest of your hardware init
 *         setScreenOn();
 *     }
 *
 *     static void onUpdate(void) {
 *         u16 pad = padPressed(0);
 *         // ... your per-frame logic
 *     }
 *
 *     int main(void) {
 *         GameLoopConfig cfg = { .init = onInit, .update = onUpdate };
 *         gameLoopRun(&cfg);
 *         return 0;   // never reached — gameLoopRun never returns
 *     }
 * @endcode
 *
 * The framework deliberately does very little. It does NOT call
 * `consoleInit()` or `setScreenOn()` for you, NOT touch palettes, NOT
 * configure the NMI handler. All of those remain caller-driven so the
 * ABI stays the same as a hand-rolled main loop and any example you
 * already have keeps compiling. What the framework owns is exactly:
 * the `while (1) WaitForVBlank(); update();` cadence.
 *
 * @par When NOT to use this
 *
 * - You need a state machine that swaps the per-frame logic between
 *   distinct screens (title, gameplay, pause, game-over). For now,
 *   either dispatch from inside `update` or wait for the
 *   `scene_2d` / scene-stack module that's planned next.
 * - You need a custom synchronisation rhythm (e.g. half-rate updates,
 *   double-buffered audio polling). Keep your hand-rolled loop —
 *   gameLoopRun is meant for the common case, not every case.
 *
 * @par Performance
 *
 * Each frame, gameLoopRun adds one indirect call (`update` via the
 * `cfg` pointer) and one direct call (`WaitForVBlank`). On the order
 * of 30 cycles per frame, dwarfed by anything `update` itself does.
 * The compiler does not currently TCO the indirect call (chantier C
 * accepted only direct tail calls); a future C.3 may close that gap.
 *
 * @par Modules required
 *
 * `gameloop` (and its transitive dependency on the runtime). No other
 * lib module is pulled in by gameLoopRun itself — what your `init` and
 * `update` callbacks use is on you.
 *
 * @see system.h (WaitForVBlank, frame_count)
 */

#ifndef SNES_GAMELOOP_H
#define SNES_GAMELOOP_H

#include <snes/types.h>

/**
 * @brief Game loop callbacks.
 *
 * Pass to gameLoopRun() to opt into the framework. `init` is optional
 * (set to NULL to skip); `update` is required and must not be NULL.
 *
 * Field stability: this struct is part of the public API. New fields
 * may be added in future releases at the END of the struct only —
 * existing code that initialises by name (designated initialisers)
 * will keep compiling. Position-based initialisation is therefore
 * discouraged.
 */
typedef struct {
    /**
     * @brief Called once before the loop starts. May be NULL.
     *
     * Run the standard init sequence here:
     * `consoleInit() → setMode() → palettes → tiles → tilemap → BG
     * pointers → sprites/text setup → setMainScreen() → setScreenOn()`.
     * The framework does not enforce the order — it just calls your
     * function with the SNES already booted by `crt0.asm` (NMI on,
     * registers cleared, BSS zeroed). You own everything else.
     */
    void (*init)(void);

    /**
     * @brief Called once per VBlank, after the framework's
     *        WaitForVBlank() returns. MUST NOT be NULL.
     *
     * The PPU is in VBlank when this runs (first ~33K cycles of the
     * call). VRAM, OAM, CGRAM writes are safe at the start. As soon
     * as you exceed the VBlank budget, further VRAM writes silently
     * fail — the standard SNES rule, the framework does not change
     * it.
     *
     * If this function takes longer than one frame's cycle budget,
     * the next WaitForVBlank() simply waits one more frame, dropping
     * the effective frame rate to 30 Hz / 25 Hz. The framework does
     * not detect or report the overrun; if you need that, time
     * yourself with `frame_count` snapshots.
     */
    void (*update)(void);
} GameLoopConfig;

/**
 * @brief Run the OpenSNES game loop. Never returns.
 *
 * Equivalent to:
 * @code{.c}
 *     if (cfg->init)  cfg->init();
 *     while (1) {
 *         WaitForVBlank();
 *         cfg->update();
 *     }
 * @endcode
 *
 * @param cfg Pointer to a GameLoopConfig. Must be non-NULL and must
 *            have a non-NULL `update`. `init` may be NULL to skip the
 *            init phase. The pointer is only consulted at the start
 *            of the loop and on each iteration's update; you may
 *            allocate `cfg` on the stack of `main()` since `main` does
 *            not return for as long as gameLoopRun runs.
 */
void gameLoopRun(const GameLoopConfig *cfg);

#endif /* SNES_GAMELOOP_H */
