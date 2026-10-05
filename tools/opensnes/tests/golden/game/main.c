#include <snes.h>

/* A single 8x8 4bpp tile. Plane 0 is all-set, planes 1-3 are clear, so every
 * pixel reads palette index 1 — which we colour white below. */
static const u8 player_tile[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};

/* Two BGR555 colours: index 0 transparent, index 1 white ($7FFF). */
static const u8 player_pal[4] = { 0x00, 0x00, 0xFF, 0x7F };

static s16 player_x = 120;
static s16 player_y = 100;

int main(void) {
    consoleInit();
    WaitForVBlank();

    /* Upload the sprite tile to VRAM and its palette to the sprite CGRAM. */
    dmaCopyVram((u8 *)player_tile, 0x4000, sizeof player_tile);
    dmaCopyCGram((u8 *)player_pal, OBJ_CGRAM_BASE, sizeof player_pal);

    REG_OBJSEL = OBJSEL(OBJ_SIZE8_L16, 0x4000);
    oamInit(OBJ_SIZE8_L16, 1);
    setMainScreen(LAYER_OBJ);
    setScreenOn();

    while (1) {
        u16 pad = padHeld(0);

        if (pad & KEY_UP)    player_y--;
        if (pad & KEY_DOWN)  player_y++;
        if (pad & KEY_LEFT)  player_x--;
        if (pad & KEY_RIGHT) player_x++;

        oamSet(0, player_x, player_y, 0, 0, 3, 0);
        WaitForVBlank();
    }

    return 0;
}
