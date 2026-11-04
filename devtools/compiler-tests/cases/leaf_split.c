// A leaf too big for the direct page keeps its costliest temps there.
//
// A leaf whose temps fit in the 16 words of tcc__lf has its whole frame
// there (dp_frame). Until 2026-11-04 one word more sent EVERY temp to the
// stack: `lda n,s` for `lda.b`, a bigger frame, and none of the peephole's
// direct-page rules. Now such a leaf is coloured as a function that calls
// is (every temp is "not cross": there is no call): the costliest temps
// (use count weighted by loop depth) take tcc__lf, the rest a smaller stack
// frame. A leaf with an object on the stack keeps the whole stack frame.
//
// Words needed by the stack-only colouring (the `slots=` of the function
// header with QBE_NO_LEAF_SPLIT=1): leaf17 17, leaf21 21, leaf25 25. After:
// 2, 6 and 10 words left on the stack. leaf17's two words belong to a
// 32-bit temp of the table index `4 * (n & 63)`, which the `lda.l ktab,x`
// that reads the table absorbs: it gets a slot and is never stored, so the
// frame is reserved and never touched (that slot was already dead weight
// when every temp was on the stack). testing/fixtures/compiler/leaf_split
// runs the same functions and checks the values.
typedef unsigned long u32;
typedef unsigned short u16;
u32 rr[8];
const u32 ktab[64] = {1, 2, 3};

void leaf17(u32 a, u32 b, u16 n) {
    u32 c = a + 2, d = a + 3;
    while (n--) { a += ktab[n & 63]; b += a; c += b; d += c; }
    rr[0] = a; rr[1] = b; rr[2] = c; rr[3] = d;
}

void leaf21(u32 a, u32 b, u16 n) {
    u32 c = a + 2, d = a + 3, e = a + 4;
    while (n--) { a += ktab[n & 63]; b += a; c += b; d += c; e += d; }
    rr[0] = a; rr[1] = b; rr[2] = c; rr[3] = d; rr[4] = e;
}

void leaf25(u32 a, u32 b, u16 n) {
    u32 c = a + 2, d = a + 3, e = a + 4, f = a + 5;
    while (n--) { a += ktab[n & 63]; b += a; c += b; d += c; e += d; f += e; }
    rr[0] = a; rr[1] = b; rr[2] = c; rr[3] = d; rr[4] = e; rr[5] = f;
}

// the same with an object on the stack: it keeps the whole stack frame
void leaf_arr(u32 a, u32 b, u16 n) {
    u32 buf[2];
    u32 c = a + 2, d = a + 3, e = a + 4;
    buf[0] = a; buf[1] = b;
    while (n--) {
        a += ktab[n & 63]; b += a; c += b; d += c; e += d;
        buf[n & 1] += e;
    }
    rr[0] = a; rr[1] = b; rr[2] = c; rr[3] = d; rr[4] = e ^ buf[0] ^ buf[1];
}
