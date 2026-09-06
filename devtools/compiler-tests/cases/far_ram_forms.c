typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

/* B2: __far objects live in bank $7E; every access must carry the bank. */
__far u8  fbuf[64];
__far u16 fwords[16];
__far u32 flong;
typedef struct { u8 a; u8 b; u16 w; u32 l; } rec;

u8   dir_rd(void)              { return fbuf[3]; }
void dir_wr(u8 v)              { fbuf[3] = v; }
u32  long_rd(void)             { return flong; }
void long_wr(u32 v)            { flong = v; }
u8   idx_rd(u16 i)             { return fbuf[i]; }
void idx_wr(u16 i, u8 v)       { fbuf[i] = v; }
void idx_wr16(u16 i, u16 v)    { fwords[i] = v; }
u16  fld_rd(rec __far *r)      { return r->w; }
void fld_wr(rec __far *r, u32 v) { r->l = v; }
void walk(u8 __far *p, u8 n)   { u8 k; for (k = 0; k < n; k++) p[k] = k; }
