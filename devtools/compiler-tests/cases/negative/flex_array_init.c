// A flexible array member takes no initialiser (C11 6.7.2.1p18); the
// compiler used to stop on an assertion in the data emitter.
struct s { int n; int a[]; };
struct s v = { 1, { 2, 3 } };
int main(void) { return v.n; }
