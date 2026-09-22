/*
 * libtest_hirom — the sram module on a HiROM cartridge.
 *
 * Asymmetric, non-zero data on purpose. The manifest
 * (tools/luna-test/manifests/sram_hirom.toml) additionally asserts the bytes
 * where the HARDWARE puts them, $30:6000 + offset, and in the .srm luna
 * writes — the C-side round trip alone would pass with any self-consistent
 * wrong bank.
 */
#include <snes.h>
#include <snes/sram.h>

static const u8 tpl[12] = { 0xC1, 0xD2, 0xE3, 0xF4, 0x15, 0x26, 0x37, 0x48, 0x59, 0x6A, 0x7B, 0x8C };
u8 back[12];
u8 back_off[4];
u16 r_rt;        /* bytes equal after sramSave / sramLoad of 12        -> 12 */
u16 r_off;       /* sramSaveOffset(tpl+4, 4, 0x123) / LoadOffset: [0]  -> 0x15 */
u16 r_off3;      /*                                              [3]  -> 0x48 */
u16 r_ck;        /* sramChecksum(tpl, 12)                              -> XOR of the 12 */
u16 r_ok;        /* sramSave(tpl, 12) on HiROM                          -> SRAM_OK (0) */
u16 r_range;     /* sramSaveOffset(tpl, 12, 0x1FF8): past the 8 KB window -> SRAM_ERR_RANGE (1) */
u16 r_clear;     /* OR of the first 12 bytes after sramClear(12)       -> 0 */
u16 r_bank_ram;  /* bank byte the compiler gives a pointer to a bank-0 RAM variable */
u16 r_bank_rom;  /* ... and to a const table */
u16 r_done;      /*                                                    -> 0xBEEF */

int main(void) {
    u8 i;
    consoleInit();

    r_bank_ram = (u16)((u32)(void *)back >> 16);
    r_bank_rom = (u16)((u32)(const void *)tpl >> 16);
    sramSave(tpl, 12);
    sramLoad(back, 12);
    r_rt = 0;
    for (i = 0; i < 12; i++) if (back[i] == tpl[i]) r_rt++;

    sramSaveOffset(tpl + 4, 4, 0x123);
    sramLoadOffset(back_off, 4, 0x123);
    r_off  = back_off[0];
    r_off3 = back_off[3];
    r_ck   = sramChecksum(tpl, 12);

    sramClear(12);
    sramLoad(back, 12);
    r_clear = 0;
    for (i = 0; i < 12; i++) r_clear |= back[i];

    r_range = sramSaveOffset(tpl, 12, 0x1FF8);
    r_ok = sramSave(tpl, 12);   /* leave the pattern in SRAM for the manifest */
    setScreenOn();
    r_done = 0xBEEF;
    while (1) { WaitForVBlank(); }
    return 0;
}
