// A string literal is in ROM: a store to it did nothing at run time and
// said nothing at build time. Refused since literals are const char[]
int main(void) { "abc"[0] = 'x'; return 0; }
