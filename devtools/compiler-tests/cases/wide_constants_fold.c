// Constants of a type the target cannot compute with are still everyday C:
// a fixed-point scale written as a floating product, INT32_MIN written as
// the negation of 2147483648 (a long long), a 64-bit shift folded to an int.
// They are folded before code generation and must keep compiling.
int scaled(void) { return (int)(1.5 * 256); }
long lowest(void) { long x = -2147483648; return x; }
unsigned long highest(void) { unsigned long x = 4294967295; return x; }
int shifted(void) { return (int)((1ULL << 40) >> 36); }
