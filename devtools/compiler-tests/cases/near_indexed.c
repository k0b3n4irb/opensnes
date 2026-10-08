// A global array read or written with a run-time index uses the 65816's
// indexed-long mode: X is the scaled index, the symbol is the operand. Until
// 2026-10-08 the address was built in A (clc / adc #sym / sta), reloaded,
// moved to X, and the access went through `$0000,x`.
//
// The mode adds X as an UNSIGNED 16-bit offset and carries into the bank, so
// it is only used for an index known to be >= 0 — or straight off a symbol
// with no offset, where a negative index is outside the object.
unsigned short tab[32];
unsigned char bytes[32];

unsigned short rd(unsigned short i) { return tab[i]; }
void wr(unsigned short i, unsigned short v) { tab[i] = v; }
unsigned char rdb(unsigned char i) { return bytes[i]; }
// offset and a signed index: may be negative, the 16-bit sum is kept
unsigned short rd_signed(short j) { return (tab + 4)[j]; }
// offset and an unsigned index: indexed
unsigned short rd_unsigned(unsigned short j) { return (tab + 4)[j]; }
