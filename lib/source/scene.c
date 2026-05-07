/**
 * @file scene.c
 * @brief Implementation of sceneRun() / scenePush() / scenePop() —
 *        see scene.h for the contract.
 *
 * The state is two parallel arrays indexed by the stack depth:
 *   - `scene_stack[]`     — the Scene pointers themselves
 *   - `scene_init_done[]` — 1 byte per slot: 0 means "init still
 *     pending", 1 means "init has been (or doesn't need to be)
 *     called". Push sets the bit to 0 so the dispatcher runs
 *     init on the next VBlank with the full budget.
 *
 * The initial scene passed to `sceneRun` is the exception: its
 * `init` runs eagerly, before the first `WaitForVBlank`, matching
 * `gameLoopRun`'s pattern. This is required because the canonical
 * init body does `consoleInit() → setMode → palettes → setScreenOn`,
 * and the first NMI must see the configured state. Pushed scenes
 * arrive with the engine already booted, so deferring their init
 * by one frame is safe and gives the init function a full ~33K
 * cycles of VBlank budget.
 *
 * No allocation, no NMI hook, no global state outside this file.
 */

#include <snes.h>
#include <snes/scene.h>

static const Scene *scene_stack[SCENE_STACK_MAX];
static u8 scene_init_done[SCENE_STACK_MAX];
static u8 scene_top;

void sceneRun(const Scene *initial) {
    scene_stack[0] = initial;
    scene_init_done[0] = 1;
    scene_top = 1;
    if (initial->init)
        initial->init();
    while (1) {
        WaitForVBlank();
        /* Drain any pending inits before dispatching update. The
         * inner `while` handles the case where an init() pushes
         * more scenes — each new push lands at a higher slot with
         * scene_init_done = 0, so the loop iterates on the new
         * top until the stack stabilises. */
        while (!scene_init_done[scene_top - 1]) {
            u8 idx = scene_top - 1;
            /* Set BEFORE calling: if init() recurses (pop+push),
             * we don't want to re-enter this slot's init. */
            scene_init_done[idx] = 1;
            if (scene_stack[idx]->init)
                scene_stack[idx]->init();
        }
        scene_stack[scene_top - 1]->update();
    }
}

void scenePush(const Scene *next) {
    if (scene_top >= SCENE_STACK_MAX)
        return;
    scene_stack[scene_top] = next;
    scene_init_done[scene_top] = 0;
    scene_top++;
}

void scenePop(void) {
    if (scene_top <= 1)
        return;
    scene_top--;
    /* scene_init_done[scene_top] stays set; if the same slot is
     * re-used by a future scenePush, push resets it to 0. */
}
