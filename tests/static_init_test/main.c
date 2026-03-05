/* Test for initialized static variables */
typedef unsigned char u8;
typedef unsigned short u16;

#define REG_INIDISP  (*(volatile u8*)0x2100)
#define REG_CGADD    (*(volatile u8*)0x2121)
#define REG_CGDATA   (*(volatile u8*)0x2122)
#define REG_TM       (*(volatile u8*)0x212C)

/* This should now work - initialized static in RAM */
static u8 test_value = 42;

int main(void) {
    REG_INIDISP = 0x8F;  /* Force blank */
    REG_TM = 0x00;       /* Disable all BG layers */

    /* Modify the initialized static - if it's in RAM, this works */
    /* If it's in ROM, this silently fails and test_value stays 42 */
    test_value = test_value + 1;  /* Should become 43 */

    /* Set backdrop color based on result */
    REG_CGADD = 0;
    if (test_value == 43) {
        /* GREEN - success: static was in RAM and modifiable */
        REG_CGDATA = 0xE0;
        REG_CGDATA = 0x03;
    } else {
        /* RED - failure: static was in ROM (read-only) */
        REG_CGDATA = 0x1F;
        REG_CGDATA = 0x00;
    }

    REG_INIDISP = 0x0F;  /* Screen on */

    while(1) {}
    return 0;
}
