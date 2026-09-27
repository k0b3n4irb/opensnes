/*
 * libtest_sa1_sram — the sram module on an SA-1 cartridge (2026-09-26).
 *
 * The save memory of an SA-1 cart is BW-RAM, which the SNES CPU sees at
 * $40-$4F and may write only after SBWE ($2226) bit 7 is set; crt0 does it
 * now. The test script asserts the C-side round trip AND the bytes where the
 * hardware puts them ($40:0000 + offset): a self-consistent wrong bank would
 * pass the first and fail the second.
 */
#include <snes.h>
#include <snes/sram.h>

static const u8 tpl[12] = { 0xC1, 0xD2, 0xE3, 0xF4, 0x15, 0x26, 0x37, 0x48, 0x59, 0x6A, 0x7B, 0x8C };
u8 back[12];
u8 back_off[4];
u16 r_rt;        /* bytes equal after sramSave / sramLoad of 12        -> 12 */
u16 r_off;       /* sramSaveOffset(tpl+4, 4, 0x123) / LoadOffset: [0]  -> 0x15 */
u16 r_off3;      /*                                              [3]  -> 0x48 */
u16 r_ck;        /* sramChecksum(tpl, 12)                              -> 0x8C */
u16 r_ok;        /* sramSave(tpl, 12)                                  -> SRAM_OK (0) */
u16 r_range;     /* sramSaveOffset(tpl, 12, 0x7FF8): past the 32 KB the header declares -> SRAM_ERR_RANGE (1) */
u16 r_clear;     /* OR of the first 12 bytes after sramClear(12)       -> 0 */
u16 r_done;      /*                                                    -> 0xBEEF */
/* What the battery held at power-on, read before anything is written: zero
 * on a fresh cart, the previous run's pattern (C1 D2 E3 F4) once a .srm is
 * loaded — the power-cycle chain in tools/luna-test/power_cycle. */
u8 r_boot[4];

int main(void) {
    u8 i;
    consoleInit();

    sramLoad(r_boot, 4);
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

    r_range = sramSaveOffset(tpl, 12, 0x7FF8);
    r_ok = sramSave(tpl, 12);   /* leave the pattern in BW-RAM for the peeks */
    setScreenOn();
    r_done = 0xBEEF;
    while (1) { WaitForVBlank(); }
    return 0;
}
