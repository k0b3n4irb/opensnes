typedef unsigned char u8;
typedef unsigned short u16;
extern const u8 romtab[];
extern const u16 romtab16[];

u8 byteidx(u16 i) { return romtab[i]; }
u16 wordidx(u16 i) { return romtab16[i]; }
