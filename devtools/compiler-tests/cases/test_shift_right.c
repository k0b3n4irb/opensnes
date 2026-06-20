/* Test shift right operation */

static volatile unsigned char *REG_VMADDH = (unsigned char*)0x2117;

void test_shift(unsigned short addr) {
    *REG_VMADDH = addr >> 8;
}

int main(void) {
    test_shift(0x1234);
    return 0;
}
