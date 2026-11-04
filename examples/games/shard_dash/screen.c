/**
 * @file screen.c
 * @brief The screen layer (see screen.h)
 * @ingroup examples
 */
#include <snes.h>
#include <snes/input.h>
#include <snes/vramqueue.h>
#include "screen.h"

/** @brief Frames between two brightness steps: 16 steps, so 16 images a fade. */
#define FADE_SPEED 1

/** @brief The function run on every image, or 0. */
static void (*pump_fn)(void);
/** @brief The hook of the screen on display, or 0. */
static void (*close_fn)(void);

u16 screen_fade_skip;
u16 screen_fade_frames;

/** @brief Pad words of this image and of the one before (the edges come from the pair). */
static u16 held0, held1, prev0, prev1;

void screenSetPump(void (*fn)(void)) { pump_fn = fn; }
void screenSetClose(void (*fn)(void)) { close_fn = fn; }
void screenFadeSkip(u16 on) { screen_fade_skip = on; }

/** @brief Run the pump. A rule of the layer: nothing waits without calling it. */
static void pump(void) {
    if (pump_fn) pump_fn();
}

/** @brief Read both pads; the old words become the "before". */
static void readPads(void) {
    prev0 = held0;
    prev1 = held1;
    held0 = padHeld(0);
    held1 = padHeld(1);
}

/** @brief Take the pads as they are now as "already held": a key held over a screen change does not count. */
static void primePads(void) {
    held0 = prev0 = padHeld(0);
    held1 = prev1 = padHeld(1);
}

/**
 * @brief Run a fade a step per image. Never the blocking fadeIn()/fadeOut():
 * the pump must not freeze during a transition.
 */
static void fade(u8 direction) {
    screen_fade_frames = 0;
    fadeStart(direction, FADE_SPEED);
    do {
        WaitForVBlank();
        pump();
        screen_fade_frames++;
    } while (fadeStep());
}

void screenFrame(void) {
    WaitForVBlank();
    vramQueueFlush();           /* first thing in VBlank */
    pump();
    readPads();
}

void screenLoad(const u8 *src, u16 addr, u16 size, u16 step) {
    while (size) {
        u16 n = (size > SCREEN_LOAD_CHUNK) ? SCREEN_LOAD_CHUNK : size;
        /* The budget is ours: what does not fit waits for the next VBlank */
        if (vramQueueCost() + n + VRAM_QUEUE_ENTRY_COST > SCREEN_VBLANK_BUDGET
            || !vramQueuePush(src, addr, n, step)) {
            screenFrame();      /* flushes the queue: it is empty and fits next time */
            continue;
        }
        src += n;
        addr += (u16)(n >> 1);  /* VRAM addresses are words */
        size -= n;
    }
}

void screenShow(void) {
    WaitForVBlank();
    vramQueueFlush();           /* what the screen queued while composing */
    setBrightness(0);           /* the shadow only: the display is still forced blank */
    setScreenOn();              /* inside the VBlank, black */
    pump();
    if (screen_fade_skip) {
        setBrightness(15);
    } else {
        fade(FADE_IN);
    }
    primePads();
}

void screenClose(void) {
    void (*hook)(void);
    if (!screen_fade_skip) fade(FADE_OUT);
    WaitForVBlank();
    setScreenOff();
    hook = close_fn;
    close_fn = 0;               /* one shot: the next screen sets its own */
    if (hook) hook();
}

u16 screenPressed(u16 keys, u16 both) {
    u16 p = held0 & (u16)~prev0;
    if (both) p |= held1 & (u16)~prev1;
    return p & keys;
}

u16 screenWait(u16 frames) {
    while (frames--) {
        screenFrame();
        if (screenPressed(KEY_START, 1)) return 1;
    }
    return 0;
}
