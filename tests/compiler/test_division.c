/* Test division operations:
 * - Division by powers of 2 (should use LSR shifts)
 * - Division by non-powers of 2 (should call __div16)
 * - Modulo by powers of 2 (should use AND masks)
 * - Modulo by non-powers of 2 (should call __mod16)
 */

/* Simple divide by 2 */
unsigned short div_by_2(unsigned short x) {
    return x / 2;
}

/* Divide by 4 */
unsigned short div_by_4(unsigned short x) {
    return x / 4;
}

/* Divide by 8 - common for animation frames */
unsigned short div_by_8(unsigned short x) {
    return x / 8;
}

/* Divide by 16 */
unsigned short div_by_16(unsigned short x) {
    return x / 16;
}

/* Divide by 256 - byte extraction */
unsigned short div_by_256(unsigned short x) {
    return x / 256;
}

/* Divide by non-power of 2 */
unsigned short div_by_3(unsigned short x) {
    return x / 3;
}

/* Divide by non-power of 2 */
unsigned short div_by_7(unsigned short x) {
    return x / 7;
}

/* Modulo by 2 */
unsigned short mod_by_2(unsigned short x) {
    return x % 2;
}

/* Modulo by 8 - common for frame cycling */
unsigned short mod_by_8(unsigned short x) {
    return x % 8;
}

/* Modulo by non-power of 2 */
unsigned short mod_by_3(unsigned short x) {
    return x % 3;
}

int main(void) {
    unsigned short value = 100;
    unsigned short result = 0;

    result += div_by_2(value);
    result += div_by_4(value);
    result += div_by_8(value);
    result += div_by_16(value);
    result += div_by_256(value);
    result += div_by_3(value);
    result += div_by_7(value);
    result += mod_by_2(value);
    result += mod_by_8(value);
    result += mod_by_3(value);

    return result;
}
