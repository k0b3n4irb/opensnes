typedef unsigned short u16;

/* === Case 1: NO inline keyword → not eligible (no hint) === */
static u16 no_kw(u16 x) { return x + 1; }
u16 call_no_kw(u16 y) { return no_kw(y); }

/* === Case 2: inline + control flow (if) → not eligible (has_cond) === */
static inline u16 with_if(u16 x) {
    if (x > 10) return x;
    return x + 1;
}
u16 call_with_if(u16 y) { return with_if(y); }

/* === Case 3: inline + nested call → not eligible (has_calls) === */
static u16 helper_for_nested(u16 x) { return x + 1; }
static inline u16 with_nested_call(u16 x) { return helper_for_nested(x); }
u16 call_with_nested(u16 y) { return with_nested_call(y); }

/* === Case 4: inline + recursive → not eligible (has_calls) === */
static inline u16 with_recursion(u16 x) {
    if (x == 0) return 0;
    return x + with_recursion(x - 1);
}
u16 call_with_recursion(u16 y) { return with_recursion(y); }

/* === Case 5: inline but body too big (> 8 instr) → not eligible === */
static inline u16 too_big(u16 x) {
    u16 a = x + 1;
    u16 b = a * 2;
    u16 c = b - 3;
    u16 d = c + 4;
    u16 e = d * 5;
    u16 f = e - 6;
    u16 g = f + 7;
    u16 h = g * 8;
    return h;
}
u16 call_too_big(u16 y) { return too_big(y); }

/* === Case 6: trivial inline, should INLINE === */
static inline u16 trivial(u16 x) { return x + 42; }
u16 call_trivial(u16 y) { return trivial(y); }
