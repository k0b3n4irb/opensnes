// A struct assigned to an int is a constraint violation (C11 6.5.16.1);
// the compiler used to stop on "internal error; unsupported conversion".
struct s { int a; };
struct s g;
int main(void) { int y; y = g; return y; }
