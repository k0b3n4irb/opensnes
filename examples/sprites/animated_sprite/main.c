/**
 * @file main.c
 * @brief Direction-based sprite animation with horizontal flip
 * @ingroup examples
 *
 * Demonstrates a classic sprite animation pattern: Kit, the fox-kit courier
 * seen from above, with a walk cycle per direction (down, up, left, right). Each direction
 * uses a different row of frames in the sprite sheet, and the left-facing
 * animation reuses the right-facing frames with the OAM horizontal flip
 * bit (bit 6 of attribute byte).
 *
 * Animation is frame-paced with a configurable delay between tile changes.
 * The sprite's OAM tile number is updated each frame via oamSet() to point
 * at the correct frame in the pre-loaded sprite sheet. Movement is
 * controlled with the D-PAD and clamped to screen boundaries.
 *
 * This began as the PVSnesLib AnimatedSprite example.
 *
 * @par SNES Concepts
 * - Sprite animation via OAM tile number updates
 * - Horizontal flip (OBJ_FLIPX) to mirror sprite frames
 * - D-PAD input for 4-directional movement
 * - Frame-paced animation with delay counters
 *
 * @par What to Observe
 * - A 32x32 character sprite that walks in 4 directions with D-PAD
 * - Walking left mirrors the right-facing animation
 * - The animation cycles through 5 frames per direction
 * - The sprite stops animating when no button is pressed
 *
 * @par Modules Used
 * console, sprite, dma, input, anim
 *
 * @see sprite.h, input.h, dma.h
 */

#include <snes.h>
#include <snes/anim.h>

/*============================================================================
 * Graphics Data (converted by the build from res/, one settings file beside each picture)
 *============================================================================*/

/** @brief Sprite sheet tile data containing all animation frames (converted by the build from res/) */
#include "res/kit_walk.inc"   /* generated: kit_walk_tiles[], kit_walk_tiles_end[], kit_walk_pal[], kit_walk_pal_end[] */
/** @brief Palette for the character sprite (16 colors, 4bpp) */

/*============================================================================
 * Constants
 *============================================================================*/


/**
 * @brief Animation direction states.
 *
 * Each state maps to a row in the sprite sheet. W_LEFT reuses the W_RIGHT
 * row and sets the OAM horizontal flip bit to mirror the frames, which is
 * a common SNES technique to halve the sprite sheet size for left/right
 * symmetric characters.
 */
enum SpriteState {
    W_DOWN = 0,     /**< Walking downward (facing camera) */
    W_UP = 1,       /**< Walking upward (facing away) */
    W_RIGHT = 2,    /**< Walking right */
    W_LEFT = 2      /**< Walking left — reuses W_RIGHT tiles with horizontal flip */
};

/**
 * @brief Screen edge boundaries for sprite clamping.
 *
 * Negative values allow the sprite to partially leave the screen (16px margin),
 * which looks more natural than stopping exactly at the visible edge.
 */
enum {
    SCREEN_TOP = -16,     /**< Top clamp: 16px above visible area */
    SCREEN_BOTTOM = 224,  /**< Bottom clamp: bottom of visible area */
    SCREEN_LEFT = -16,    /**< Left clamp: 16px left of visible area */
    SCREEN_RIGHT = 256    /**< Right clamp: right edge of visible area */
};

/*============================================================================
 * Game Data
 *============================================================================*/

/**
 * @brief Sprite state structure holding position, animation, and direction.
 *
 * Combines all per-sprite data into one struct. On the SNES, each OAM entry
 * only stores position and tile number — animation state must be tracked
 * separately in RAM and used to compute the correct tile each frame.
 */
typedef struct {
    s16 x, y;          /**< Screen position in pixels (signed for off-screen clamping) */
    u16 gfx_frame;     /**< Current tile number to display in OAM */
    u8 state;          /**< Current direction (SpriteState enum value) */
    u8 flipx;          /**< Horizontal flip flag: 1 = mirror sprite for left-facing */
} Walker;

/**
 * @brief Number of frames to hold each animation frame before advancing.
 *
 * At 60fps, a delay of 6 means each walk frame lasts 6/60 = 100ms,
 * producing a 500ms walk cycle (5 frames x 100ms). Adjust this value
 * to make the animation faster (lower) or slower (higher).
 */
#define ANIM_DELAY 6

/** @brief Kit, the player-controlled sprite, initialized facing down at (100, 100) */
Walker kit = {100, 100, 0, W_DOWN, 0};

/*============================================================================
 * Animation clips (anim module) — one clip per facing.
 *
 * A frame value here is a raw OAM tile number. The converter lays the
 * sheet's 32x32 blocks out four to a 128-pixel row of OBJ VRAM, so block k
 * starts at tile (k % 4) * 4 + (k / 4) * 64: the sheet holds five frames
 * walking down (blocks 0-4), five up (5-9), five to the right (10-14).
 * LEFT reuses the RIGHT clip with H-flip.
 * ANIM_DELAY ticks per frame = the same 100ms cadence the old hand-rolled
 * anim_delay counter produced.
 *============================================================================*/
