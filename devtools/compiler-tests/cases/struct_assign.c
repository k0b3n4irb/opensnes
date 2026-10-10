// Whole-struct assignment, member by member at the member's width. Refused
// by accident until 2026-10-03 (the front end emitted halfword loads for a
// 2-aligned struct, which the backend has no lowering for) and silently
// half-copied for a 4-aligned one; testing/fixtures/compiler/d_quals
// checks every byte on luna. A FAR struct on either side is still refused.
struct P { unsigned int x, y; };
struct P a, b;
int main(void) { a.x = 1; a.y = 2; b = a; return 0; }
