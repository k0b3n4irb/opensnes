/**
 * @file main.c
 * @brief Fixture: a 4 MB LoROM image, its last bank read back.
 *
 * ROM_BANKS=128 gives 128 banks of 32 KB. C const data goes to the highest
 * bank first, so the table below sits in bank 127, addressed $FF:8000 (the
 * code is linked from $80, clear of the WRAM at $7E/$7F). The fixture reads
 * it back through the far read C uses for const data, and records the
 * header's ROM size byte.
 */
#include <snes.h>

static const u8 table[16] = {
    0x5A, 0xA5, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
    0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE
};

/** @brief the table's bank, from its address */
u16 r_table_bank;
/** @brief table[0] + table[15] read back: $5A + $EE = $148 */
u16 r_table_sum;
/** @brief header ROM size byte ($FFD7): $0C for 4 MB */
u16 r_header_size;
/** @brief marker: the run reached its end */
u16 r_done;

int main(void) {
    const u8 *p = table;
    consoleInit();
    r_table_bank = (u16)((u32)p >> 16);
    r_table_sum = (u16)table[0] + (u16)table[15];
    r_header_size = *(volatile u8 *)0xFFD7;
    r_done = 0xBEEF;
    setScreenOn();
    for (;;) {
        WaitForVBlank();
    }
    return 0;
}
