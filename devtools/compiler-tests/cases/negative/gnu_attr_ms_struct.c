// GNU attribute ms_struct: the bit-field layout of Microsoft compilers, where
// a bit-field of another type starts a new unit: sizeof(struct s) is 4 there
// (clang, x86_64). Ignored, it was 2.
struct __attribute__((ms_struct)) s { char a : 4; short b : 4; };
unsigned int n = sizeof(struct s);
