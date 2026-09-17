// A struct passed by value (QBE `parc` / `argc`): no w65816 lowering.
struct P { unsigned int x, y; };
unsigned int sum(struct P p) { return p.x + p.y; }
unsigned int r;
int main(void) { struct P p; p.x = 5; p.y = 6; r = sum(p); return 0; }
