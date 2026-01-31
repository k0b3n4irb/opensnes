/**
 * @file animation.h
 * @brief SNES Sprite Animation Framework
 *
 * Simple frame-based animation system for sprites.
 *
 * ## Overview
 *
 * This module provides:
 * - Frame-based animation playback
 * - Multiple animation slots (up to 32 concurrent animations)
 * - Variable frame delays
 * - Loop and one-shot modes
 * - Easy integration with OAM sprites
 *
 * ## Usage Example
 *
 * @code
 * // Define animation frames (tile indices)
 * u8 walkFrames[] = { 0, 1, 2, 3, 2, 1 };
 *
 * // Create animation definition
 * Animation walkAnim = {
 *     .frames = walkFrames,
 *     .frameCount = 6,
 *     .frameDelay = 6,   // 6 VBlanks per frame = 10 FPS
 *     .loop = 1
 * };
 *
 * // In game init:
 * animInit(0, &walkAnim);  // Slot 0 = walk animation
 * animPlay(0);             // Start playing
 *
 * // In main loop (after WaitForVBlank):
 * animUpdate();            // Update all animations
 *
 * // Get current frame for sprite:
 * u8 tile = animGetFrame(0);
 * oamSet(0, x, y, tile, ...);
 * @endcode
 *
 * ## Integration with Sprites
 *
 * The animation system returns tile indices. You use these with
 * oamSet() to update sprite graphics. The frames array should
 * contain tile indices that match your sprite tileset.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_ANIMATION_H
#define OPENSNES_ANIMATION_H

#include <snes/types.h>

/*============================================================================
 * Constants
 *============================================================================*/

/** @brief Maximum number of concurrent animations */
#define ANIM_MAX_SLOTS 32

/*============================================================================
 * Types
 *============================================================================*/

/**
 * @brief Animation definition
 *
 * Defines the frames and timing for an animation sequence.
 */
typedef struct {
    u8 *frames;         /**< Array of frame indices (tile numbers) */
    u8 frameCount;      /**< Number of frames in animation */
    u8 frameDelay;      /**< VBlanks per frame (1 = 60 FPS, 6 = 10 FPS) */
    u8 loop;            /**< 1 = loop forever, 0 = play once and stop */
} Animation;

/**
 * @brief Animation state flags
 */
#define ANIM_STATE_STOPPED  0   /**< Animation is stopped */
#define ANIM_STATE_PLAYING  1   /**< Animation is playing */
#define ANIM_STATE_PAUSED   2   /**< Animation is paused */
#define ANIM_STATE_FINISHED 3   /**< One-shot animation finished */

/*============================================================================
 * Animation Control Functions
 *============================================================================*/

/**
 * @brief Initialize an animation slot
 *
 * Associates an Animation definition with a slot ID. The animation
 * is initialized but not started.
 *
 * @param slotId Animation slot (0 to ANIM_MAX_SLOTS-1)
 * @param anim Pointer to Animation definition
 *
 * @code
 * animInit(0, &walkAnim);
 * animInit(1, &idleAnim);
 * @endcode
 */
void animInit(u8 slotId, Animation *anim);

/**
 * @brief Start playing an animation
 *
 * Starts or restarts animation from the first frame.
 *
 * @param slotId Animation slot to play
 *
 * @code
 * animPlay(0);  // Start walk animation
 * @endcode
 */
void animPlay(u8 slotId);

/**
 * @brief Stop an animation
 *
 * Stops animation and resets to first frame.
 *
 * @param slotId Animation slot to stop
 */
void animStop(u8 slotId);

/**
 * @brief Pause an animation
 *
 * Pauses animation at current frame. Use animResume() to continue.
 *
 * @param slotId Animation slot to pause
 */
void animPause(u8 slotId);

/**
 * @brief Resume a paused animation
 *
 * Continues a paused animation from where it stopped.
 *
 * @param slotId Animation slot to resume
 */
void animResume(u8 slotId);

/**
 * @brief Update all animations
 *
 * Must be called once per frame (typically after WaitForVBlank).
 * Updates all animation counters and advances frames as needed.
 *
 * @code
 * while (1) {
 *     WaitForVBlank();
 *     animUpdate();
 *     // ... update sprites ...
 * }
 * @endcode
 */
void animUpdate(void);

/*============================================================================
 * Animation Query Functions
 *============================================================================*/

/**
 * @brief Get current frame index
 *
 * Returns the current tile index for the animation. Use this value
 * to set the sprite's tile in oamSet().
 *
 * @param slotId Animation slot
 * @return Current frame value (tile index)
 *
 * @code
 * u8 tile = animGetFrame(0);
 * oamSet(spriteId, x, y, tile, palette, flags);
 * @endcode
 */
u8 animGetFrame(u8 slotId);

/**
 * @brief Get current frame number (0 to frameCount-1)
 *
 * Returns which frame number is currently showing, not the tile index.
 *
 * @param slotId Animation slot
 * @return Current frame number (0 to frameCount-1)
 */
u8 animGetFrameNum(u8 slotId);

/**
 * @brief Get animation state
 *
 * @param slotId Animation slot
 * @return Animation state (ANIM_STATE_STOPPED, PLAYING, PAUSED, FINISHED)
 */
u8 animGetState(u8 slotId);

/**
 * @brief Check if animation is playing
 *
 * @param slotId Animation slot
 * @return 1 if playing, 0 otherwise
 */
u8 animIsPlaying(u8 slotId);

/**
 * @brief Check if one-shot animation has finished
 *
 * @param slotId Animation slot
 * @return 1 if finished, 0 otherwise
 */
u8 animIsFinished(u8 slotId);

/*============================================================================
 * Animation Modification Functions
 *============================================================================*/

/**
 * @brief Set animation speed
 *
 * Change the frame delay while animation is running.
 *
 * @param slotId Animation slot
 * @param delay New frame delay (VBlanks per frame)
 */
void animSetSpeed(u8 slotId, u8 delay);

/**
 * @brief Jump to specific frame
 *
 * @param slotId Animation slot
 * @param frameNum Frame number to jump to (0 to frameCount-1)
 */
void animSetFrame(u8 slotId, u8 frameNum);

/**
 * @brief Change animation definition
 *
 * Switch to a different animation without resetting. Useful for
 * seamless transitions between animations.
 *
 * @param slotId Animation slot
 * @param anim New Animation definition
 *
 * @code
 * // Switch from walk to run animation
 * animSetAnim(0, &runAnim);
 * @endcode
 */
void animSetAnim(u8 slotId, Animation *anim);

#endif /* OPENSNES_ANIMATION_H */
