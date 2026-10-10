#include <snes.h>
void f(void) {
    x = WINDOW_BG1;
    x = WINDOW_BG2;
    x = WINDOW_BG3;
    x = WINDOW_BG4;
    x = WINDOW_OBJ;
    x = COLORMATH_BG1;
    x = COLORMATH_BG2;
    x = COLORMATH_BG3;
    x = COLORMATH_BG4;
    x = COLORMATH_OBJ;
    x = MOSAIC_BG1;
    x = MOSAIC_BG2;
    x = MOSAIC_BG3;
    x = MOSAIC_BG4;
    x = BGMODE_MODE0;
    x = BGMODE_MODE1;
    x = BGMODE_MODE2;
    x = BGMODE_MODE3;
    x = BGMODE_MODE7;
    x = OAM_SET_GFX_BANK;
    x = audioUpdate(1);
    x = colorMathEnable(1);
    x = consoleInitEx(1);
    x = getRegion(1);
    x = rand(1);
    x = srand(1);
    x = dmaCopyVramBank(1);
    x = dmaCopyCGramBank(1);
    x = dsp1Parameter(1);
    x = dsp1Present(1);
    x = hdmaSetupBank(1);
    x = padRaw(1);
    x = scopeButtonsDown(1);
    x = nmiSetBank(1);
    x = irqSetBank(1);
    x = LzssDecodeVram(1);
    x = ease_in_quad(1);
    x = ease_out_quad(1);
    x = mode7SetPivot(1);
    x = mosaicEnable(1);
    x = profileGetFrameCount(1);
    x = sa1Init(1);
    x = snesmodSetSoundTable(1);
    x = snesmodAllocateSoundRegion(1);
    x = oamDrawMeta(1);
    x = oamDrawMetaFlip(1);
    hdmaEnable(0x40); hdmaDisable(1 << 2);   // hdmaEnable in a comment
    // padRaw(0) commented out
    dmaTransfer(0, 1, 2, 3, 4);
    mode7SetScale(0x200, 0x200); mode7Transform(50, 100);
    my_padRaw(1); padRawish(); xrand(); WINDOW_BG1X;
}
