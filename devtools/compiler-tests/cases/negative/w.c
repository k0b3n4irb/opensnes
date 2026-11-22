// The same for a static object of a source named w.c: `lda.w v.w` reads v.
static unsigned v;
unsigned w_get(void) { return v++; }
