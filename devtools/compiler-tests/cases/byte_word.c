/* A byte read is a word read and a mask (A6a R1), except where the byte
 * above may be a hardware register. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef volatile unsigned char vu8;
#define FAR __far

u8 g;
u8 arr[16];
FAR u8 farr[64];
const u8 tab[8] = {1, 2, 3, 4, 5, 6, 7, 8};
volatile u8 vflag;
typedef struct { u16 x; u8 t, a; } S;

u16 plain(void) { return g; }
u16 indexed(u16 i) { return arr[i]; }
u16 far_indexed(u16 i) { return farr[i]; }
u16 const_indexed(u16 i) { return tab[i & 7]; }
u16 through_ptr(u8 *p) { return *p; }
u16 field(S *s) { return s->a; }
u16 far_ptr(u8 FAR *p, u16 i) { return p[i]; }

/* kept 8-bit: volatile, and constant addresses outside low RAM */
u16 vol_global(void) { return vflag; }
u16 vol_ptr(vu8 *p) { return *p; }
u16 reg_volatile(void) { return *(vu8 *)0x2137; }
u16 reg_plain(void) { return *(u8 *)0x2138; }
u16 reg_indexed(u16 i) { return ((u8 *)0x4210)[i]; }
