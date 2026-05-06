/**
 * @file main.c
 * @brief Scene stack demo — title → counter → pause overlay
 * @ingroup examples
 *
 * Demonstrates the opt-in `scene` framework: the running game is split
 * into three Scene structs that push and pop without a hand-rolled
 * state machine. The demo is deliberately minimal — three text screens
 * that swap when buttons are pressed — so you can read the dispatch
 * shape without graphics noise getting in the way.
 *
 * Flow:
 *   1. `title` scene: shows "PRESS START". Start pushes `counter`.
 *   2. `counter` scene: increments a number every frame. Select pushes
 *      `pause` (counter is suspended; numbers stop). Start pops back
 *      to title.
 *   3. `pause` scene: shows "PAUSED". Select pops back to counter
 *      (numbers resume from where they stopped, no init replay).
 *
 * @par SNES Concepts
 * - Scene stack: push/pop without enum + switch boilerplate
 * - `init` runs once on first push; resume after pop does not re-init
 * - Suspended scenes get no callbacks (pause overlay = counter frozen)
 * - Text rendering through the auto-flush NMI hook
 *
 * @par What to Observe
 * - **Start** on title: enters counter, numbers increment from 0
 * - **Select** while counting: PAUSED appears, numbers freeze
 * - **Select** in pause: counter resumes, numbers continue from where
 *   they stopped (no reset — `init` was not re-called)
 * - **Start** while counting: pops back to title (counter discarded;
 *   pressing Start again restarts from 0 because counter's `init`
 *   re-runs on the next push)
 *
 * @par Modules Used
 * console, sprite, dma, background, text, input, scene
 *
 * @see scene.h, gameloop.h
 */

#include <snes.h>
#include <snes/text.h>
#include <snes/scene.h>

/* Forward declarations so the Scene structs can name each other. */
static void title_init(void);
static void title_update(void);
static void counter_init(void);
static void counter_update(void);
static void pause_init(void);
static void pause_update(void);

/** @brief Title screen — waits for Start to begin, no game state. */
static const Scene title_scene = {
    .init   = title_init,
    .update = title_update,
};

/** @brief Counter screen — increments a number every frame. */
static const Scene counter_scene = {
    .init   = counter_init,
    .update = counter_update,
};

/** @brief Pause overlay — freezes the counter, waits for Select to resume. */
static const Scene pause_scene = {
    .init   = pause_init,
    .update = pause_update,
};

/** @brief Counter value, lives across pause/resume because counter's
 *         init runs once on push, not on every resume. */
static u16 counter_value;

/*============================================================================
 * Title scene
 *============================================================================*/

static void title_init(void) {
    textModeInit();
    textPrintAt(11, 12, "SCENE STACK");
    textPrintAt(11, 14, "PRESS START");
    WaitForVBlank();
    setScreenOn();
}

static void title_update(void) {
    if (padPressed(0) & KEY_START)
        scenePush(&counter_scene);
}

/*============================================================================
 * Counter scene
 *============================================================================*/

static void counter_init(void) {
    counter_value = 0;
    textClear();
    textPrintAt(8, 12, "COUNTER:");
    textPrintAt(7, 14, "SELECT=PAUSE");
    textPrintAt(7, 16, "START=TITLE");
}

static void counter_update(void) {
    u16 pad = padPressed(0);
    if (pad & KEY_SELECT) {
        scenePush(&pause_scene);
        return;
    }
    if (pad & KEY_START) {
        scenePop();        /* back to title; next push restarts at 0 */
        return;
    }
    counter_value++;
    textSetPos(17, 12);
    textPrintU16(counter_value);
}

/*============================================================================
 * Pause scene
 *============================================================================*/

static void pause_init(void) {
    textPrintAt(13, 18, "PAUSED");
}

static void pause_update(void) {
    if (padPressed(0) & KEY_SELECT) {
        /* Erase the PAUSED label so the resumed counter screen is clean. */
        textPrintAt(13, 18, "      ");
        scenePop();
    }
}

/*============================================================================
 * Entry point
 *============================================================================*/

/**
 * @brief Hand control to the scene framework. Never returns.
 */
int main(void) {
    sceneRun(&title_scene);
    return 0;   /* never reached — sceneRun loops forever */
}
