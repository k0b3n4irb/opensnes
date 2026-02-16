/**
 * @file main.c
 * @brief Two Players Demo
 *
 * Demonstrates reading two controllers simultaneously.
 * Each player controls their own sprite with the D-pad.
 *
 * Controls:
 *   Player 1 (Controller 1): D-pad moves blue sprite
 *   Player 2 (Controller 2): D-pad moves red sprite
 */

#include <snes.h>

/* Simple 8x8 sprite tile (solid square) */
const u8 sprite_tile[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* Plane 0 */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* Plane 1 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  /* Plane 2 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   /* Plane 3 */
};

/*============================================================================
 * Player State
 *
 * NOTE: Using a struct with s16 coordinates is REQUIRED for reliable
 * sprite movement. Separate u16 variables cause horizontal movement
 * issues due to a compiler quirk. See .claude/KNOWLEDGE.md for details.
 *============================================================================*/

typedef struct {
    s16 x, y;
} Player;

Player p1 = {64, 112};
Player p2 = {192, 112};

int main(void) {
    u16 pad1, pad2;
    u16 i;

    /* Positions initialized in global struct declaration */

    /* Initialize console (sets up display, enables NMI) */
    consoleInit();

    /* Set video mode 1 with 8x8 sprites */
    REG_OBJSEL = 0x00;  /* 8x8 and 16x16 sprites, base at 0x0000 */

    /* Upload sprite tiles to VRAM at 0x0000 */
    REG_VMAIN = 0x80;  /* Increment on high byte write */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;

    /* Copy tile data (32 bytes = 16 words) */
    for (i = 0; i < 32; i += 2) {
        REG_VMDATAL = sprite_tile[i];
        REG_VMDATAH = sprite_tile[i + 1];
    }

    /* Set up sprite palettes */
    /* Palette 0 (Player 1): Blue */
    REG_CGADD = 128;  /* Sprite palettes start at 128 */
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;  /* Color 0: Transparent */
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;  /* Color 1: Black */
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;  /* Color 2: Black */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7C;  /* Color 3: Blue */

    /* Palette 1 (Player 2): Red */
    REG_CGADD = 128 + 16;  /* Next palette */
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;  /* Color 0: Transparent */
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;  /* Color 1: Black */
    REG_CGDATA = 0x00; REG_CGDATA = 0x00;  /* Color 2: Black */
    REG_CGDATA = 0x1F; REG_CGDATA = 0x00;  /* Color 3: Red */

    /* Initialize OAM */
    oamInit();

    /* Set up Player 1 sprite (OAM entry 0) */
    oamSet(0, p1.x, p1.y, 0, 0, 0, 0);

    /* Set up Player 2 sprite (OAM entry 1) */
    oamSet(1, p2.x, p2.y, 0, 0, 0, 1);  /* Palette 1 */

    /* Enable sprites on main screen */
    REG_TM = TM_OBJ;

    /* Turn on screen */
    setScreenOn();

    /* Main loop */
    while (1) {
        WaitForVBlank();

        /* Wait for auto-joypad read to complete */
        while (REG_HVBJOY & 0x01) {}

        /* Read both controllers directly */
        pad1 = REG_JOY1L | (REG_JOY1H << 8);
        pad2 = REG_JOY2L | (REG_JOY2H << 8);

        /* Skip if controller disconnected */
        if (pad1 != 0xFFFF) {
            /* Player 1 movement */
            if (pad1 & KEY_UP)    { if (p1.y > 0) p1.y--; }
            if (pad1 & KEY_DOWN)  { if (p1.y < 224) p1.y++; }
            if (pad1 & KEY_LEFT)  { if (p1.x > 0) p1.x--; }
            if (pad1 & KEY_RIGHT) { if (p1.x < 248) p1.x++; }
        }

        if (pad2 != 0xFFFF) {
            /* Player 2 movement */
            if (pad2 & KEY_UP)    { if (p2.y > 0) p2.y--; }
            if (pad2 & KEY_DOWN)  { if (p2.y < 224) p2.y++; }
            if (pad2 & KEY_LEFT)  { if (p2.x > 0) p2.x--; }
            if (pad2 & KEY_RIGHT) { if (p2.x < 248) p2.x++; }
        }

        /* Update sprite positions */
        oamSet(0, p1.x, p1.y, 0, 0, 0, 0);
        oamSet(1, p2.x, p2.y, 0, 0, 0, 1);

        /* Update OAM */
        oamUpdate();
    }

    return 0;
}
