// || and && of constants are the int 0 or 1. cproc's evaluator returned one
// of the operands (7 for `5 && 7`, 9 for `0 || 9`): wrong enum values, array
// sizes and initialisers. Found by testing/difftest.py, seed 134, 2026-10-08.
enum { AND_TRUE = 5 && 7, OR_RIGHT = 0 || 9, OR_LEFT = 43732U || 0, AND_FALSE = 0 && 3 };
int and_true = AND_TRUE;
int or_right = OR_RIGHT;
int or_left = OR_LEFT;
int and_false = AND_FALSE;
int table[(2 || 0) + 1];
int table_bytes(void) { return sizeof table; }
