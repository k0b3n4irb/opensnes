// A conditional over a near target is one inverted branch; over a far one it
// stays `bxx + / jmp @target / +` (issue #166, pattern 5). "Near" is an upper
// bound of the bytes in between, so `far_target` must keep the long form: its
// skipped body is well over 127 bytes.
unsigned int g0, g1, g2, g3, g4, g5, g6, g7;

unsigned int near_target(unsigned int a, unsigned int b) {
    if (a < b)
        g0 = a;
    return b;
}

unsigned int far_target(unsigned int a, unsigned int b) {
    if (a < b) {
        g0 = a + 1; g1 = a + 2; g2 = a + 3; g3 = a + 4;
        g4 = a + 5; g5 = a + 6; g6 = a + 7; g7 = a + 8;
        g0 += b; g1 += b; g2 += b; g3 += b;
        g4 += b; g5 += b; g6 += b; g7 += b;
        g0 ^= g1; g2 ^= g3; g4 ^= g5; g6 ^= g7;
    }
    return b;
}
