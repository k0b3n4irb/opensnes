/**
 * @file main.c
 * @brief Minimal Sprite Test
 *
 * Tests basic sprite display using the same pattern as simple_sprite.
 */

#include <snes.h>

/* 8x8 4bpp sprite tile - solid red square (color 1)
 * 4bpp format: first 16 bytes = bitplanes 0-1, next 16 bytes = bitplanes 2-3
 * Color 1 = 0b0001: bp0=1, bp1=0, bp2=0, bp3=0
 */
static const u8 sprite_tile[] = {
    /* Bitplanes 0-1: bp0=0xFF (all 1s), bp1=0x00 (all 0s) per row */
    0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00,
    0xFF,0x00, 0xFF,0x00, 0xFF,0x00, 0xFF,0x00,
    /* Bitplanes 2-3: all zeros */
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
};

/* 16-color sprite palette (32 bytes) */
static const u8 sprite_pal[] = {
    0x00, 0x00,  /* Color 0: Transparent */
    0x1F, 0x00,  /* Color 1: Red (BGR555: 00000 00000 11111) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

int main(void) {
    u16 i;

    /* Force blank - MUST be set before loading graphics! */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Enable NMI for VBlank sync */
    REG_NMITIMEN = NMITIMEN_NMI_ENABLE;

    /* Initialize OAM with 8x8/16x16 sprites, tile base at $0000 */
    oamInit();

    /* Load sprite tile to VRAM $0000 */
    REG_VMAIN = 0x80;  /* Auto-increment after high byte write */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;
    for (i = 0; i < 32; i += 2) {
        REG_VMDATAL = sprite_tile[i];
        REG_VMDATAH = sprite_tile[i + 1];
    }

    /* Load sprite palette to CGRAM 128 (first sprite palette) */
    REG_CGADD = 128;
    for (i = 0; i < 32; i++) {
        REG_CGDATA = sprite_pal[i];
    }

    /* Set sprite 0 at center of screen
     * Position: (120, 100)
     * Tile: 0
     * Palette: 0
     * Priority: 3 (highest)
     * Flags: 0 (no flip)
     */
    oamSet(0, 120, 100, 0, 0, 3, 0);

    /* Update OAM buffer to hardware */
    oamUpdate();

    /* Set Mode 1 (required for sprites) */
    REG_BGMODE = 0x01;

    /* Enable sprites on main screen */
    REG_TM = TM_OBJ;

    /* Enable display at full brightness */
    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    /* Main loop */
    while (1) {
        WaitForVBlank();
    }

    return 0;
}
