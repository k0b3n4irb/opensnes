/**
 * @file main.c
 * @brief Hardware vibrato — one voice's output modulates another's pitch
 * @ingroup examples
 *
 * Port of krom (Peter Lemon)'s PitchMod demo (PeterLemon/SNES,
 * SPC700/PitchMod). The S-DSP's PMON feature multiplies a voice's
 * pitch by the OUTPUT of the previous voice. Voice 1 holds a cello
 * note at C5; voice 0 plays a tiny looping square wave at pitch $0003
 * — inaudibly slow, it is a hardware LFO. One second in, the LFO keys
 * on and the cello starts to sing with vibrato: no CPU, no tables,
 * the DSP does the modulation every sample.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - Pitch modulation (PMON): voice N-1's output scales voice N's pitch
 * - A silent voice as a hardware LFO (GAIN = depth, PITCH = rate)
 * - Long FIR echo (EDL 15) as a concert hall
 * - The apu module upload path (apuWaitBoot/apuUpload/apuExecute)
 *
 * @par What to Observe
 * The cello enters dry; after ~1 s the vibrato blooms. The note rings
 * out like a bow stroke (the sample's own ADSR: instant attack, gentle
 * ~7 s fade) and is re-bowed every 7 s, forever. Validation oracle:
 * luna --audio-out.
 *
 * @par Modules Used
 * console, apu
 *
 * @see https://github.com/PeterLemon/SNES — original demo
 * @see player.spc700.asm — voices, PMON routing, echo
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

    setColor(0, RGB(16, 8, 0));
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
