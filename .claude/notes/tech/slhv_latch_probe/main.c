#include <snes.h>
extern void latchProbe(void);
int main(void) {
    consoleInit();
    latchProbe();
    while (1) { WaitForVBlank(); }
    return 0;
}
