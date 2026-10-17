// A const variable is not an integer constant expression in C, so this array
// was a VLA too, and miscompiled the same way (audit 04-1). An enum or a
// #define gives the constant length the author meant.
static const unsigned N = 4;
unsigned first(void) { unsigned a[N]; a[0] = 1; return a[0]; }
