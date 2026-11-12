/* and_and with the rule off: both masks stay. */
typedef unsigned char u8;
typedef unsigned short u16;

u8 flags;

u16 bit3(void) { return (flags & 8) != 0; }
