/**
 * @file main.c
 * @brief Direction-based sprite animation with horizontal flip
 * @ingroup examples
 *
 * Demonstrates a classic sprite animation pattern: a character sprite with
 * multiple directional walk cycles (down, up, left, right). Each direction
 * uses a different row of frames in the sprite sheet, and the left-facing
 * animation reuses the right-facing frames with the OAM horizontal flip
 * bit (bit 6 of attribute byte).
 *
 * Animation is frame-paced with a configurable delay between tile changes.
 * The sprite's OAM tile number is updated each frame via oamSet() to point
 * at the correct frame in the pre-loaded sprite sheet. Movement is
 * controlled with the D-PAD and clamped to screen boundaries.
 *
 * Ported from PVSnesLib AnimatedSprite example.
 *
 * @par SNES Concepts
 * - Sprite animation via OAM tile number updates
 * - Horizontal flip (OBJ_FLIPX) to mirror sprite frames
 * - D-PAD input for 4-directional movement
 * - Frame-paced animation with delay counters
 *
 * @par What to Observe
 * - A 16x16 character sprite that walks in 4 directions with D-PAD
 * - Walking left mirrors the right-facing animation
 * - The animation cycles through 3 frames per direction
 * - The sprite stops animating when no button is pressed
 *
 * @par Modules Used
 * console, sprite, dma, input
 *
 * @see sprite.h, input.h, dma.h
 */

#include <snes.h>

/*============================================================================
 * External Graphics Data (defined in data.asm)
 *============================================================================*/

/** @brief Sprite sheet tile data containing all animation frames (defined in data.asm) */
extern u8 sprite_tiles[], sprite_tiles_end[];
/** @brief Palette for the character sprite (16 colors, 4bpp) */
extern u8 sprite_pal[], sprite_pal_end[];

/*============================================================================
 * Constants
 *============================================================================*/

/** @brief Number of distinct frames in each directional walk cycle */
#define FRAMES_PER_ANIMATION 3

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
    u16 anim_frame;    /**< Current frame index within the walk cycle (0 to FRAMES_PER_ANIMATION-1) */
    u16 anim_delay;    /**< Frame counter for animation timing (counts up to ANIM_DELAY) */
    u8 state;          /**< Current direction (SpriteState enum value) */
    u8 flipx;          /**< Horizontal flip flag: 1 = mirror sprite for left-facing */
} Monster;

/**
 * @brief Number of frames to hold each animation frame before advancing.
 *
 * At 60fps, a delay of 6 means each walk frame lasts 6/60 = 100ms,
 * producing a 300ms walk cycle (3 frames x 100ms). Adjust this value
 * to make the animation faster (lower) or slower (higher).
 */
#define ANIM_DELAY 6

/** @brief The player-controlled character sprite, initialized facing down at (100, 100) */
Monster monster = {100, 100, 0, 0, 0, W_DOWN, 0};

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

    /* Force blank during setup — prevents visible VRAM writes */
    setScreenOff();

    /* oamInitGfxSet() is a convenience function that:
     * 1. DMAs tile data to OBJ VRAM at word address $0000
     * 2. DMAs palette to CGRAM address 128 (sprite palette 0)
     * 3. Sets OBJSEL size mode to 16x16 small / 32x32 large
     * Palette slot 0 = CGRAM 128-159 (16 colors x 2 bytes) */
    oamInitGfxSet(sprite_tiles, sprite_tiles_end - sprite_tiles,
                  sprite_pal, sprite_pal_end - sprite_pal,
                  0, 0x0000, OBJ_SIZE16_L32);

    /* Initialize OAM entry 0 at the monster's starting position.
     * Tile 0, palette 0, priority 3 (in front of all BGs), no flip. */
    oamSet(0, monster.x, monster.y, 0, 0, 3, 0);
    oamSetSize(0, 0);       /* 0 = use small size (16x16 in this OBJSEL mode) */
    oamSetVisible(0, 1);    /* Make sprite 0 visible */

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
                if (monster.y >= SCREEN_TOP) monster.y--;
                monster.state = W_UP;
                monster.flipx = 0;
            }
            if (pad0 & KEY_LEFT) {
                if (monster.x >= SCREEN_LEFT) monster.x--;
                monster.state = W_LEFT;
                monster.flipx = 1;
            }
            if (pad0 & KEY_RIGHT) {
                if (monster.x <= SCREEN_RIGHT) monster.x++;
                monster.state = W_RIGHT;
                monster.flipx = 0;
            }
            if (pad0 & KEY_DOWN) {
                if (monster.y <= SCREEN_BOTTOM) monster.y++;
                monster.state = W_DOWN;
                monster.flipx = 0;
            }

            /* Advance animation with delay */
            monster.anim_delay++;
            if (monster.anim_delay >= ANIM_DELAY) {
                monster.anim_delay = 0;
                monster.anim_frame++;
                if (monster.anim_frame >= FRAMES_PER_ANIMATION)
                    monster.anim_frame = 0;
            }
        }

        /* Calculate tile based on state and animation frame
         * Sprite sheet layout (16x16 sprites, tile numbers):
         *   DOWN:  0, 2, 4
         *   UP:    6, 8, 10
         *   RIGHT: 12, 14, 32
         *   LEFT:  same as RIGHT with H-flip
         */
        if (monster.state == W_DOWN) {
            monster.gfx_frame = monster.anim_frame * 2;        /* 0, 2, 4 */
        } else if (monster.state == W_UP) {
            monster.gfx_frame = 6 + monster.anim_frame * 2;    /* 6, 8, 10 */
        } else {
            /* W_RIGHT / W_LEFT */
            if (monster.anim_frame < 2) {
                monster.gfx_frame = 12 + monster.anim_frame * 2; /* 12, 14 */
            } else {
                monster.gfx_frame = 32;                         /* 32 */
            }
        }
        u16 flags = monster.flipx ? OBJ_FLIPX : 0;
        oamSet(0, monster.x, monster.y, monster.gfx_frame, 0, 3, flags);
    }

    return 0;
}
