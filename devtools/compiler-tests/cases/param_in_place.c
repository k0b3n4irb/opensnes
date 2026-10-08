// A parameter is read where the caller pushed it, in every function: the
// argument area lies above the frame and no call the function makes writes
// it. Until 2026-10-08 only a function without calls did that; any other
// began by copying each parameter into its frame (`lda 14,s` / `sta 4,s`).
//
// A function left with nothing on the stack has no frame at all, and its
// parameters are then at 4,s 6,s 8,s — not 2 bytes higher (the bug that hid
// the text of 25 examples while this was being written).
unsigned short w, v, u;
void nop(void) { w = 0; }

unsigned short fib(unsigned short n) { if (n < 2) return n; return fib(n - 1) + fib(n - 2); }

void set3(unsigned short a, unsigned short b, unsigned short c) { w = a; v = b; nop(); u = c; nop(); }

// (u32)x * 2..256 with a 16-bit x: x is loaded once and kept in tcc__r0 for
// the high half (it was read a second time from a slot that may never have
// been written).
unsigned long scale(unsigned long a) { return (unsigned long)(unsigned short)(a != 0) * 16; }
