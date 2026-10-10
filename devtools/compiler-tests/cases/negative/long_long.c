// 64-bit integers: `long long` is 8 bytes per C99, but the widest arithmetic
// the target has is 32 bits. Until 2026-10-08 this add dropped the upper half
// without a word.
long long next(long long a) { return a + 1; }