DECLARE_ANIM_CLIP(walk_down, ANIM_LOOP, ANIM_DELAY, 0, 4, 8, 12, 64);
DECLARE_ANIM_CLIP(walk_up,   ANIM_LOOP, ANIM_DELAY, 68, 72, 76, 128, 132);
DECLARE_ANIM_CLIP(walk_side, ANIM_LOOP, ANIM_DELAY, 136, 140, 192, 196, 200);

/** @brief Playback head for Kit (zero-init = stopped). */
AnimPlayer kit_anim = ANIM_PLAYER_INIT;

/*============================================================================
 * Main
 *============================================================================*/

/**
 * @brief Entry point: animated character sprite with D-PAD movement.
 *
 * Loads a multi-frame sprite sheet, initializes OAM, and runs a game loop
 * that reads D-PAD input, updates the sprite position and animation state,
 * then writes the new tile number and flip flags to OAM each frame.
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    u16 pad0;

    consoleInit();

    /* Force blank during setup — prevents visible VRAM writes */
    setScreenOff();

    /* oamInitGfxSet() is a convenience function that:
     * 1. DMAs tile data to OBJ VRAM at word address $0000
     * 2. DMAs palette to CGRAM address 128 (sprite palette 0)
     * 3. Sets OBJSEL size mode to 16x16 small / 32x32 large
     * Palette slot 0 = CGRAM 128-159 (16 colors x 2 bytes) */
    oamInitGfxSet(kit_walk_tiles, kit_walk_tiles_end - kit_walk_tiles,
                  kit_walk_pal, kit_walk_pal_end - kit_walk_pal,
                  0, 0x0000, OBJ_SIZE16_L32);

    /* Initialize OAM entry 0 at Kit's starting position.
     * Tile 0, palette 0, priority 3 (in front of all BGs), no flip.
     * The valid on-screen Y set here is what makes the sprite visible —
     * SNES visibility is Y-based, there is no separate "show" call. */
    oamSet(0, kit.x, kit.y, 0, 0, 3, 0);
    oamSetSize(0, OBJ_LARGE);   /* large = 32x32 in this OBJSEL mode */

    /* The SNES has 128 hardware sprites. Any unused sprites must be hidden
     * (moved off-screen) to prevent garbage tiles from appearing. */
    for (u8 i = 1; i < 128; i++) {
        oamHide(i);
    }

    /* Flush the OAM shadow buffer to signal the NMI handler to DMA it */
    oamUpdate();

    /* Mode 1 with only the OBJ layer visible (no backgrounds) */
    setMode(BG_MODE1, 0);
    setMainScreen(LAYER_OBJ);

    /* Enable display at full brightness (INIDISP = $0F) */
    setScreenOn();

    /* Main loop: runs once per frame (60Hz on NTSC) */
    while (1) {
        WaitForVBlank();

        /** @brief Current joypad state — bits set for each held button */
        pad0 = padHeld(0);

        if (pad0 != 0) {
            /* Handle movement */
            if (pad0 & KEY_UP) {
                if (kit.y >= SCREEN_TOP) kit.y--;
                kit.state = W_UP;
                kit.flipx = 0;
            }
            if (pad0 & KEY_LEFT) {
                if (kit.x >= SCREEN_LEFT) kit.x--;
                kit.state = W_LEFT;
                kit.flipx = 1;
            }
            if (pad0 & KEY_RIGHT) {
                if (kit.x <= SCREEN_RIGHT) kit.x++;
                kit.state = W_RIGHT;
                kit.flipx = 0;
            }
            if (pad0 & KEY_DOWN) {
                if (kit.y <= SCREEN_BOTTOM) kit.y++;
                kit.state = W_DOWN;
                kit.flipx = 0;
            }

            /* Select the clip for the current facing — animPlay's
             * continue-if-same semantics make this safe to call every
             * frame: holding a direction keeps the cycle running, and
             * changing direction restarts the new clip cleanly. */
            if (kit.state == W_DOWN)      animPlay(&kit_anim, &walk_down);
            else if (kit.state == W_UP)   animPlay(&kit_anim, &walk_up);
            else                              animPlay(&kit_anim, &walk_side);

            /* Advance one tick — the clip data owns the cadence (the old
             * anim_delay/anim_frame counters are gone). */
            kit.gfx_frame = animTick(&kit_anim);
        }
        /* Idle: no tick — a paused animation costs nothing and the last
         * frame simply stays in OAM (same behavior as before). */

        u16 flags = kit.flipx ? OBJ_FLIPX : 0;
        oamSet(0, kit.x, kit.y, kit.gfx_frame, 0, 3, flags);
    }

    return 0;
}
