/**
 * @file main.c
 * @brief Speech synthesis — the SNES says "OPEN SNES"
 * @ingroup examples
 *
 * Port of krom (Peter Lemon)'s SpeechSynth demo (PeterLemon/SNES,
 * SPC700/SpeechSynth). Speech on the SNES = sequencing a PHONEME BANK
 * on one DSP voice: per phoneme, the APU-side program selects the
 * sample (SRCN), shapes it (ADSR: plosives one-shot, vowels sustained),
 * inflects the pitch (prosody) and times the wait — krom's bank spells
 * "PETER LEMON"; our original formant-synthesized bank says
 * "OPEN SNES" on loop.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - Phoneme-bank speech synthesis on the S-DSP (no CPU streaming)
 * - Per-phoneme ADSR/SRCN/pitch sequencing from SPC700 code
 * - The apu module upload path (apuWaitBoot/apuUpload/apuExecute)
 *
 * @par What to Observe
 * The console speaks. Validation oracle: luna --audio-out (see README).
 *
 * @par Modules Used
 * console, apu
 *
 * @see https://github.com/PeterLemon/SNES — original demo
 * @see gen_phonemes.py, player.spc700.asm
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

    setColor(0, RGB(8, 0, 16));
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
