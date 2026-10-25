// A10: the same cast as far_address_plain_pointer, written as a game would:
// a base address macro plus an offset. The expression is not folded when
// the cast is parsed; the check computes it without folding the tree.
#include <snes/types.h>
#define SRAM_BASE 0x700000
#define SRAM_SLOT(n) ((u8 *)(SRAM_BASE + 0x40 * (n)))
void put(u16 i, u8 v) { SRAM_SLOT(1)[i] = v; }
