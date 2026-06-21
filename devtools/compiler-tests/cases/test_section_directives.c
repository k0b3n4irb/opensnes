/* Verifies cc65816 emits .ACCU 16 and .INDEX 16 at the start of every
 * function's .SECTION. WLA-DX defaults to 8-bit register tracking per
 * object file; without these directives, immediate-operand instructions
 * like `cpx #imm` assemble at the wrong width and every subsequent
 * address shifts by one byte (the variable-shift bug from 2026-Q1).
 *
 * The directives must appear:
 *   1. After the .SECTION line for the function
 *   2. Before the function label
 *
 * This test compiles a trivial function and verifies the layout.
 */

unsigned short trivial(unsigned short x) {
    return x + 1;
}
