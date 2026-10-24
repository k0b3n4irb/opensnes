/**
 * @file main.c
 * @brief Scene stack demo — title → counter → pause overlay
 * @ingroup examples
 *
 * Demonstrates the opt-in `scene` framework: the running game is split
 * into three Scene structs that push and pop without a hand-rolled
 * state machine. The demo is deliberately small — three screens that
 * swap when buttons are pressed — so you can read the dispatch shape. It
 * uses the pictures of basics/game_skeleton (the logo, the play square),
 * so the two ways of structuring a game can be compared side by side.
 *
 * Flow:
 *   1. `title` scene: the logo and "PRESS START". Start pushes `counter`.
 *   2. `counter` scene: the square, and a number that increments every
 *      frame. Select pushes `pause` (counter is suspended; numbers stop).
 *      Start pops back to title.
 *   3. `pause` scene: shows "PAUSED" and darkens the square beneath it
 *      (colour math): an overlay, the scene below still drawn but frozen.
 *      Select pops back to counter (numbers resume from where they
 *      stopped, no init replay).
 *
 * @par SNES Concepts
 * - Scene stack: push/pop without enum + switch boilerplate
 * - `init` runs once on first push; resume after pop does not re-init
 * - Suspended scenes get no callbacks (pause overlay = counter frozen)
 * - Text rendering through the auto-flush NMI hook, on BG3 in front
 * - Each scene picks its layers (setMainScreen) when it is entered or resumed
 * - A pause overlay by colour math: colorMathShadow() darkens BG1
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
 * console, sprite, dma, background, text, input, scene, colormath
 *
 * @see scene.h, gameloop.h
 */

#include <snes.h>
#include <snes/text.h>
#include <snes/scene.h>
#include <snes/colormath.h>

#include "res/field.inc"
#include "res/logo.inc"
/* generated: field_, logo_ tiles/_pal/_map (each with _end) */

/** @name VRAM (word addresses), as in basics/game_skeleton
 *  @{ */
#define VRAM_BG1_MAP  0x0000   /**< the square */
#define VRAM_BG2_MAP  0x0400   /**< the logo */
#define VRAM_BG3_MAP  0x0800   /**< the text */
#define VRAM_BG1_GFX  0x1000
#define VRAM_BG2_GFX  0x2000
#define VRAM_BG3_FONT 0x3000
/** @} */

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

/** @brief Set by the counter before it pops: the title is resumed, not
 *  re-initialised (scene.h: `init` runs once per push), so it redraws
 *  itself here. The luna manifest counts the redraws. */
static u8 title_resume_pending;
u16 title_redraws;

static void title_draw(void) {
    textClear();
    setMainScreen(LAYER_BG2 | LAYER_BG3);   /* the logo and the text */
    textPrintAt(10, 2, "SCENE STACK");
    textPrintAt(10, 25, "PRESS START");
}

/**
 * @brief The root scene's init: load everything once, then draw the title
 *
 * The title is pushed once, by sceneRun(), so its init is where the
 * program sets up the hardware — the same layers as basics/game_skeleton:
 * Mode 1, BG1 the square, BG2 the logo, BG3 the text in front.
 */
static void title_init(void) {
    consoleInit();
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);

    bgInitTileSet(0, field_tiles, field_pal, 0, field_tiles_end - field_tiles,
                  field_pal_end - field_pal, BG_16COLORS, VRAM_BG1_GFX);
    bgInitTileSet(1, logo_tiles, logo_pal, 1, logo_tiles_end - logo_tiles,
                  logo_pal_end - logo_pal, BG_16COLORS, VRAM_BG2_GFX);
    bgSetMapPtr(0, VRAM_BG1_MAP, SC_32x32);
    bgSetMapPtr(1, VRAM_BG2_MAP, SC_32x32);
    dmaCopyVram(field_map, VRAM_BG1_MAP, field_map_end - field_map);
    dmaCopyVram(logo_map, VRAM_BG2_MAP, logo_map_end - logo_map);
    setColor(0, RGB(2, 3, 5));      /* the backdrop: the night behind the logo */

    textLoadFont(VRAM_BG3_FONT);
    textInit(VRAM_BG3_MAP, 0, 7);
    text_config.priority = 1;
    setColor(29, RGB(31, 31, 31));  /* slot 7, colour 1: white */
    bgSetGfxPtr(2, VRAM_BG3_FONT);
    bgSetMapPtr(2, VRAM_BG3_MAP, SC_32x32);

    title_draw();
    WaitForVBlank();
    setScreenOn();
}

static void title_update(void) {
    if (title_resume_pending) {
        /* Back from the counter: its screen is still on the layer (until
         * 2026-10-03 the title stayed hidden behind it). */
        title_resume_pending = 0;
        title_redraws++;
        title_draw();
    }
    if (padPressed(0) & KEY_START)
        scenePush(&counter_scene);
}

/*============================================================================
 * Counter scene
 *============================================================================*/

static void counter_init(void) {
    counter_value = 0;
    textClear();
    setMainScreen(LAYER_BG1 | LAYER_BG3);   /* the square and the text */
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
        title_resume_pending = 1;   /* the title redraws on its first update */
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
    /* The counter's screen stays on the layers, frozen: darken its square
     * (BG1 only, so the text stays bright) to say it is behind the pause */
    colorMathShadow(LAYER_BG1, 8);
}

static void pause_update(void) {
    if (padPressed(0) & KEY_SELECT) {
        /* Erase the PAUSED label so the resumed counter screen is clean. */
        textPrintAt(13, 18, "      ");
        colorMathDisable();
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
