/* The target's type sizes, pinned at compile time (KNOWN_LIMITATIONS.md,
 * "int and long sizes AND semantics match the w65816 target" and "Pointer
 * IR size is 4 bytes"). int is the native 16-bit word, long is 32 bits,
 * long long stays at 8 per C99, and a pointer carries its bank: 4 bytes
 * (24-bit address + 1 byte of alignment). A compiler change that moves any
 * of these fails this case to compile, which is what the two green
 * entries of the limitations page name as their test since 2026-10-05. */
_Static_assert(sizeof(char) == 1, "char is one byte");
_Static_assert(sizeof(short) == 2, "short is 16 bits");
_Static_assert(sizeof(int) == 2, "int is the native 16-bit word");
_Static_assert(sizeof(unsigned int) == 2, "unsigned int is 16 bits");
_Static_assert(sizeof(long) == 4, "long is 32 bits");
_Static_assert(sizeof(unsigned long) == 4, "unsigned long is 32 bits");
_Static_assert(sizeof(long long) == 8, "long long stays at 64 bits (C99)");
_Static_assert(sizeof(void *) == 4, "a pointer is 24-bit address + 1 byte: 4 bytes");
_Static_assert(sizeof(int (*)(void)) == 4, "a function pointer carries its bank too");

unsigned int sizes_pinned(void) {
    return (unsigned int)(sizeof(int) + sizeof(long) + sizeof(void *));
}
