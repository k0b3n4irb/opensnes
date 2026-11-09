// GNU attribute vector_size: it makes a vector of several elements. Ignored,
// as it was until 2026-11-09, sizeof(v4) was 2 where GNU C says 8 and every
// vector operation was a scalar one (gcc.c-torture scal-to-vec2).
typedef short v4 __attribute__((__vector_size__(8)));
v4 twice(v4 a) { return a + a; }
