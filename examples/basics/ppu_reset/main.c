/**
 * @file main.c
 * @brief Basics — ppuReset(): the next screen starts from a clean PPU
 * @ingroup examples
 *
 * A game is a chain of screens: title, game, game over, title again. Each
 * one sets the PPU registers it needs, and every register it does not set
 * keeps the value the previous screen left. consoleInit() rewrites only a
 * few of them; ppuReset() rewrites all of them, to their power-on value.
 *
 * Screen A changes nearly every PPU register: the BG mode (Mode 1, text on
 * BG3), the sprite size and tile base, the map and tile bases of the four
 * layers, the scrolls, main and sub screen, a window, color math (a tint),
 * mosaic, the Mode 7 matrix, SETINI, the brightness, and an HDMA wave on
 * BG3. Two seconds later the program calls ppuReset() and sets up screen B
 * the way any first screen is set up. Screen B shows no trace of screen A.
 * Remove the ppuReset() call and screen B inherits the scroll of BG1 and
 * the tint.
 *
 * @par SNES Concepts
 * - The PPU registers are write-only and keep their value until written:
 *   a screen inherits every register the previous one set and it did not
 * - ppuReset() writes them all back to the values crt0 writes at boot, and
 *   puts the library's shadows (scrolls, mosaic, color math, windows,
 *   Mode 7, HDMA) back with them
 * - It leaves VRAM, CGRAM, the audio and NMITIMEN alone: the next screen
 *   still loads its own tiles, maps and palettes
 *
 * @par What to Observe
 * - Screen A, two seconds: a waving, tinted, blocky text on a blue backdrop
 * - Screen B: plain white text on green, in place, no wave, no tint
 *
 * @par Modules Used
 * console, dma, text, background, sprite, colormath, mosaic, window, mode7,
 * hdma, ppu_reset
 *
 * @see console.h (ppuReset), testing/manifests/basics_ppu_reset.toml
 */

#include <snes.h>

/** @brief How long screen A stays, in frames (two seconds at 60 Hz). */
#define SCREEN_A_FRAMES 120

/**
 * @brief Screen A: text on BG3 of Mode 1, and every other PPU register
 *        changed on purpose
 *
 * The visible part is the text (BG3), its scroll, the tint, the mosaic and
 * the wave. The rest (BG1, BG2, BG4, OBJSEL, the window, the sub screen,
 * Mode 7, SETINI) is invisible here and changed so that ppuReset() has
 * something to undo; the manifest reads it back from the PPU.
 */
void screen_a_setup(void) {
    textModeInit();                       /* Mode 0, text on BG1 */

    /* The text moves to BG3 of Mode 1: same 2bpp font, same map. */
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);
    bgSetGfxPtr(2, 0x0000);
    bgSetMapPtr(2, 0x3800, BG_MAP_32x32);
    bgSetScroll(2, 4, 4);                 /* the text, 4 pixels up and left */
    setMainScreen(LAYER_BG3);
    setColor(0, RGB(2, 4, 12));

    textPrintAt(4, 10, "SCREEN A");
    textPrintAt(4, 12, "EVERY PPU REGISTER CHANGED");
    textPrintAt(4, 14, "PPURESET() IN 2 SECONDS");

    /* The other three layers: maps, tiles and scrolls nobody shows. */
    bgSetMapPtr(0, 0x4000, BG_MAP_64x64);
    bgSetGfxPtr(0, 0x5000);
    bgSetScroll(0, 33, 17);
    bgSetMapPtr(1, 0x4800, BG_MAP_64x32);
    bgSetGfxPtr(1, 0x6000);
    bgSetScroll(1, 64, 128);
    bgSetMapPtr(3, 0x5800, BG_MAP_32x64);
    bgSetGfxPtr(3, 0x7000);
    bgSetScroll(3, 100, 50);
    setSubScreen(LAYER_BG1 | LAYER_BG2);

    oamInit(OBJ_SIZE16_L32, 1);           /* OBJSEL: 16/32 sprites, tiles at $2000 */

    /* Two windows on the hidden layers and on the sprites (none shown). */
    windowSetPos(WINDOW_1, 16, 240);
    windowSetPos(WINDOW_2, 40, 200);
    windowEnable(WINDOW_1, LAYER_BG1 | LAYER_BG2);
    windowEnable(WINDOW_2, LAYER_BG4 | LAYER_OBJ);
    windowSetLogic(LAYER_BG1, WINDOW_LOGIC_XOR);
    windowSetLogic(LAYER_OBJ, WINDOW_LOGIC_AND);
    windowSetMainMask(LAYER_BG1);
    windowSetSubMask(LAYER_BG2);

    /* Color math: the fixed colour added to the backdrop and the text. */
    colorMathTint(COLORMATH_BACKDROP | LAYER_BG3, 6, 2, 8);
    colorMathSetDirectColor(1);           /* CGWSEL bit 0: no effect without a 256-colour layer */

    mosaicSetSize(1);                     /* 2x2 blocks */
    mosaicSetLayers(0x04);                /* on BG3 */

    /* Mode 7 is not shown, but its registers keep what they are given. */
    mode7SetSettings(0x80);               /* M7SEL: transparent outside the map */
    mode7SetCenter(128, 112);             /* M7X, M7Y */
    mode7SetScale(0x0180, 0x0180);
    mode7SetAngle(32);                    /* M7A-M7D from the angle and the scale */

    videoSetObjInterlace(1);              /* SETINI bit 1 */
    fadeStart(FADE_OUT, 255);             /* a fade left running, never stepped */
    setBrightness(13);

    hdmaWaveInit();
    hdmaWaveH(HDMA_CHANNEL_6, 2, 4, 4);   /* the text waves */
    hdmaEnableMask(1 << HDMA_CHANNEL_6);

    WaitForVBlank();
    setScreenOn();
}

/**
 * @brief Screen B: set up exactly like a first screen
 *
 * Nothing here undoes screen A: ppuReset() did. The manifest reads the PPU
 * at the first instruction of this function, right after ppuReset(), and
 * finds the values it reads at main().
 */
void screen_b_setup(void) {
    textModeInit();
    setColor(0, RGB(0, 8, 4));

    textPrintAt(4, 10, "SCREEN B");
    textPrintAt(4, 12, "AFTER PPURESET()");
    textPrintAt(4, 14, "NO WAVE, NO TINT, NO MOSAIC");
    textPrintAt(4, 16, "NO SCROLL LEFT OVER");

    WaitForVBlank();
    setScreenOn();
}

/**
 * @brief Entry point: screen A for two seconds, ppuReset(), screen B
 *
 * @return Never returns (infinite loop).
 */
int main(void) {
    u16 start;

    screen_a_setup();
    /* Counted on the frame counter, not on loop turns: rebuilding the wave
     * table takes more than one frame, so a turn lasts two. */
    start = getFrameCount();
    while ((u16)(getFrameCount() - start) < SCREEN_A_FRAMES) {
        WaitForVBlank();
        hdmaWaveUpdate();
    }

    ppuReset();                           /* every PPU register to power-on */
    screen_b_setup();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
