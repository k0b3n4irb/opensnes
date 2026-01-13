/**
 * @file main.c
 * @brief Minimal OpenSNES Example
 *
 * Tests the new cc65816 compiler with functions and for-loops.
 *
 * License: CC0 (Public Domain)
 */

typedef unsigned char u8;

/* SNES hardware registers */
#define REG_INIDISP  (*(volatile u8*)0x2100)
#define REG_VMDATAL  (*(volatile u8*)0x2118)

/* Simple function */
int add(int a, int b) {
    return a + b;
}

/* Function with for-loop */
int sum_to_n(int n) {
    int i;
    int total = 0;
    for (i = 1; i <= n; i++) {
        total = total + i;
    }
    return total;
}

/* Main entry point */
int main(void) {
    int x;
    int y;
    int sum;
    int result;

    /* Test simple function */
    x = 10;
    y = 5;
    sum = add(x, y);

    /* Test for-loop: sum 1+2+3+4+5 = 15 */
    result = sum_to_n(5);

    /* Enable display */
    REG_INIDISP = 0x0F;

    /* Return sum of both results (15 + 15 = 30) */
    return sum + result;
}
