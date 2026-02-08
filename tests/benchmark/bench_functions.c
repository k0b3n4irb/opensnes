/*
 * Compiler benchmark functions — standalone C for cycle analysis.
 *
 * Compile:  ./bin/cc65816 bench_functions.c -o bench_functions.asm
 * Analyze:  python3 tools/cyclecount/cyclecount.py bench_functions.asm
 * Compare:  python3 tools/cyclecount/cyclecount.py --compare before.asm after.asm
 *
 * Each function isolates one compiler code generation pattern.
 * After a compiler change, recompile and compare cycle counts.
 */

/* --- 1. Empty function: prologue/epilogue overhead --- */
unsigned short empty_func(void) {
    return 0;
}

/* --- 2. Addition: basic 16-bit ALU --- */
unsigned short add_u16(unsigned short a, unsigned short b) {
    return a + b;
}

/* --- 3. Subtraction --- */
unsigned short sub_u16(unsigned short a, unsigned short b) {
    return a - b;
}

/* --- 4. Multiply by constant (shift+add pattern) --- */
unsigned short mul_const_13(unsigned short a) {
    return a * 13;
}

/* --- 5. Multiply by power of 2 (should be single shift) --- */
unsigned short mul_const_8(unsigned short a) {
    return a * 8;
}

/* --- 6. Division by constant (runtime call) --- */
unsigned short div_const_10(unsigned short a) {
    return a / 10;
}

/* --- 7. Modulo by constant (runtime call) --- */
unsigned short mod_const_10(unsigned short a) {
    return a % 10;
}

/* --- 8. Left shift by constant --- */
unsigned short shift_left_3(unsigned short a) {
    return a << 3;
}

/* --- 9. Right shift by constant --- */
unsigned short shift_right_4(unsigned short a) {
    return a >> 4;
}

/* --- 10. Bitwise AND --- */
unsigned short bitwise_and(unsigned short a, unsigned short b) {
    return a & b;
}

/* --- 11. Bitwise OR --- */
unsigned short bitwise_or(unsigned short a, unsigned short b) {
    return a | b;
}

/* --- 12. Simple if/else --- */
unsigned short conditional(unsigned short a, unsigned short b) {
    if (a > b) return a;
    return b;
}

/* --- 13. Loop with accumulator --- */
unsigned short loop_sum(unsigned short n) {
    unsigned short sum;
    unsigned short i;
    sum = 0;
    for (i = 0; i < n; i++) {
        sum += i;
    }
    return sum;
}

/* --- 14. Array write --- */
void array_write(unsigned short *arr, unsigned short idx, unsigned short val) {
    arr[idx] = val;
}

/* --- 15. Array read --- */
unsigned short array_read(unsigned short *arr, unsigned short idx) {
    return arr[idx];
}

/* --- 16. Struct field access --- */
struct Point { unsigned short x; unsigned short y; };

unsigned short struct_sum(struct Point *p) {
    return p->x + p->y;
}

/* --- 17. Multiple return values via pointer --- */
void swap(unsigned short *a, unsigned short *b) {
    unsigned short tmp;
    tmp = *a;
    *a = *b;
    *b = tmp;
}

/* --- 18. Nested function call --- */
unsigned short call_add(unsigned short a, unsigned short b) {
    return add_u16(a, b);
}

/* --- 19. Multiply by variable (runtime call) --- */
unsigned short mul_variable(unsigned short a, unsigned short b) {
    return a * b;
}

/* --- 20. Comparison chain --- */
unsigned short clamp(unsigned short val, unsigned short lo, unsigned short hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}
