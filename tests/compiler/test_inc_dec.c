/* Test: INC/DEC optimization for ±1 constants
 * add with const 1 should emit 'inc a' instead of 'clc; adc.w #1'
 * sub with const 1 should emit 'dec a' instead of 'sec; sbc.w #1'
 */

unsigned short increment(unsigned short x) {
    return x + 1;
}

unsigned short decrement(unsigned short x) {
    return x - 1;
}

/* add with 0xFFFF (-1 in u16) should emit 'dec a' */
unsigned short add_ffff(unsigned short x) {
    return x + 0xFFFF;
}

/* sub with 0xFFFF (-1 in u16) should emit 'inc a' */
unsigned short sub_ffff(unsigned short x) {
    return x - 0xFFFF;
}

/* Regular add should NOT use inc/dec */
unsigned short add_five(unsigned short x) {
    return x + 5;
}
