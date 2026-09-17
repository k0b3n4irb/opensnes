// A struct returned by value: the caller receives it through `argc`/`parc`
// plumbing the w65816 backend does not implement.
struct P { unsigned int x, y; };
struct P mk(unsigned int a) { struct P p; p.x = a; p.y = a + 1; return p; }
unsigned int r;
int main(void) { struct P p = mk(5); r = p.x + p.y; return 0; }
