#include <snes.h>
static u16 k = 0x2222;
static u16 tag(void) { return 0xBBBB; }
u16 other_sum(void) { return k + tag(); }
