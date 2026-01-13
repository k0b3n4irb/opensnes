#include <snes.h>

int main(void) {
    consoleEnableNMI();
    setScreenOn();
    while(1) {
        WaitForVBlank();
    }
    return 0;
}
