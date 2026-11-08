/**
 * @file window.c
 * @brief OpenSNES Window/Masking Implementation
 *
 * Window system for masking portions of layers.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>
#include <snes/window.h>

/* Screen center X coordinate (256 pixels wide / 2) */
#define SCREEN_CENTER_X 128

/*============================================================================
 * Window Registers
 *
 * $2123 - W12SEL: Window 1/2 settings for BG1/BG2
 *   Bits 7-6: BG2 Window 2 (enable, invert)
 *   Bits 5-4: BG2 Window 1 (enable, invert)
 *   Bits 3-2: BG1 Window 2 (enable, invert)
 *   Bits 1-0: BG1 Window 1 (enable, invert)
 *
 * $2124 - W34SEL: Window 1/2 settings for BG3/BG4
 *   Same format as W12SEL
 *
 * $2125 - WOBJSEL: Window 1/2 settings for OBJ/MATH
 *   Bits 7-6: MATH Window 2
 *   Bits 5-4: MATH Window 1
 *   Bits 3-2: OBJ Window 2
 *   Bits 1-0: OBJ Window 1
 *
 * $2126 - WH0: Window 1 left position
 * $2127 - WH1: Window 1 right position
 * $2128 - WH2: Window 2 left position
 * $2129 - WH3: Window 2 right position
 *
 * $212A - WBGLOG: Window logic for BG1-4
 *   Bits 7-6: BG4 logic
 *   Bits 5-4: BG3 logic
 *   Bits 3-2: BG2 logic
 *   Bits 1-0: BG1 logic
 *
 * $212B - WOBJLOG: Window logic for OBJ/MATH
 *   Bits 3-2: MATH logic
 *   Bits 1-0: OBJ logic
 *
 * $212E - TMW: Main screen window mask designation
 * $212F - TSW: Sub screen window mask designation
 *============================================================================*/

/* Shadows of the write-only window registers. Not static, and prefixed,
 * since ppuReset() (ppu_reset.asm) brings them back to their power-on value
 * with the registers. */
u8 window_w12sel;
u8 window_w34sel;
u8 window_wobjsel;
u8 window_wbglog;
u8 window_wobjlog;

/*============================================================================
 * Core Window Functions
 *============================================================================*/

void windowInit(void) {
    /* Clear all window settings */
    window_w12sel = 0;
    window_w34sel = 0;
    window_wobjsel = 0;
    window_wbglog = 0;
    window_wobjlog = 0;

    REG_W12SEL = 0;
    REG_W34SEL = 0;
    REG_WOBJSEL = 0;
    REG_WH0 = 0;
    REG_WH1 = 0;
    REG_WH2 = 0;
    REG_WH3 = 0;
    REG_WBGLOG = 0;
    REG_WOBJLOG = 0;
    REG_TMW = 0;
    REG_TSW = 0;
}

void windowSetPos(u8 window, u8 left, u8 right) {
    if (window == WINDOW_1) {
        REG_WH0 = left;
        REG_WH1 = right;
    } else {
        REG_WH2 = left;
        REG_WH3 = right;
    }
}

void windowEnable(u8 window, u8 layers) {
    u8 shift;
    u8 mask;

    /* Window 1 uses bits 0/1, Window 2 uses bits 2/3 */
    shift = (window == WINDOW_1) ? 0 : 2;
    mask = 0x02 << shift;  /* Enable bit (not invert) */

    /* BG1 */
    if (layers & LAYER_BG1) {
        window_w12sel |= mask;
    }

    /* BG2 */
    if (layers & LAYER_BG2) {
        window_w12sel |= (mask << 4);
    }

    /* BG3 */
    if (layers & LAYER_BG3) {
        window_w34sel |= mask;
    }

    /* BG4 */
    if (layers & LAYER_BG4) {
        window_w34sel |= (mask << 4);
    }

    /* OBJ */
    if (layers & LAYER_OBJ) {
        window_wobjsel |= mask;
    }

    /* MATH */
    if (layers & WINDOW_MATH) {
        window_wobjsel |= (mask << 4);
    }

    /* Write to hardware */
    REG_W12SEL = window_w12sel;
    REG_W34SEL = window_w34sel;
    REG_WOBJSEL = window_wobjsel;
}

