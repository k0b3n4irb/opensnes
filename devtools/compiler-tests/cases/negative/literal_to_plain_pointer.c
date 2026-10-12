// A string literal is ROM data in a bank the linker picks; only a read
// through a const pointee is a far read. Handed to a plain `char *`, the
// literal was read from bank $00 at run time, with no diagnostic anywhere.
// Literals are `const char[]` since 2026-10-11, so this is refused.
unsigned char rd(char *s, unsigned int i) { return (unsigned char)s[i]; }
int main(void) { return rd("WXYZ", 2); }
