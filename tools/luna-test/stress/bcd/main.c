/* luna stress probe — 65816 decimal-mode (BCD) ADC/SBC accuracy.
 * The D flag isn't reachable from C, so the arithmetic is in bcd.asm. Results
 * (value + P-flags byte per case) land in bcd_res for a luna/Mesen2 diff vs
 * the documented 65816 decimal semantics. Transitory stress prototype. */
#include <snes.h>
u8 bcd_res[20];
extern void bcd_probe(void);
int main(void){ consoleInit(); bcd_probe(); while(1){} return 0; }
