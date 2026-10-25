// A10: the casts of an address that the refusal of
// negative/far_address_plain_pointer must leave alone. A FAR pointee takes
// the bank from the pointer on an indexed store, a const pointee on an
// indexed read; an address in bank $00's range and a negative sentinel are
// not addresses with a bank to lose.
#include <snes/types.h>
#define EDGE_FAR   ((u8 FAR *)0x708040)
#define EDGE_CONST ((const u8 *)0x708040)
#define REG        ((volatile u8 *)0x2100)
void put_far(u16 i, u8 v) { EDGE_FAR[i] = v; }
u8 get_const(u16 i) { return EDGE_CONST[i]; }
void put_reg(u8 v) { *REG = v; }
u8 *sentinel(void) { return (u8 *)-1; }
u8 *low_sum(void) { return (u8 *)(0x2000 + 0x100 * 2); }
