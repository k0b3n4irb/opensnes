typedef unsigned char u8;
typedef unsigned short u16;
#define REG (*(volatile u8 *)0x211F)
void setcenter(u16 v) {
    REG = (u8)v;
    REG = (u8)(v >> 8);
}
