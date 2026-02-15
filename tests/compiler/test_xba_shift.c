/* Test: XBA byte-swap optimization for constant shifts ≥ 8
 * Verifies that shifts by 8+ use xba instead of repeated shift loops.
 */

unsigned short shr8(unsigned short x) {
    return x >> 8;
}

short sar8(short x) {
    return x >> 8;
}

unsigned short shl8(unsigned short x) {
    return x << 8;
}

unsigned short shr12(unsigned short x) {
    return x >> 12;
}

short sar12(short x) {
    return x >> 12;
}

unsigned short shl12(unsigned short x) {
    return x << 12;
}
