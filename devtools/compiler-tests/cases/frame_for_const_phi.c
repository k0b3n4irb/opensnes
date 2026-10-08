// A phi whose arguments are all constants once QBE has folded the
// condition: no instruction is left to define a temp, and the function used
// to go frameless while the edge still stored the phi's value — on the
// return address (`sta 2,s` with no frame). Found by testing/difftest.py,
// seed 8, 2026-10-08.
int both(void) { int a = 1; return a && 1; }
int either(void) { int a = 0; return a || 3; }
