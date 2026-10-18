/* The same compare with the rule off: through a temporary. */
typedef unsigned short u16;
extern u16 sent[8], out[8];
void changed(u16 c, u16 v) { if (v != sent[c]) out[c] = v; }
