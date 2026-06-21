/* Test tail call optimization:
 * - call_add: pass-through wrapper, same args → jml (no stores)
 * - call_chain: second call is tail call, different arg → sta + jml
 * - no_tail_call: callee arg count differs → no TCO
 */
unsigned short add_u16(unsigned short a, unsigned short b);
unsigned short add_one(unsigned short x);
unsigned short call_add(unsigned short a, unsigned short b) { return add_u16(a, b); }
unsigned short call_chain(unsigned short x) { return add_one(add_one(x)); }
unsigned short no_tail_call(void) { return add_u16(42, 100); }
