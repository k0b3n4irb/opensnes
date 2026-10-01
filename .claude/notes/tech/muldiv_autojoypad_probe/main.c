#include <snes.h>
extern void mulProbe(void);
int main(void) {
    consoleInit();
    mulProbe();
    while (1) { WaitForVBlank(); }
    return 0;
}
