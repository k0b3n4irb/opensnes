// A branch tests 16 bits; a 32-bit condition is compared with zero first and
// that compare is folded back into the branch (both halves OR-ed). A long
// narrowed to 16 bits must test its low word only. Found by
// testing/difftest.py, seeds 8 and 57, 2026-10-08.
long big;
int wide(void) { if (big) return 1; return 2; }
int narrowed(void) { if ((short)big) return 1; return 2; }
int right_of_and(int a) { return a && big; }
