/**
 * @file main.c
 * @brief Drum loop from the DSP noise generator — no samples at all
 * @ingroup examples
 *
 * Port of krom (Peter Lemon)'s PlayNoise demo (PeterLemon/SNES,
 * SPC700/PlayNoise). The S-DSP has a built-in white-noise source: set
 * a voice's NON flag and the voice plays noise instead of a sample.
 * Each drum of the loop (kick, closed hi-hat, open hi-hat, snare) is
 * nothing but a noise clock (FLG bits 0-4), an ADSR envelope, and a
 * key-on — the whole kit ships zero bytes of sample data.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - The S-DSP noise generator (NON + the FLG noise clock)
 * - ADSR envelopes as percussion design (attack/release = drum type)
 * - FIR echo (ESA/EDL/EFB/FIR0) for the drum room
 * - The apu module upload path (apuWaitBoot/apuUpload/apuExecute)
 *
 * @par What to Observe
 * A four-hit drum bar loops: kick, closed hat, open hat, snare.
 * Validation oracle: luna --audio-out (see README).
 *
 * @par Modules Used
 * console, apu
 *
 * @see https://github.com/PeterLemon/SNES — original demo
 * @see player.spc700.asm — the whole kit
 */

#include <snes.h>
#include <snes/apu.h>

extern u8 spc_image[], spc_image_end[];

#define SPC_BASE 0x0200

int main(void) {
    consoleInit();

    apuWaitBoot();
    apuUpload(spc_image, SPC_BASE, (u16)(spc_image_end - spc_image));
    apuExecute(SPC_BASE);

    setColor(0, RGB(0, 12, 12));
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
