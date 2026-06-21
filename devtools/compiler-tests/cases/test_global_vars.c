/**
 * Test: Global variable reads use direct addressing
 *
 * BUG: Compiler was generating indirect addressing for global variable reads:
 *   lda.w #symbol    ; Load address
 *   tax
 *   lda.l $0000,x    ; Indirect load - WRONG
 *
 * FIXED: Should generate direct addressing:
 *   lda.l symbol     ; Direct load - CORRECT
 */

/* Global variables */
volatile unsigned short global_x;
volatile unsigned short global_y;
volatile unsigned char global_state;

/* Extern variables (simulates accessing vars defined in assembly) */
extern unsigned short extern_x;
extern unsigned short extern_y;

void read_globals(void) {
    unsigned short local_x, local_y;

    /* These reads should use lda.l global_x, NOT lda.l $0000,x */
    local_x = global_x;
    local_y = global_y;

    /* Read-modify-write pattern */
    global_x = global_x + 1;
    global_y = global_y - 1;
}

void read_externs(void) {
    unsigned short local_x, local_y;

    /* These reads should use lda.l extern_x, NOT lda.l $0000,x */
    local_x = extern_x;
    local_y = extern_y;

    /* Read-modify-write pattern */
    extern_x = extern_x + 1;
    extern_y = extern_y - 1;
}

unsigned short get_global_sum(void) {
    /* Multiple reads in expression */
    return global_x + global_y;
}
