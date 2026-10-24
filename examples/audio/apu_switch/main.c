/**
 * @file main.c
 * @brief Two S-DSP programs — noise drums and hardware vibrato — hot-swapped with apuReset()
 * @ingroup examples
 *
 * The apu module's upload path (apuWaitBoot/apuUpload/apuExecute) runs
 * once, at boot. This example adds the missing capability: replacing
 * the running APU program at ANY time. The method is krom (Peter
 * Lemon)'s PlayTwoSong protocol, productized: the 65816 writes
 * APU_RESET_MAGIC to I/O port 0 (apuReset()); the SPC700 program polls
 * for it in its wait loops (the APU_CHECK_RESET idiom), silences the
 * DSP, and jumps back to the IPL boot ROM — ready for the next upload.
 *
 * Press A for the drum kit, B for the vibrato cello. Both APU programs
 * are this repo's own ports of krom's demos: the drums are the DSP's
 * white-noise generator alone (NON, the FLG noise clock, one ADSR per
 * drum: krom's PlayNoise), the cello sings by pitch modulation (PMON:
 * a silent voice as a hardware LFO: krom's PitchMod). No external
 * assets beyond the cello sample already attributed.
 *
 * ROM mode: LoROM (project default).
 *
 * @par SNES Concepts
 * - Cooperative APU reset: CPU→APU messaging over $2140 after boot
 * - The IPL ROM is re-entrant — jump to $FFC0 with the ROM re-enabled
 *   and the full boot handshake ($AA/$BB, $CC upload) works again
 * - Two independent APU programs in one ROM, uploaded on demand
 * - The S-DSP noise source (NON, FLG bits 0-4) as an instrument; the
 *   ADSR envelope is the drum type
 * - Pitch modulation (PMON): voice N-1's output scales voice N's pitch
 * - FIR echo setup (ESA/EDL/EFB/FIR0) in each program
 *
 * @par What to Observe
 * Drums start immediately. Press B: the kit cuts, the cello blooms.
 * Press A: back to the drums. The backdrop tracks the active program
 * (red = drums, amber = cello). Swap latency is a few frames — the
 * SPC polls for the reset request every millisecond.
 *
 * @par Modules Used
 * console, apu, input
 *
 * @see https://github.com/PeterLemon/SNES — PlayTwoSong, the original
 *   protocol demo (its songs use samples ripped from commercial games,
 *   so this port swaps between two OpenSNES-native programs instead)
 * @see drums.spc700.asm, cello.spc700.asm — the two APU programs
 */

#include <snes.h>
#include <snes/apu.h>
#include <snes/input.h>

extern u8 spc_drums[], spc_drums_end[];
extern u8 spc_cello[], spc_cello_end[];

#define SPC_BASE 0x0200

/** @brief Active program: 0 = drums, 1 = cello (probe oracle) */
u8 current_song;

/** @brief Upload and start one of the two APU programs */
static void start_song(u8 song) {
    if (song) {
        apuUpload(spc_cello, SPC_BASE, (u16)(spc_cello_end - spc_cello));
        setColor(0, RGB(20, 12, 0));    /* amber = cello */
    } else {
        apuUpload(spc_drums, SPC_BASE, (u16)(spc_drums_end - spc_drums));
        setColor(0, RGB(20, 4, 4));     /* red = drums */
    }
    apuExecute(SPC_BASE);
    current_song = song;
}

int main(void) {
    u16 keys;

    consoleInit();
    /* BG1 is on (TM=1) with its tilemap at VRAM $0400 and no tiles are ever
     * loaded here: on real hardware VRAM powers up as garbage and would show
     * through. Clear it once in forced blank (found by luna --power-on random,
     * gaps review item R10). */
    dmaClearVRAM();

    apuWaitBoot();
    start_song(0);

    setScreenOn();

    while (1) {
        WaitForVBlank();
        keys = padPressed(0);
        if ((keys & KEY_A) && current_song != 0) {
            apuReset();
            start_song(0);
        } else if ((keys & KEY_B) && current_song != 1) {
            apuReset();
            start_song(1);
        }
    }

    return 0;
}
