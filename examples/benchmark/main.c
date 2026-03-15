/**
 * @file main.c
 * @brief OpenSNES Compiler Benchmark — Real CPU Cycle Measurement
 *
 * Counts loop iterations completed in one frame (between two VBlanks).
 * Uses frame_count (incremented by NMI) to detect frame boundaries.
 * Higher iteration count = faster generated code.
 *
 * Same tests as PVSnesLib benchmark for direct comparison:
 *   U16 ADD 4, U16 ADD VAR, U32 ADD 4, U32 ADD VAR
 */

#include <snes.h>

/* frame_count is incremented by NMI handler each VBlank */
extern volatile u16 frame_count;

/* Results — opensnes-emu reads these from WRAM */
u32 result_u16_add_const;
u32 result_u16_add_var;
u32 result_u32_add_const;
u32 result_u32_add_var;

/* Benchmark done flag */
u8 bench_done;

int main(void) {
    u16 start_frame;
    u32 count;
    u16 a16;
    u16 b16;
    unsigned int a32;
    unsigned int b32;

    consoleInit();
    setMode(BG_MODE0, 0);
    setScreenOn();

    /* --- Test 1: U16 ADD constant 4 --- */
    a16 = 0;
    count = 0;
    WaitForVBlank();
    start_frame = frame_count;
    while (frame_count == start_frame) {
        a16 += 4; a16 += 4; a16 += 4; a16 += 4;
        a16 += 4; a16 += 4; a16 += 4; a16 += 4;
        count++;
    }
    result_u16_add_const = count;

    /* --- Test 2: U16 ADD variable --- */
    a16 = 0;
    b16 = 4;
    count = 0;
    WaitForVBlank();
    start_frame = frame_count;
    while (frame_count == start_frame) {
        a16 += b16; a16 += b16; a16 += b16; a16 += b16;
        a16 += b16; a16 += b16; a16 += b16; a16 += b16;
        count++;
    }
    result_u16_add_var = count;

    /* --- Test 3: U32 ADD constant 4 --- */
    a32 = 0;
    count = 0;
    WaitForVBlank();
    start_frame = frame_count;
    while (frame_count == start_frame) {
        a32 += 4; a32 += 4; a32 += 4; a32 += 4;
        a32 += 4; a32 += 4; a32 += 4; a32 += 4;
        count++;
    }
    result_u32_add_const = count;

    /* --- Test 4: U32 ADD variable --- */
    a32 = 0;
    b32 = 4;
    count = 0;
    WaitForVBlank();
    start_frame = frame_count;
    while (frame_count == start_frame) {
        a32 += b32; a32 += b32; a32 += b32; a32 += b32;
        a32 += b32; a32 += b32; a32 += b32; a32 += b32;
        count++;
    }
    result_u32_add_var = count;

    bench_done = 1;

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
