/* byte_word with the rule off: every byte read is an 8-bit read. */
typedef unsigned char u8;
typedef unsigned short u16;
#define FAR __far

u8 g;
u8 arr[16];
FAR u8 farr[64];

u16 plain(void) { return g; }
u16 indexed(u16 i) { return arr[i]; }
u16 far_indexed(u16 i) { return farr[i]; }
u16 through_ptr(u8 *p) { return *p; }
