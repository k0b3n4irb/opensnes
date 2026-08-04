/**
 * @file main.c
 * @brief One-shot sound effects from WAV files — the wav2brr pipeline.
 *
 * The smallest "press a button, hear a sound" example, and a showcase of the
 * zero-config .wav -> .brr build rule: `res/blip.wav` and `res/coin.wav` are
 * plain PCM WAV files. data.asm .incbin's them as `blip.brr` / `coin.brr`,
 * and the build system converts each with wav2brr automatically — no .brr is
 * committed, they are generated on demand like a .pic from a .png.
 *
 * See docs/tutorials/audio.md ("One-shot samples from WAV") and
 * tools/wav2brr for the converter.
 *
 * Init order: textModeInit (display), audioInit, load samples, setScreenOn.
 * Modules: console, dma, audio, input, background, text.
 *
 * @see lib/include/snes/audio.h — audioLoadSample / audioPlaySample
 */
#include <snes.h>
#include <snes/audio.h>
#include <snes/input.h>
#include <snes/text.h>

/* Symbols from data.asm — the .brr blobs the build system generated. */
extern u8 blip_brr[], blip_brr_end[];
extern u8 coin_brr[], coin_brr_end[];

#define SMP_BLIP 0
#define SMP_COIN 1

int main(void) {
    textModeInit();
    audioInit();
    setColor(0, RGB(2, 4, 12));

    /* Load each sample once into the SPC700; 0 loop point = one-shot. */
    audioLoadSample(SMP_BLIP, blip_brr, (u16)(blip_brr_end - blip_brr), 0);
    audioLoadSample(SMP_COIN, coin_brr, (u16)(coin_brr_end - coin_brr), 0);

    textPrintAt(6, 4, "SFX FROM WAV");
    textPrintAt(4, 8, "A - BLIP");
    textPrintAt(4, 10, "B - COIN");
    textPrintAt(3, 16, "WAV2BRR: RES/*.WAV");

    WaitForVBlank();
    setScreenOn();

    while (1) {
        WaitForVBlank();

        u16 keys = padPressed(0);   /* edge-triggered: one play per press */
        if (keys & KEY_A)
            audioPlaySample(SMP_BLIP);
        if (keys & KEY_B)
            audioPlaySample(SMP_COIN);
    }
    return 0;
}
