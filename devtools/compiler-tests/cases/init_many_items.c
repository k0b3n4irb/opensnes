/* A const object of more than 16384 initialized elements: the emitter
 * buffered them in a fixed table of 16384 and died on the next with
 * "too many data items", naming nothing (2026-10-14). The
 * buffer grows now; 20480 elements, the last one checked in the output. */
#define A4      1, 2, 3, 4
#define A16     A4, A4, A4, A4
#define A64     A16, A16, A16, A16
#define A256    A64, A64, A64, A64
#define A1024   A256, A256, A256, A256
#define A4096   A1024, A1024, A1024, A1024
#define A16384  A4096, A4096, A4096, A4096

const unsigned char many[20480 + 1] = { A16384, A4096, 99 };

unsigned char last(void) { return many[20480]; }
