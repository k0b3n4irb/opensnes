/**
 * @file screen.h
 * @brief The screen layer: what every screen outside the round needs
 * @ingroup examples
 *
 * A screen is one function `xxxRun()`: compose with the display off,
 * screenShow(), loop on screenFrame() + screenPressed(), screenClose().
 * The layer owns what they share, so no screen repeats it: the VBlank wait,
 * the VRAM queue's flush, the fades, the pad's edges, and the **pump**, the
 * one function the game wants run on every image whatever the screen is
 * doing (its sound driver, its counters).
 */
#ifndef SHARD_DASH_SCREEN_H
#define SHARD_DASH_SCREEN_H

#include <snes.h>

/** @brief VBlank budget, in bytes of vramQueueCost(), that screenLoad() keeps to. */
#define SCREEN_VBLANK_BUDGET 3072
/** @brief Largest piece screenLoad() queues in one entry. */
#define SCREEN_LOAD_CHUNK    2048

/** @brief 1: the fades are skipped (long tests). 0 by default. A test may poke it. */
extern u16 screen_fade_skip;
/** @brief Images the last fade took (16 at speed 1), for the tests. */
extern u16 screen_fade_frames;

/** @brief Function run on EVERY image: fades, waits, loads and frames. No state is passed. */
void screenSetPump(void (*fn)(void));
/** @brief Hook run by screenClose(): undoes what the screen set up. One shot. */
void screenSetClose(void (*fn)(void));
/** @brief 1: skip the fades; 0: play them for real. */
void screenFadeSkip(u16 on);
/** @brief Upload a block to VRAM through the queue, in pieces that keep to SCREEN_VBLANK_BUDGET. */
void screenLoad(const u8 *src, u16 addr, u16 size, u16 step);
/** @brief Display on, non-blocking fade-in; a key already held does not count. */
void screenShow(void);
/** @brief Non-blocking fade-out, display off, then the close hook. */
void screenClose(void);
/** @brief One image of a lit screen: VBlank, queue flush, pump, pads. */
void screenFrame(void);
/** @brief Keys newly pressed this image, among @p keys; @p both also reads pad 2. */
u16  screenPressed(u16 keys, u16 both);
/** @brief Wait @p frames images or START; 1 if START cut it short. */
u16  screenWait(u16 frames);

#endif
