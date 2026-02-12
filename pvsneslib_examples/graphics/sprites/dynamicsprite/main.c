/*
 * DynamicSprite - PVSnesLib example ported to OpenSNES
 *
 * Demonstrates the dynamic sprite engine with frame animation:
 *   - Uses OBJ_SIZE8_L16 (small=8x8, large=16x16)
 *   - oamInitDynamicSprite() sets up the VRAM upload queue
 *   - oambuffer[0] struct holds sprite state (position, frame, graphics ptr)
 *   - oamDynamic16Draw() draws and queues VRAM uploads for changed frames
 *   - oamVramQueueUpdate() uploads changed tiles to VRAM during VBlank
 *   - Animation cycles through 4 frames every 16 VBlanks
 */

#include <snes.h>

extern u8 spr_tiles[];
extern u8 spr_pal[];

int main(void) {
    t_sprites *player;

    consoleInit();

    /* Configure BG1 (empty background, just for mode setup) */
    bgSetGfxPtr(0, 0x2000);
    bgSetMapPtr(0, 0x6800, BG_MAP_32x32);

    setMode(BG_MODE1, 0);
    REG_TM = TM_BG1 | TM_OBJ;

    /* Init dynamic sprite engine:
     * Large sprites at VRAM $0000, small sprites at VRAM $1000
     * OBJ_SIZE8_L16: small=8x8, large=16x16 */
    oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);

    /* Load sprite palette to CGRAM slot 128 (OBJ palette 0) */
    dmaCopyCGram(spr_pal, 128, 16 * 2);

    /* Setup player sprite */
    player = &oambuffer[0];
    player->oamx = 128 - 8;    /* Center of screen */
    player->oamy = 112 - 8;
    player->oamframeid = 0;     /* Start at frame 0 */
    player->oamattribute = 0;   /* Palette 0, no flip, priority 0 */
    OAM_SET_GFX(0, spr_tiles);  /* Set 24-bit graphics address */
    player->oamrefresh = 1;     /* Request initial VRAM upload */

    setScreenOn();

    while (1) {
        /* Animate: cycle through 4 frames every 16 VBlanks */
        if ((getFrameCount() & 15) == 15) {
            player->oamframeid++;
            player->oamframeid &= 3;
            player->oamrefresh = 1; /* Request VRAM upload for new frame */
        }

        /* Draw dynamic sprite (queues VRAM upload if oamrefresh is set) */
        oamDynamic16Draw(0);

        /* End frame: hide any unused sprite slots */
        oamInitDynamicSpriteEndFrame();

        WaitForVBlank();

        /* Upload changed sprite tiles to VRAM (must be during VBlank) */
        oamVramQueueUpdate();
    }
    return 0;
}
