/* Minimal test to isolate compiler issue */
typedef unsigned char u8;

#define REG_JOY1H (*(volatile u8*)0x4219)

u8 joypad;
u8 pos_x;

void test_input(void) {
    joypad = REG_JOY1H;
    
    if (joypad & 0x01) {
        pos_x = pos_x + 1;
    }
}
