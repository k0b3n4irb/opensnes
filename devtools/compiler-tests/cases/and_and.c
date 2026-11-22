/* Two masks in a row are one (A6a R5). */
typedef unsigned char u8;
typedef unsigned short u16;
#define FAR __far

u16 tab[256];
FAR u8 map[256];
u8 flags;

/* a byte read (one mask) used as a 16-bit index (its extension, another) */
u16 lookup(u8 FAR *p) { return tab[*p]; }

/* a bit of a byte */
u16 bit3(void) { return (flags & 8) != 0; }
