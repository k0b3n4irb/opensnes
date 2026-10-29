/* A pointer advanced in place and read at once (textPrint's shape): the
 * add of str += n is folded into the next load, which reads str and n
 * there. The 32-bit exact slot sharing of 2026-10-25 first put the sum in
 * str's slot, and the load read str + n + n; the slot-ownership check
 * stopped the library build. An add that leads to an address shares no
 * slot with its operand. */
typedef unsigned short u16; typedef unsigned char u8;
extern u8 cx, flag;
extern void put(char c);
extern u16 run(const char *s, u16 n);
void tp(const char *str) {
    u16 n; u8 c = (u8)*str;
    while (c) {
        if (c < 32 || cx >= 31) { put((char)c); c = (u8)*++str; continue; }
        n = run(str, (u16)(31 - cx));
        str += n;
        cx = (u8)(cx + n);
        c = (u8)*str;
    }
}