void windowDisable(u8 window, u8 layers) {
    u8 shift;
    u8 mask;

    shift = (window == WINDOW_1) ? 0 : 2;
    mask = 0x03 << shift;  /* Both enable and invert bits */

    if (layers & LAYER_BG1) {
        window_w12sel &= ~mask;
    }
    if (layers & LAYER_BG2) {
        window_w12sel &= ~(mask << 4);
    }
    if (layers & LAYER_BG3) {
        window_w34sel &= ~mask;
    }
    if (layers & LAYER_BG4) {
        window_w34sel &= ~(mask << 4);
    }
    if (layers & LAYER_OBJ) {
        window_wobjsel &= ~mask;
    }
    if (layers & WINDOW_MATH) {
        window_wobjsel &= ~(mask << 4);
    }

    REG_W12SEL = window_w12sel;
    REG_W34SEL = window_w34sel;
    REG_WOBJSEL = window_wobjsel;
}

void windowDisableAll(void) {
    window_w12sel = 0;
    window_w34sel = 0;
    window_wobjsel = 0;

    REG_W12SEL = 0;
    REG_W34SEL = 0;
    REG_WOBJSEL = 0;
    REG_TMW = 0;
    REG_TSW = 0;
}

void windowSetInvert(u8 window, u8 layers, u8 invert) {
    u8 shift;
    u8 invertBit;

    shift = (window == WINDOW_1) ? 0 : 2;
    invertBit = invert ? (0x01 << shift) : 0;

    if (layers & LAYER_BG1) {
        window_w12sel = (window_w12sel & ~(0x01 << shift)) | invertBit;
    }
    if (layers & LAYER_BG2) {
        window_w12sel = (window_w12sel & ~(0x10 << shift)) | (invertBit << 4);
    }
    if (layers & LAYER_BG3) {
        window_w34sel = (window_w34sel & ~(0x01 << shift)) | invertBit;
    }
    if (layers & LAYER_BG4) {
        window_w34sel = (window_w34sel & ~(0x10 << shift)) | (invertBit << 4);
    }
    if (layers & LAYER_OBJ) {
        window_wobjsel = (window_wobjsel & ~(0x01 << shift)) | invertBit;
    }
    if (layers & WINDOW_MATH) {
        window_wobjsel = (window_wobjsel & ~(0x10 << shift)) | (invertBit << 4);
    }

    REG_W12SEL = window_w12sel;
    REG_W34SEL = window_w34sel;
    REG_WOBJSEL = window_wobjsel;
}

void windowSetLogic(u8 layer, u8 logic) {
    switch (layer) {
        case LAYER_BG1:
            window_wbglog = (window_wbglog & 0xFC) | (logic & 0x03);
            break;
        case LAYER_BG2:
            window_wbglog = (window_wbglog & 0xF3) | ((logic & 0x03) << 2);
            break;
        case LAYER_BG3:
            window_wbglog = (window_wbglog & 0xCF) | ((logic & 0x03) << 4);
            break;
        case LAYER_BG4:
            window_wbglog = (window_wbglog & 0x3F) | ((logic & 0x03) << 6);
            break;
        case LAYER_OBJ:
            window_wobjlog = (window_wobjlog & 0xFC) | (logic & 0x03);
            break;
        case WINDOW_MATH:
            window_wobjlog = (window_wobjlog & 0xF3) | ((logic & 0x03) << 2);
            break;
    }

    REG_WBGLOG = window_wbglog;
    REG_WOBJLOG = window_wobjlog;
}

void windowSetMainMask(u8 layers) {
    REG_TMW = layers;
}

void windowSetSubMask(u8 layers) {
    REG_TSW = layers;
}

/*============================================================================
 * Window Effect Helpers
 *============================================================================*/

void windowCentered(u8 window, u8 width) {
    u8 half;
    u8 left;
    u8 right;

    half = width >> 1;
    left = SCREEN_CENTER_X - half;
    /* right from left and the width, not from the centre: an odd width
     * kept its full span, and a width of 1 gave right < left, an empty
     * window (until 2026-10-03). width 0 is an empty window on purpose. */
    if (width == 0) {
        windowSetPos(window, 1, 0);
        return;
    }
    right = (u8)(left + width - 1);

    windowSetPos(window, left, right);
}

void windowSplit(u8 splitX) {
    /* Window 1: left side (0 to splitX-1); splitX = 0 means no left part,
     * and an empty window is left > right (0..0 was one pixel wide, until
     * 2026-10-03) */
    if (splitX == 0) {
        windowSetPos(WINDOW_1, 1, 0);
    } else {
        windowSetPos(WINDOW_1, 0, splitX - 1);
    }

    /* Window 2: right side (splitX to 255) */
    windowSetPos(WINDOW_2, splitX, 255);
}
