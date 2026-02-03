/**
 * HDMA Wave Demo
 *
 * Demonstrates the hdmaWave* functions for animated wave distortion effects.
 * Commonly used for water reflections, heat shimmer, or dream sequences.
 *
 * Controls:
 *   A: Toggle wave effect on/off
 *   LEFT/RIGHT: Adjust wave amplitude (1-8)
 *   UP/DOWN: Adjust wave frequency (1-8)
 *   L/R: Adjust animation speed (1-4)
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

/* State variables */
static u8 wave_enabled;
static u8 amplitude;
static u8 frequency;
static u8 speed;

/* Display current settings */
void updateDisplay(void) {
    textSetPos(1, 26);
    textPrint("AMP:");
    textPrintU16(amplitude);
    textPrint(" FREQ:");
    textPrintU16(frequency);
    textPrint(" SPD:");
    textPrintU16(speed);
    textPrint("  ");

    if (wave_enabled) {
        textPrintAt(1, 27, "WAVE: ON ");
    } else {
        textPrintAt(1, 27, "WAVE: OFF");
    }
}

/* Reinitialize wave with current settings */
void applyWaveSettings(void) {
    if (wave_enabled) {
        hdmaWaveStop();
        hdmaWaveH(HDMA_CHANNEL_6, 1, amplitude, frequency);
        hdmaWaveSetSpeed(speed);
        hdmaEnable(1 << HDMA_CHANNEL_6);
    }
}

int main(void) {
    u16 i;

    /* Initialize state */
    wave_enabled = 1;
    amplitude = 4;
    frequency = 2;
    speed = 2;

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
    hdmaWaveH(HDMA_CHANNEL_6, 1, amplitude, frequency);
    hdmaWaveSetSpeed(speed);
    hdmaEnable(1 << HDMA_CHANNEL_6);

    textPrintAt(1, 1, "HDMA WAVE DEMO");
    textPrintAt(1, 3, "A: Toggle wave");
    textPrintAt(1, 4, "L/R: Amplitude");
    textPrintAt(1, 5, "U/D: Frequency");
    textPrintAt(1, 6, "LR Buttons: Speed");
    updateDisplay();

    setScreenOn();

    while (1) {
        WaitForVBlank();
        padUpdate();

        /* A: Toggle wave on/off */
        if (padPressed(0) & KEY_A) {
            wave_enabled = !wave_enabled;
            if (wave_enabled) {
                hdmaWaveH(HDMA_CHANNEL_6, 1, amplitude, frequency);
                hdmaWaveSetSpeed(speed);
                hdmaEnable(1 << HDMA_CHANNEL_6);
            } else {
                hdmaWaveStop();
                /* Reset scroll to remove distortion */
                REG_BG1HOFS = 0;
                REG_BG1HOFS = 0;
            }
            updateDisplay();
        }

        /* LEFT/RIGHT: Adjust amplitude */
        if (padPressed(0) & KEY_RIGHT) {
            if (amplitude < 8) {
                amplitude++;
                applyWaveSettings();
                updateDisplay();
            }
        }
        if (padPressed(0) & KEY_LEFT) {
            if (amplitude > 1) {
                amplitude--;
                applyWaveSettings();
                updateDisplay();
            }
        }

        /* UP/DOWN: Adjust frequency */
        if (padPressed(0) & KEY_UP) {
            if (frequency < 8) {
                frequency++;
                applyWaveSettings();
                updateDisplay();
            }
        }
        if (padPressed(0) & KEY_DOWN) {
            if (frequency > 1) {
                frequency--;
                applyWaveSettings();
                updateDisplay();
            }
        }

        /* L/R: Adjust speed */
        if (padPressed(0) & KEY_R) {
            if (speed < 4) {
                speed++;
                hdmaWaveSetSpeed(speed);
                updateDisplay();
            }
        }
        if (padPressed(0) & KEY_L) {
            if (speed > 1) {
                speed--;
                hdmaWaveSetSpeed(speed);
                updateDisplay();
            }
        }

        /* Update wave animation */
        if (wave_enabled) {
            hdmaWaveUpdate();
        }
    }

    return 0;
}
