/**
 * @file main.c
 * @brief Parallax Scrolling Example
 *
 * Port of PVSnesLib Mode1MixedScroll example.
 * Demonstrates parallax scrolling with two backgrounds:
 * - BG1: Static main background
 * - BG0: Auto-scrolling foreground layer
 */

#include <snes.h>

extern void load_parallax_graphics(void);
extern void setup_parallax_regs(void);
extern void update_parallax(void);

int main(void) {
    REG_INIDISP = INIDISP_FORCE_BLANK;

    load_parallax_graphics();
    setup_parallax_regs();

    REG_INIDISP = INIDISP_BRIGHTNESS(15);

    while (1) {
        WaitForVBlank();
        update_parallax();
    }

    return 0;
}
