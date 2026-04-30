/**
 * @file main.c
 * @brief Two-player simultaneous input with independent sprites
 * @ingroup examples
 *
 * Demonstrates reading two SNES controllers simultaneously via the
 * auto-joypad registers. The NMI handler reads both ports every frame,
 * and padHeld(0) / padHeld(1) return the held-button bitmasks for each
 * player. Each player controls an independent 8x8 sprite with the D-pad.
 *
 * Sprite tiles are defined inline as constant data (a solid 4bpp square),
 * and two palettes (blue for Player 1, red for Player 2) are loaded to
 * separate CGRAM sprite palette slots so each player has a distinct color.
 *
 * @par SNES Concepts
 * - Simultaneous two-controller reading via auto-joypad
 * - Per-player sprite palettes using CGRAM palette indexing
 * - Inline tile data (no external assets needed)
 * - oamSet() for simple sprite positioning
 *
 * @par What to Observe
 * - A blue square (Player 1) and a red square (Player 2) on a black screen
 * - Each sprite moves independently with its controller's D-pad
 * - Both sprites can move at the same time without interference
 *
 * @par Modules Used
 * console, input, sprite, dma, text
 *
 * @see input.h, sprite.h, dma.h
 */

#include <snes.h>

/**
 * @brief Inline 8x8 solid square sprite tile in SNES 4bpp format (32 bytes).
 *
 * SNES 4bpp tiles are stored as 4 interleaved bitplanes.
 * Planes 0-1 (first 16 bytes) are all 0xFF = every pixel has bits 0-1 set.
 * Planes 2-3 (next 16 bytes) are all 0x00 = bits 2-3 clear.
 * Result: every pixel = color index 3 (0b0011), producing a solid square
 * in whichever palette color 3 is assigned. No external asset file needed.
 */
static const u8 sprite_tile[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* Plane 0 */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* Plane 1 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* Plane 2 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   /* Plane 3 */
};

/**
 * @brief Sprite palette 0 (Player 1): Blue.
 *
 * SNES CGRAM stores 15-bit BGR555 colors (2 bytes each, little-endian).
 * Color 0 is transparent for sprites regardless of value. Only color 3
 * matters here because the tile's bitplanes produce index 3 for every pixel.
 * 0x7CFF = B=31, G=3, R=31... wait -- 0xFF,0x7C = $7CFF = blue ($7C00) + some.
 * Actual: low=0xFF high=0x7C => word $7CFF => R=31, G=7, B=15 => bright blue.
 */
static const u8 pal_blue[] = {
    0x00, 0x00,  /* Color 0: Transparent */
    0x00, 0x00,  /* Color 1: Black */
    0x00, 0x00,  /* Color 2: Black */
    0xFF, 0x7C,  /* Color 3: Blue */
};

/**
 * @brief Sprite palette 1 (Player 2): Red.
 *
 * Loaded to CGRAM 128+16 (sprite palette bank 1). The sprite's attribute
 * byte selects palette 1, so this red color is used for Player 2's square.
 * 0x001F = R=31, G=0, B=0 => pure bright red.
 */
static const u8 pal_red[] = {
    0x00, 0x00,  /* Color 0: Transparent */
    0x00, 0x00,  /* Color 1: Black */
    0x00, 0x00,  /* Color 2: Black */
    0x1F, 0x00,  /* Color 3: Red */
};

/*============================================================================
 * Player State
 *
 * NOTE: Using a struct with s16 coordinates is REQUIRED for reliable
 * sprite movement. Separate u16 variables cause horizontal movement
 * issues due to a compiler quirk. See .claude/KNOWLEDGE.md for details.
 *============================================================================*/

/**
 * @brief Per-player position state.
 *
 * Signed coordinates allow easy boundary clamping (< 0 check). Bundled
 * in a struct because the cc65816 compiler produces correct code for
 * struct-member arithmetic but has quirks with separate u16 variables.
 */
typedef struct {
    s16 x;  /**< Horizontal screen position (pixels, 0-255) */
    s16 y;  /**< Vertical screen position (pixels, 0-223) */
} Player;

/** @brief Player 1 state, initialized at left-center of screen */
Player p1 = {64, 112};
/** @brief Player 2 state, initialized at right-center of screen */
Player p2 = {192, 112};

/**
 * @brief Main entry point -- two-player sprite movement demo.
 *
 * Loads a single solid-square tile to sprite VRAM, sets up two palettes
 * (blue for Player 1, red for Player 2), and enters a loop where each
 * player's D-pad input moves their respective sprite independently.
 * Both controllers are read every frame via the NMI auto-joypad mechanism.
 *
 * @return 0 (never reached -- infinite game loop)
 */
int main(void) {
    u16 pad1, pad2;

    /* Positions initialized in global struct declaration */

    /* Initialize console (sets up display, enables NMI) */
    consoleInit();

    /* Initialize sprite graphics: tile + palette 0 (blue) + OBJSEL */
    oamInitGfxSet((u8 *)sprite_tile, 32,
                  (u8 *)pal_blue, 8,
                  0, 0x0000, OBJ_SIZE8_L16);

    /* Load palette 1 (Player 2: Red) */
    dmaCopyCGram((u8 *)pal_red, OBJ_CGRAM_PAL(1), 8);

    /* Initialize OAM */
    oamInit(OAM_DEFAULT_SIZE, OAM_DEFAULT_TILE_BASE);

    /* Set up Player 1 sprite (OAM entry 0) */
    oamSet(0, p1.x, p1.y, 0, 0, 0, 0);

    /* Set up Player 2 sprite (OAM entry 1) */
    oamSet(1, p2.x, p2.y, 0, 0, 0, 1);  /* Palette 1 */

    /* Enable sprites on main screen */
    setMainScreen(TM_OBJ);

    /* Turn on screen */
    setScreenOn();

    /* Main loop */
    while (1) {
        WaitForVBlank();

        /* Read both controllers (NMI handler reads joypads automatically) */
        pad1 = padHeld(0);
        pad2 = padHeld(1);

        /* Player 1 movement */
        if (pad1 & KEY_UP)    { if (p1.y > 0) p1.y--; }
        if (pad1 & KEY_DOWN)  { if (p1.y < 224) p1.y++; }
        if (pad1 & KEY_LEFT)  { if (p1.x > 0) p1.x--; }
        if (pad1 & KEY_RIGHT) { if (p1.x < 248) p1.x++; }

        /* Player 2 movement */
        if (pad2 & KEY_UP)    { if (p2.y > 0) p2.y--; }
        if (pad2 & KEY_DOWN)  { if (p2.y < 224) p2.y++; }
        if (pad2 & KEY_LEFT)  { if (p2.x > 0) p2.x--; }
        if (pad2 & KEY_RIGHT) { if (p2.x < 248) p2.x++; }

        /* Update sprite positions */
        oamSet(0, p1.x, p1.y, 0, 0, 0, 0);
        oamSet(1, p2.x, p2.y, 0, 0, 0, 1);

        /* Update OAM */
        oamUpdate();
    }

    return 0;
}
