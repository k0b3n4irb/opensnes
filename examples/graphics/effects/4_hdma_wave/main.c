/**
 * HDMA Wave Demo
 *
 * Demonstrates HDMA wave distortion effects for water reflections,
 * heat shimmer, or dream sequences.
 *
 * Controls:
 *   A: Toggle wave effect on/off
 *   LEFT/RIGHT: Adjust wave amplitude (4 levels)
 *   UP: Start wave animation
 *   DOWN: Stop wave animation (freeze)
 */
#include <snes.h>
#include <snes/console.h>
#include <snes/hdma.h>
#include <snes/background.h>
#include <snes/input.h>
#include <snes/text.h>

/* Simple tiles for visual pattern */
static const u8 tiles[] = {
    /* Tile 0: empty */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* Tile 1: vertical stripe */
    0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00,
    0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF0, 0x00,
};

/* Amplitude levels (pixels of displacement) */
static const u8 amplitudeLevels[] = { 2, 4, 8, 16 };

/* State variables */
static u8 wave_enabled;
static u8 animating;
static u8 amplitudeLevel;  /* 0-3 index into amplitudeLevels */

/* Display current settings */
void updateDisplay(void) {
    /* Line 1: Wave status */
    if (wave_enabled) {
        textPrintAt(1, 25, "WAVE: ON ");
    } else {
        textPrintAt(1, 25, "WAVE: OFF");
    }

    /* Line 2: Animation status */
    if (animating) {
        textPrintAt(1, 26, "ANIM: RUNNING");
    } else {
        textPrintAt(1, 26, "ANIM: STOPPED");
    }

    /* Line 3: Amplitude */
    textSetPos(1, 27);
    textPrint("AMPLITUDE: ");
    textPrintU16(amplitudeLevels[amplitudeLevel]);
    textPrint("px ");
}

/* Apply wave settings with current amplitude */
void applyWaveSettings(void) {
    if (wave_enabled) {
        hdmaWaveStop();
        hdmaWaveH(HDMA_CHANNEL_6, 1, amplitudeLevels[amplitudeLevel], 2);
        hdmaWaveSetSpeed(2);
        hdmaEnable(1 << HDMA_CHANNEL_6);
    }
}

int main(void) {
    u16 i;

    /* Initialize state */
    wave_enabled = 1;
    animating = 1;
    amplitudeLevel = 1;  /* Start at level 1 (4 pixels) */

    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

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

    /* Setup palette: color 0 = dark blue, color 1 = cyan */
    REG_CGADD = 0;
    REG_CGDATA = 0x00; REG_CGDATA = 0x50;  /* Dark blue background */
    REG_CGDATA = 0xFF; REG_CGDATA = 0x7F;  /* Cyan stripes */

    /* Fill tilemap with vertical stripes (alternating tiles) */
    REG_VMADDL = 0x00;
    REG_VMADDH = 0x04;
    for (i = 0; i < 1024; i++) {
        REG_VMDATAL = ((i % 32) & 1);  /* Alternate by column */
        REG_VMDATAH = 0;
    }

    /* Enable BG1 on main screen */
    REG_TM = TM_BG1;

    /* Initialize and start HDMA wave effect */
    hdmaWaveInit();
    applyWaveSettings();

    textPrintAt(1, 1, "HDMA WAVE DEMO");
    textPrintAt(1, 3, "Controls:");
    textPrintAt(1, 4, "  A: Toggle wave on/off");
    textPrintAt(1, 5, "  LEFT/RIGHT: Amplitude");
    textPrintAt(1, 6, "  UP: Start animation");
    textPrintAt(1, 7, "  DOWN: Stop animation");
    updateDisplay();

    setScreenOn();

    while (1) {
        WaitForVBlank();
        padUpdate();

        /* A: Toggle wave on/off */
        if (padPressed(0) & KEY_A) {
            wave_enabled = !wave_enabled;
            if (wave_enabled) {
                applyWaveSettings();
            } else {
                hdmaWaveStop();
                /* Reset scroll to remove distortion */
                REG_BG1HOFS = 0;
                REG_BG1HOFS = 0;
            }
            updateDisplay();
        }

        /* RIGHT: Increase amplitude */
        if (padPressed(0) & KEY_RIGHT) {
            if (amplitudeLevel < 3) {
                amplitudeLevel++;
                applyWaveSettings();
                updateDisplay();
            }
        }

        /* LEFT: Decrease amplitude */
        if (padPressed(0) & KEY_LEFT) {
            if (amplitudeLevel > 0) {
                amplitudeLevel--;
                applyWaveSettings();
                updateDisplay();
            }
        }

        /* UP: Start animation */
        if (padPressed(0) & KEY_UP) {
            if (!animating) {
                animating = 1;
                updateDisplay();
            }
        }

        /* DOWN: Stop animation (freeze) */
        if (padPressed(0) & KEY_DOWN) {
            if (animating) {
                animating = 0;
                updateDisplay();
            }
        }

        /* Update wave animation only if animating */
        if (wave_enabled && animating) {
            hdmaWaveUpdate();
        }
    }

    return 0;
}
