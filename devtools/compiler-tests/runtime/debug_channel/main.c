/**
 * @file main.c
 * @brief Runtime self-test for the SDK debug channel (snes/debug.h).
 *
 * Nothing in the example corpus uses SNES_NOCASH / SNES_ASSERT, so this ROM is
 * the only coverage of lib/source/debug.asm. It exercises both channels luna
 * can capture headlessly:
 *   - consoleNocashMessage() writes a known string byte-by-byte to $21FC
 *     (captured by `luna run --nocash-out`).
 *   - SNES_ASSERT(false) → SNES_BREAK() → WDM $00 (captured by `--wdm-out`).
 *
 * Not an example (no README/screenshot) — a devtools runtime harness, run by
 * test_debug_channel.py and wired into `make tests`.
 */
#include <snes.h>
#include <snes/debug.h>

int main(void) {
    consoleNocashMessage("LUNA_DBG_OK");  /* → $21FC Nocash-TTY stream */
    SNES_ASSERT(1 == 2);                  /* false → SNES_BREAK() → WDM $00 */
    while (1) {
    }
    return 0;
}
