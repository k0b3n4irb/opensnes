/**
 * @file game.h
 * @brief What the screens of shard_dash share: the layout, the rules, the run's result
 * @ingroup examples
 */
#ifndef SHARD_DASH_GAME_H
#define SHARD_DASH_GAME_H

#include <snes.h>

/** @name VRAM (word addresses): three maps, then each layer's tiles on a $1000 boundary
 *  @{ */
#define VRAM_BG1_MAP  0x0000   /**< the play square */
#define VRAM_BG2_MAP  0x0400   /**< the logo */
#define VRAM_BG3_MAP  0x0800   /**< the text */
#define VRAM_BG1_GFX  0x1000
#define VRAM_BG2_GFX  0x2000
#define VRAM_BG3_FONT 0x3000
#define SPR_VRAM      0x4000   /**< sprite tiles */
/** @} */

/** @name The rules
 *  @{ */
#define GOAL          5        /**< shards to collect */
#define TIME_LIMIT    1800     /**< frames on the clock: 30 s at 60 Hz */
#define RESULT_WAIT   900      /**< frames the result stays before the title returns */
/** @} */

/** @name Values of game_screen
 *  @{ */
#define SCREEN_TITLE  0
#define SCREEN_PLAY   1
#define SCREEN_RESULT 2
/** @} */

/** @brief Which screen runs (the tests read it by name). */
extern u8 game_screen;
/** @brief Frames the pump ran since power-on: it runs in fades, waits and loads too. */
extern u16 pump_frames;
/** @brief The last round: 1 if the goal was reached before the clock. */
extern u8 run_won;
/** @brief The last round: frames from the start to the last shard (or TIME_LIMIT). */
extern u16 run_time;
/** @brief The last round: 1 if it beat the saved best. */
extern u8 run_new_best;

/** @brief The title screen: returns when START is pressed. */
void titleRun(void);
/** @brief One round: returns when the goal is reached or the clock is out. */
void playRun(void);
/** @brief The result screen: saves a win, shows it, returns on START or after RESULT_WAIT. */
void resultRun(void);

#endif
