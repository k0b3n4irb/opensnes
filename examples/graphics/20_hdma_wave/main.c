/**
 * HDMA Wave Demo - Indirect Mode
 *
 * Uses indirect HDMA with a small rotating data table for efficient
 * wave animation without visual corruption.
 *
 * Controls:
 *   A: Toggle HDMA effect on/off (off = straight vertical lines)
 *   LEFT/RIGHT: Change wave amplitude (4 levels)
 *   UP: Start animation
 *   DOWN: Stop animation
 */
#include <snes.h>

/* Assembly functions (defined in hdma_wave_asm.asm) */
extern void hdmaWaveInit(void);
extern void hdmaWaveSetup(void);
extern void hdmaWaveEnable(void);
extern void hdmaWaveDisable(void);
extern void hdmaWaveRotate(void);
extern void hdmaWaveSetAmplitude(u8 level);
extern void hdmaWaveStartAnimation(void);
extern void hdmaWaveStopAnimation(void);

/* Simple tiles for visual pattern */
static const u8 tiles[] = {
    /* Tile 0: empty */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Tile 1: solid */
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
};

/* Use static variables for more reliable code generation */
static u16 pad, pad_old;
static u8 amplitude;
static u8 hdma_enabled;

int main(void) {
    u16 i;

    /* Initialize state */
    pad = 0;
    pad_old = 0;
    amplitude = 1;      /* Start at medium amplitude */
    hdma_enabled = 1;   /* HDMA effect enabled by default */

    consoleInit();
    setMode(BG_MODE0, 0);

    /* Setup BG1 tilemap at VRAM $0400 (word address) */
    REG_BG1SC = 0x04;
    REG_BG12NBA = 0x00;

    /* Upload tiles to VRAM $0000 */
    REG_VMAIN = 0x80;
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x00;
    for (i = 0; i < sizeof(tiles); i += 2) {
        REG_VMDATAL = tiles[i];
        REG_VMDATAH = tiles[i + 1];
    }

    /* Setup palette: color 0 = dark blue, color 1 = light blue */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x50;  /* Dark blue */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x5A;  /* Light blue */

    /* Fill tilemap with vertical stripes (alternating tiles 0 and 1) */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x04;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = ((i % 32) & 1);  /* Alternate by column */
        REG_VMDATAH = 0;
    }

    /* Enable BG1 on main screen */
    REG_TM = TM_BG1;

    /* Initialize HDMA wave system (once) */
    hdmaWaveInit();
    hdmaWaveSetup();
    hdmaWaveEnable();

    setScreenOn();

    while (1) {
        WaitForVBlank();

        /* Read controller directly as 16-bit (more reliable with HDMA) */
        while (REG_HVBJOY & 0x01) {}  /* Wait for auto-read complete */
        pad = *(volatile u16*)0x4218;

        /* RIGHT: increase amplitude (held, not edge triggered for testing) */
        if (pad & KEY_RIGHT) {
            if (amplitude < 3) {
                amplitude++;
                hdmaWaveSetAmplitude(amplitude);
            }
        }

        /* LEFT: decrease amplitude (edge triggered) */
        if ((pad & KEY_LEFT) && !(pad_old & KEY_LEFT)) {
            if (amplitude > 0) {
                amplitude--;
                hdmaWaveSetAmplitude(amplitude);
            }
        }

        /* UP: start animation (edge triggered) */
        if ((pad & KEY_UP) && !(pad_old & KEY_UP)) {
            hdmaWaveStartAnimation();
        }

        /* DOWN: stop animation (edge triggered) */
        if ((pad & KEY_DOWN) && !(pad_old & KEY_DOWN)) {
            hdmaWaveStopAnimation();
        }

        /* A: toggle HDMA effect on/off (edge triggered) */
        if ((pad & KEY_A) && !(pad_old & KEY_A)) {
            if (hdma_enabled) {
                hdmaWaveDisable();
                /* Reset BG1 horizontal scroll to 0 for straight lines */
                REG_BG1HOFS = 0;
                REG_BG1HOFS = 0;  /* Write twice (16-bit register) */
                hdma_enabled = 0;
            } else {
                hdmaWaveEnable();
                hdma_enabled = 1;
            }
        }

        pad_old = pad;

        /* Update wave animation (fast assembly, safe to call every frame) */
        hdmaWaveRotate();
    }

    return 0;
}
