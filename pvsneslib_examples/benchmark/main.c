/*
 * benchmark - Ported from PVSnesLib to OpenSNES
 *
 * Math benchmark measuring operation speed using frame counting.
 * PVSnesLib original used H-blank IRQ cycle counter; this version
 * uses getFrameCount() to measure elapsed frames for batches of ops.
 *
 * -- alekmaul (original PVSnesLib example)
 */

#include <snes.h>

#define ITERATIONS 10000

/* Volatile to prevent optimizer from removing operations */
volatile u16 result16;
volatile u16 operand_a;
volatile u16 operand_b;

/* Run a benchmark and return elapsed frames */
u16 bench_u16_add_const(void) {
    u16 start, end, i;
    u16 val = 0;

    WaitForVBlank();
    start = getFrameCount();

    for (i = 0; i < ITERATIONS; i++) {
        val = val + 42;
    }
    result16 = val;

    WaitForVBlank();
    end = getFrameCount();
    return end - start;
}

u16 bench_u16_add_var(void) {
    u16 start, end, i;
    u16 val = 0;

    operand_a = 42;
    WaitForVBlank();
    start = getFrameCount();

    for (i = 0; i < ITERATIONS; i++) {
        val = val + operand_a;
    }
    result16 = val;

    WaitForVBlank();
    end = getFrameCount();
    return end - start;
}

u16 bench_u16_mul_const(void) {
    u16 start, end, i;
    u16 val;

    WaitForVBlank();
    start = getFrameCount();

    for (i = 0; i < ITERATIONS; i++) {
        val = i * 3;
    }
    result16 = val;

    WaitForVBlank();
    end = getFrameCount();
    return end - start;
}

u16 bench_u16_shift(void) {
    u16 start, end, i;
    u16 val;

    WaitForVBlank();
    start = getFrameCount();

    for (i = 0; i < ITERATIONS; i++) {
        val = i << 2;
    }
    result16 = val;

    WaitForVBlank();
    end = getFrameCount();
    return end - start;
}

u16 bench_u16_mul_var(void) {
    u16 start, end, i;
    u16 val;

    operand_b = 7;
    WaitForVBlank();
    start = getFrameCount();

    for (i = 0; i < ITERATIONS; i++) {
        val = i * operand_b;
    }
    result16 = val;

    WaitForVBlank();
    end = getFrameCount();
    return end - start;
}

u16 bench_u16_div_const(void) {
    u16 start, end, i;
    u16 val;

    WaitForVBlank();
    start = getFrameCount();

    for (i = 0; i < ITERATIONS; i++) {
        val = i / 4;
    }
    result16 = val;

    WaitForVBlank();
    end = getFrameCount();
    return end - start;
}

void showResult(u8 y, const char *name, u16 frames) {
    textPrintAt(2, y, name);
    textSetPos(22, y);
    textPrintU16(frames);
    textPrint(" FR");
}

int main(void) {
    u16 frames;

    /* Initialize hardware */
    consoleInit();
    setMode(BG_MODE0, 0);

    /* Initialize text system and load font */
    textInit();
    textLoadFont(0x0000);

    /* Configure BG1 */
    bgSetGfxPtr(0, 0x0000);
    bgSetMapPtr(0, 0x3800, BG_MAP_32x32);

    /* White text color */
    REG_CGADD = 1;
    REG_CGDATA = 0xFF;
    REG_CGDATA = 0x7F;

    REG_TM = TM_BG1;

    textPrintAt(5, 1, "MATH BENCHMARK");
    textPrintAt(2, 3, "10000 ITERATIONS EACH");
    textPrintAt(2, 4, "LOWER FRAMES = FASTER");
    textPrintAt(2, 6, "RUNNING...");
    textFlush();
    WaitForVBlank();
    setScreenOn();

    /* Run benchmarks with screen on so user sees progress */
    frames = bench_u16_add_const();
    showResult(8, "U16 ADD CONST(+42)", frames);
    textFlush();

    frames = bench_u16_add_var();
    showResult(10, "U16 ADD VAR", frames);
    textFlush();

    frames = bench_u16_mul_const();
    showResult(12, "U16 MUL CONST(*3)", frames);
    textFlush();

    frames = bench_u16_shift();
    showResult(14, "U16 SHIFT LEFT(<<2)", frames);
    textFlush();

    frames = bench_u16_mul_var();
    showResult(16, "U16 MUL VAR(*7)", frames);
    textFlush();

    frames = bench_u16_div_const();
    showResult(18, "U16 DIV CONST(/4)", frames);
    textFlush();

    /* Clear "RUNNING..." */
    textPrintAt(2, 6, "DONE!         ");
    textFlush();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
