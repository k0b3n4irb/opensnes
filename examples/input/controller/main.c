/**
 * @file main.c
 * @brief Both controllers on screen: held, pressed, released, and a frame counter
 * @ingroup examples
 *
 * Reads the two SNES controllers and shows, for each, every button held —
 * one letter a button, so several at once show together. Under them, the
 * last button pressed and the last button released on pad 1, and the
 * number of VBlanks since boot. This is the starting point for understanding
 * how SNES games read player input, and the screen the console check uses.
 *
 * The SNES joypad has 12 buttons: 4 face buttons (A, B, X, Y), 2 shoulder
 * buttons (L, R), a D-pad (4 directions), START, and SELECT. The NMI
 * handler reads both joypads automatically every VBlank. Your code just
 * calls padHeld(pad) for the buttons down, padPressed(pad) for the ones
 * that went down on this frame, and padReleased(pad) for the ones that
 * came up.
 *
 * Ported from PVSnesLib "controller" example by alekmaul; the second pad
 * was the two_players example, the counter the timer example, both merged
 * here.
 *
 * @par SNES Concepts
 * - padHeld(n) — 16-bit bitmask of the buttons down on pad n (0 or 1)
 * - padPressed(n) / padReleased(n) — the edges: down this frame, up this frame
 * - KEY_A, KEY_B, KEY_X, KEY_Y, KEY_L, KEY_R, KEY_UP, KEY_DOWN, KEY_LEFT,
 *   KEY_RIGHT, KEY_START, KEY_SELECT — the bits, in the order the pad
 *   shifts them out (B Y SELECT START UP DOWN LEFT RIGHT A X L R)
 * - frame_count — the VBlanks since boot, counted by the NMI handler
 *
 * @par What to Observe
 * - Hold buttons on pad 1 or pad 2: their letters appear on that pad's line
 *   while held, several at once, and disappear on release
 * - The last button pressed and released on pad 1 stay on screen
 * - The counter goes up by 60 a second (50 on PAL); L+R together resets it
 *
 * @par Modules Used
 * console, sprite, dma, background, text, input, gameloop
 *
 * @see input.h
 */

#include <snes.h>
#include <snes/text.h>
#include <snes/gameloop.h>

/** @brief The 12 buttons in the order the pad sends them, with a letter and a name */
static const u16 key_bit[12] = {
    KEY_B, KEY_Y, KEY_SELECT, KEY_START, KEY_UP, KEY_DOWN,
    KEY_LEFT, KEY_RIGHT, KEY_A, KEY_X, KEY_L, KEY_R
};
static const char key_letter[13] = "BYES^V<>AXLR";   /* E = SELECT, S = START, the arrows = the D-pad */
static const char *const key_name[12] = {
    "B     ", "Y     ", "SELECT", "START ", "UP    ", "DOWN  ",
    "LEFT  ", "RIGHT ", "A     ", "X     ", "L     ", "R     "
};

/** @brief One pad's line: a letter where the button is held, a dot where not */
static char line[13];

/** @brief Print pad n's held buttons at row y. */
static void show_pad(u8 pad, u8 y) {
    u16 held = padHeld(pad);
    u8 i;
    for (i = 0; i < 12; i++)
        line[i] = (held & key_bit[i]) ? key_letter[i] : '.';
    line[12] = 0;
    textPrintAt(12, y, line);
}

/** @brief The name of the first button set in a mask, for the edges. */
static const char *first_name(u16 mask) {
    u8 i;
    for (i = 0; i < 12; i++)
        if (mask & key_bit[i])
            return key_name[i];
    return 0;
}

static void on_init(void) {
    textModeInit();
    textPrintAt(12, 1, "PAD TEST");
    textPrintAt(4, 4, "PAD 1");
    textPrintAt(4, 6, "PAD 2");
    textPrintAt(12, 8, "BYES^V<>AXLR");
    textPrintAt(4, 11, "PRESSED  -");
    textPrintAt(4, 13, "RELEASED -");
    textPrintAt(4, 17, "FRAMES");
    textPrintAt(4, 20, "L+R RESETS THE COUNT");
    WaitForVBlank();
    setScreenOn();
}

static void on_update(void) {
    const char *name;

    show_pad(0, 4);
    show_pad(1, 6);

    name = first_name(padPressed(0));
    if (name)
        textPrintAt(13, 11, name);
    name = first_name(padReleased(0));
    if (name)
        textPrintAt(13, 13, name);

    /* L+R resets once, on the frame the pair is completed (one of the two
     * just went down while the other is held), not on every frame held */
    if ((padHeld(0) & (KEY_L | KEY_R)) == (KEY_L | KEY_R)
            && (padPressed(0) & (KEY_L | KEY_R)))
        frame_count = 0;
    textSetPos(12, 17);
    textPrintU16(frame_count);
    textPrint("     ");
}

/**
 * @brief Entry point — the game loop runs on_init once, then on_update each frame.
 * @return Never returns
 */
int main(void) {
    GameLoopConfig cfg = {
        .init = on_init,
        .update = on_update,
    };
    gameLoopRun(&cfg);
    return 0;
}
