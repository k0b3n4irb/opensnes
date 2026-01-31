/**
 * @file animation.c
 * @brief OpenSNES Animation Implementation
 *
 * Frame-based sprite animation system.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>
#include <snes/animation.h>

/*============================================================================
 * Animation State
 *============================================================================*/

/* Animation slot structure (internal state) */
typedef struct {
    Animation *anim;    /* Pointer to animation definition */
    u8 frameNum;        /* Current frame number (0 to frameCount-1) */
    u8 counter;         /* Delay counter (counts down each VBlank) */
    u8 state;           /* Animation state */
} AnimSlot;

/* Animation slots */
static AnimSlot slots[ANIM_MAX_SLOTS];

/*============================================================================
 * Animation Control Functions
 *============================================================================*/

void animInit(u8 slotId, Animation *anim) {
    if (slotId >= ANIM_MAX_SLOTS) return;

    slots[slotId].anim = anim;
    slots[slotId].frameNum = 0;
    slots[slotId].counter = anim->frameDelay;
    slots[slotId].state = ANIM_STATE_STOPPED;
}

void animPlay(u8 slotId) {
    AnimSlot *slot;

    if (slotId >= ANIM_MAX_SLOTS) return;

    slot = &slots[slotId];
    if (slot->anim == 0) return;

    slot->frameNum = 0;
    slot->counter = slot->anim->frameDelay;
    slot->state = ANIM_STATE_PLAYING;
}

void animStop(u8 slotId) {
    AnimSlot *slot;

    if (slotId >= ANIM_MAX_SLOTS) return;

    slot = &slots[slotId];
    slot->frameNum = 0;
    slot->counter = 0;
    slot->state = ANIM_STATE_STOPPED;
}

void animPause(u8 slotId) {
    if (slotId >= ANIM_MAX_SLOTS) return;

    if (slots[slotId].state == ANIM_STATE_PLAYING) {
        slots[slotId].state = ANIM_STATE_PAUSED;
    }
}

void animResume(u8 slotId) {
    if (slotId >= ANIM_MAX_SLOTS) return;

    if (slots[slotId].state == ANIM_STATE_PAUSED) {
        slots[slotId].state = ANIM_STATE_PLAYING;
    }
}

void animUpdate(void) {
    u8 i;
    AnimSlot *slot;
    Animation *anim;

    for (i = 0; i < ANIM_MAX_SLOTS; i++) {
        slot = &slots[i];

        /* Skip if not playing */
        if (slot->state != ANIM_STATE_PLAYING) continue;

        /* Skip if no animation assigned */
        anim = slot->anim;
        if (anim == 0) continue;

        /* Decrement counter */
        if (slot->counter > 0) {
            slot->counter = slot->counter - 1;
        }

        /* Check if it's time to advance frame */
        if (slot->counter == 0) {
            /* Advance to next frame */
            slot->frameNum = slot->frameNum + 1;

            /* Check for end of animation */
            if (slot->frameNum >= anim->frameCount) {
                if (anim->loop) {
                    /* Loop: restart from frame 0 */
                    slot->frameNum = 0;
                } else {
                    /* One-shot: stay on last frame and mark finished */
                    slot->frameNum = anim->frameCount - 1;
                    slot->state = ANIM_STATE_FINISHED;
                    continue;
                }
            }

            /* Reset counter for next frame */
            slot->counter = anim->frameDelay;
        }
    }
}

/*============================================================================
 * Animation Query Functions
 *============================================================================*/

u8 animGetFrame(u8 slotId) {
    AnimSlot *slot;
    Animation *anim;

    if (slotId >= ANIM_MAX_SLOTS) return 0;

    slot = &slots[slotId];
    anim = slot->anim;

    if (anim == 0) return 0;
    if (slot->frameNum >= anim->frameCount) return 0;

    return anim->frames[slot->frameNum];
}

u8 animGetFrameNum(u8 slotId) {
    if (slotId >= ANIM_MAX_SLOTS) return 0;
    return slots[slotId].frameNum;
}

u8 animGetState(u8 slotId) {
    if (slotId >= ANIM_MAX_SLOTS) return ANIM_STATE_STOPPED;
    return slots[slotId].state;
}

u8 animIsPlaying(u8 slotId) {
    if (slotId >= ANIM_MAX_SLOTS) return 0;
    return (slots[slotId].state == ANIM_STATE_PLAYING) ? 1 : 0;
}

u8 animIsFinished(u8 slotId) {
    if (slotId >= ANIM_MAX_SLOTS) return 0;
    return (slots[slotId].state == ANIM_STATE_FINISHED) ? 1 : 0;
}

/*============================================================================
 * Animation Modification Functions
 *============================================================================*/

void animSetSpeed(u8 slotId, u8 delay) {
    AnimSlot *slot;

    if (slotId >= ANIM_MAX_SLOTS) return;

    slot = &slots[slotId];
    if (slot->anim != 0) {
        slot->anim->frameDelay = delay;
        /* Don't reset counter - let current frame finish */
    }
}

void animSetFrame(u8 slotId, u8 frameNum) {
    AnimSlot *slot;

    if (slotId >= ANIM_MAX_SLOTS) return;

    slot = &slots[slotId];
    if (slot->anim == 0) return;

    if (frameNum >= slot->anim->frameCount) {
        frameNum = slot->anim->frameCount - 1;
    }

    slot->frameNum = frameNum;
    slot->counter = slot->anim->frameDelay;
}

void animSetAnim(u8 slotId, Animation *anim) {
    AnimSlot *slot;

    if (slotId >= ANIM_MAX_SLOTS) return;

    slot = &slots[slotId];
    slot->anim = anim;

    /* Reset to start of new animation if frame is out of bounds */
    if (anim != 0) {
        if (slot->frameNum >= anim->frameCount) {
            slot->frameNum = 0;
        }
        slot->counter = anim->frameDelay;
    }
}
