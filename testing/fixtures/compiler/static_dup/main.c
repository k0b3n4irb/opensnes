/* Two translation units each define `static u16 k` and `static u16 tag(void)`.
 * Until 2026-10-05 the link failed ("Label k was defined more than once");
 * the statics are now emitted k.main / k.other. test_static_dup.py reads the
 * four results back by those names. */
#include <snes.h>
static u16 k = 0x1111;
static u16 tag(void) { return 0xAAAA; }
u16 r_main, r_other;
extern u16 other_sum(void);
int main(void) {
    consoleInit();
    r_main = k + tag();          /* 0xBBBB */
    r_other = other_sum();       /* 0xDDDD */
    while (1) WaitForVBlank();
    return 0;
}
