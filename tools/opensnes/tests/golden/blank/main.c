#include <snes.h>

int main(void) {
    /* One call sets up the PPU, the text engine, and the default font. */
    textModeInit();

    /* The NMI handler flushes printed text to VRAM automatically. */
    textPrintAt(9, 14, "HELLO SNES!");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
