/* OpenSNES: `__ramcode` (RAM_CODE in snes/types.h, 2026-09-30) puts a
 * function in the RAM code window: QBE emits it as a section appended to
 * ".ram_code" with BASE $7D (stored in ROM bank 1, labels at $7E:xxxx),
 * guarded by an assembler .FAIL when the project has no window
 * (RAM_CODE_SIZE = 0). A plain function keeps its SUPERFREE .text section.
 * The keyword on a prototype carries to the definition. */
unsigned short __ramcode ram_step(unsigned short x);

unsigned short ram_step(unsigned short x) {
    return x + 3;
}

__ramcode void ram_loop(volatile unsigned char *flag) {
    while (*flag)
        ;
}

unsigned short rom_caller(unsigned short x) {
    return ram_step(x);
}
