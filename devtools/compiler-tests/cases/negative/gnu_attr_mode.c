// GNU attribute mode: it sets the width of a type. Ignored, as it was until
// 2026-11-09, sizeof(u8q) was 2 where GNU C says 1, and the program read and
// wrote two bytes for one without a word (gcc.c-torture 20020108-1).
typedef unsigned int u8q __attribute__((mode(QI)));
u8q next(u8q a) { return a + 1; }
