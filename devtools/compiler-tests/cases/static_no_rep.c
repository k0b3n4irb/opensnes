// A static function that is only ever called directly does not open with
// `rep #$20`: every call compiled in this unit is made in 16-bit A. A
// function that is exported, or whose address is used (stored in data, or
// taken in code), keeps it — it can be entered from assembly, in either
// width. The calling convention seen from assembly is unchanged.
/* noinline: the function this case is about must stay a function, whatever
 * an inliner would decide (I1 S3) */
static unsigned short __attribute__((noinline)) helper(unsigned short a) { return a + 3; }
static unsigned short in_table(unsigned short a) { return a + 5; }
unsigned short (*table)(unsigned short) = in_table;
static unsigned short taken(unsigned short a) { return a + 7; }
unsigned short pub(unsigned short a) {
    unsigned short (*q)(unsigned short) = taken;
    return helper(a) + q(a);
}
/* a second call site: until noinline was honoured (I1 S3), it alone kept
 * `helper` from being inlined */
unsigned short pub2(unsigned short a) { return helper(a) * 2; }
