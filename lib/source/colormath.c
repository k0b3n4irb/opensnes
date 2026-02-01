/**
 * @file colormath.c
 * @brief OpenSNES Color Math Implementation
 *
 * Color math for transparency, shadows, and blending effects.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>
#include <snes/colormath.h>

/*============================================================================
 * Color Math Registers
 *
 * $2130 - CGWSEL: Color math control register A
 *   Bits 7-6: Force main screen black (0=never, 1=outside window, 2=inside, 3=always)
 *   Bits 5-4: Color math enable (0=always, 1=inside window, 2=outside, 3=never)
 *   Bit 1: Sub screen color source (0=sub screen, 1=fixed color)
 *   Bit 0: Direct color mode for 256-color BG
 *
 * $2131 - CGADSUB: Color math control register B
 *   Bit 7: 0=add, 1=subtract
 *   Bit 6: Half mode (divide result by 2)
 *   Bits 5-0: Enable color math for layers (same as TM/TS bits)
 *
 * $2132 - COLDATA: Fixed color data
 *   Bit 7: Apply to blue
 *   Bit 6: Apply to green
 *   Bit 5: Apply to red
 *   Bits 4-0: Intensity (0-31)
 *============================================================================*/

/* Internal state */
static u8 cgwsel;
static u8 cgadsub;

/*============================================================================
 * Core Color Math Functions
 *============================================================================*/

void colorMathInit(void) {
    cgwsel = 0;
    cgadsub = 0;

    REG_CGWSEL = 0;
    REG_CGADSUB = 0;
    REG_COLDATA = 0;
}

void colorMathEnable(u8 layers) {
    /* Set layer enable bits (bits 0-5 of CGADSUB) */
    cgadsub = (cgadsub & 0xC0) | (layers & 0x3F);
    REG_CGADSUB = cgadsub;
}

void colorMathDisable(void) {
    cgadsub &= 0xC0;  /* Clear layer bits */
    REG_CGADSUB = cgadsub;
}

void colorMathSetOp(u8 op) {
    if (op == COLORMATH_SUB) {
        cgadsub |= 0x80;  /* Subtract mode */
    } else {
        cgadsub &= ~0x80; /* Add mode */
    }
    REG_CGADSUB = cgadsub;
}

void colorMathSetHalf(u8 enable) {
    if (enable) {
        cgadsub |= 0x40;  /* Half mode on */
    } else {
        cgadsub &= ~0x40; /* Half mode off */
    }
    REG_CGADSUB = cgadsub;
}

void colorMathSetSource(u8 source) {
    if (source == COLORMATH_SRC_FIXED) {
        cgwsel |= 0x02;  /* Use fixed color */
    } else {
        cgwsel &= ~0x02; /* Use sub screen */
    }
    REG_CGWSEL = cgwsel;
}

void colorMathSetCondition(u8 condition) {
    /* Bits 5-4 control when color math is enabled */
    cgwsel = (cgwsel & 0xCF) | ((condition & 0x03) << 4);
    REG_CGWSEL = cgwsel;
}

void colorMathSetFixedColor(u8 r, u8 g, u8 b) {
    /* Write each channel separately with channel select bits */
    REG_COLDATA = COLDATA_RED | (r & 0x1F);
    REG_COLDATA = COLDATA_GREEN | (g & 0x1F);
    REG_COLDATA = COLDATA_BLUE | (b & 0x1F);
}

void colorMathSetChannel(u8 channel, u8 intensity) {
    REG_COLDATA = channel | (intensity & 0x1F);
}

/*============================================================================
 * Color Math Effect Helpers
 *============================================================================*/

void colorMathTransparency50(u8 layers) {
    /* Enable add mode with half = 50% blend */
    colorMathEnable(layers);
    colorMathSetOp(COLORMATH_ADD);
    colorMathSetHalf(1);
    colorMathSetSource(COLORMATH_SRC_SUBSCREEN);
}

void colorMathShadow(u8 layers, u8 intensity) {
    /* Subtract fixed gray to darken */
    colorMathEnable(layers);
    colorMathSetOp(COLORMATH_SUB);
    colorMathSetHalf(0);
    colorMathSetSource(COLORMATH_SRC_FIXED);
    colorMathSetFixedColor(intensity, intensity, intensity);
}

void colorMathTint(u8 layers, u8 r, u8 g, u8 b) {
    /* Add fixed color to tint */
    colorMathEnable(layers);
    colorMathSetOp(COLORMATH_ADD);
    colorMathSetHalf(0);
    colorMathSetSource(COLORMATH_SRC_FIXED);
    colorMathSetFixedColor(r, g, b);
}

void colorMathSetBrightness(u8 brightness) {
    /* Set uniform brightness for fading */
    colorMathSetFixedColor(brightness, brightness, brightness);
}
