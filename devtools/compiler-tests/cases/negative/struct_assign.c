// Whole-struct assignment (QBE `blit`): the 2026-07-19 silent drop, now a
// refusal. Copy field by field.
struct P { unsigned int x, y; };
struct P a, b;
int main(void) { a.x = 1; a.y = 2; b = a; return 0; }
