/**
 * @file console.c
 * @brief OpenSNES Console Functions Implementation
 *
 * Core console initialization and VBlank synchronization.
 *
 * Attribution:
 *   Based on PVSnesLib console.c by Alekmaul
 *   License: zlib (compatible with MIT)
 *   Modifications:
 *     - Simplified implementation
 *     - Added documentation
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>

/*============================================================================
 * External Variables (defined in crt0.asm)
 *============================================================================*/

/* vblank_flag and oam_update_flag are declared in <snes/system.h> (via <snes.h>) */
extern volatile u16 frame_count;
extern volatile u8 nmi_callback[4];     /* 24-bit function pointer + padding (PVSnesLib compatible) */
extern volatile u8 nmi_has_callback;    /* 0 = default no-op, 1 = user callback */
extern void DefaultNmiCallback(void);  /* Default callback in crt0.asm */
extern volatile u8 irq_callback[4];     /* 24-bit raw IRQ handler pointer + padding */
extern volatile u8 nmitimen_shadow;     /* Software copy of write-only $4200 */
extern void DefaultIrqHandler(void);   /* Default IRQ handler (ack+rti) in crt0.asm */
extern void unmaskIrq(void);           /* ASM helper: CLI (crt0 boots with SEI) */
extern void clearIrqFlag(void);        /* ASM helper: read $4211 to drop a pending IRQ */

/*============================================================================
 * Static Variables
 *============================================================================*/

/** Current screen brightness (0-15), defaults to full brightness.
 *  Initialized here so setScreenOn() works without consoleInit().
 *  External linkage so the `inline getBrightness()` in console.h
 *  can access it from any TU. */
u8 current_brightness = 15;

/** Force blank state shadow (REG_INIDISP is write-only, can't read back).
 *  1 = force blanked (screen off), 0 = screen on.
 *  Starts at 1 because consoleInit() sets force blank first. */
/* External linkage so the `inline setScreenOff()` in console.h can
 * access it from any TU. */
u8 force_blanked = 1;

/** PAL/NTSC flag */
static u8 is_pal_system;

/** Random seed */
static u16 rand_seed;

/*============================================================================
 * Console Initialization
 *============================================================================*/

void consoleInit(void) {
    /* Force blank - MUST be first to allow safe PPU register writes */
    REG_INIDISP = INIDISP_FORCE_BLANK;

    /* Detect PAL/NTSC */
    is_pal_system = (REG_STAT78 & 0x10) ? TRUE : FALSE;

    /* Set default brightness (screen still blanked) */
    current_brightness = 15;

    /* Initialize random seed from hardware.
     * OPHCT/OPVCT are 2-read registers (low byte then high bit) with an
     * internal read pointer per register. Reading each ONCE for entropy
     * leaves both pointers mid-sequence, silently corrupting every later
     * latch read (H-IRQ handlers, profileScanline). The STAT78 read that
     * follows resets both pointers — it must stay AFTER the counter reads. */
    rand_seed = REG_OPHCT | (REG_OPVCT << 8);
    rand_seed ^= REG_STAT78;
    if (rand_seed == 0) rand_seed = 0xACE1;

    /* Set up Mode 1 as default */
    REG_BGMODE = BGMODE_MODE1;

    /* Set default BG memory layout.
     * Without this, crt0 leaves $2107-$210C at zero, which puts both
     * tilemap and tile data at VRAM $0000 (overlapping = garbage).
     *
     * Default layout:
     *   BG1 tilemap at VRAM $0400 (BG1SC = $04, 32x32)
     *   BG1 tile data at VRAM $0000 (BG12NBA low nibble = $0)
     *
     * Users can override with bgSetMapPtr() / bgSetGfxPtr() after init.
     * Direct register writes (not bgSet*) to avoid depending on the
     * background module — but through the REG_* names like everywhere else.
     */
    REG_BG1SC = 0x04;    /* tilemap at VRAM $0400, 32x32 */
    REG_BG12NBA = 0x00;  /* BG1 tiles at VRAM $0000 */

    /* Disable mosaic effect (register can have garbage on power-up) */
    REG_MOSAIC = 0;

    /* Clear palettes to black */
    REG_CGADD = 0;
    u16 i;
    for (i = 0; i < 256; i++) {
        REG_CGDATA = 0;
        REG_CGDATA = 0;
    }

    /* Enable NMI (VBlank interrupt) and auto-joypad read. consoleInit is a
     * full console reset, so timer IRQ bits are deliberately dropped —
     * resync the shadow rather than routing through it. */
    nmitimen_shadow = NMITIMEN_NMI_ENABLE | NMITIMEN_JOY_ENABLE;
    REG_NMITIMEN = nmitimen_shadow;
}

void consoleInitEx(u16 options) {
    /* For now, just call standard init */
    consoleInit();
    (void)options;  /* Reserved for future use */
}

/*============================================================================
 * Screen Control
 *============================================================================*/

/* setScreenOn() is `inline` in console.h. Force-emit canonical here. */
void (*const __opensnes_force_emit_setScreenOn)(void) = setScreenOn;

/* Force standalone emission of the inline setScreenOff in this TU.
 * Taking the function's address creates a data-section indirect
 * reference; the QBE inline pass counts it and suppresses the
 * "header-only inclusion" suppress rule, ensuring this TU emits the
 * canonical fallback body for non-inlining callers and fn-ptr users. */
void (*const __opensnes_force_emit_setScreenOff)(void) = setScreenOff;

void setBrightness(u8 brightness) {
    current_brightness = brightness & 0x0F;
    /* Only update hardware if screen is on (not force blanked).
     * REG_INIDISP ($2100) is write-only — use shadow variable. */
    if (!force_blanked) {
        REG_INIDISP = current_brightness;
    }
}

void fadeOut(u8 speed) {
    s8 brightness;
    u8 i;

    for (brightness = 15; brightness >= 0; brightness--) {
        setBrightness((u8)brightness);
        for (i = 0; i < speed; i++) {
            WaitForVBlank();
        }
    }
}

void fadeIn(u8 speed) {
    u8 brightness;
    u8 i;

    for (brightness = 0; brightness <= 15; brightness++) {
        setBrightness(brightness);
        for (i = 0; i < speed; i++) {
            WaitForVBlank();
        }
    }
}

/* getBrightness() is `inline` in console.h. Force-emit the standalone
 * in this TU via address-taking, mirror of setScreenOff's pattern. */
u8 (*const __opensnes_force_emit_getBrightness)(void) = getBrightness;

/*============================================================================
 * VBlank Synchronization
 *============================================================================*/

/* WaitForVBlank() is now implemented in assembly (crt0.asm) using WAI
 * instruction for power savings and reduced bus contention (Opt 1). */

u8 isInVBlank(void) {
    return (REG_HVBJOY & 0x80) ? TRUE : FALSE;
}

/*============================================================================
 * Frame Counter
 *============================================================================*/

/* getFrameCount() is `inline` in console.h. Force-emit canonical here. */
u16 (*const __opensnes_force_emit_getFrameCount)(void) = getFrameCount;

void resetFrameCount(void) {
    frame_count = 0;
}

/*============================================================================
 * System Information
 *============================================================================*/

u8 isPAL(void) {
    return is_pal_system;
}

u8 getRegion(void) {
    return is_pal_system ? 1 : 0;
}

/*============================================================================
 * Random Number Generation
 *============================================================================*/

u16 rand(void) {
    /* 16-bit LFSR (Linear Feedback Shift Register) */
    /* Polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
    u16 bit = ((rand_seed >> 0) ^ (rand_seed >> 2) ^
               (rand_seed >> 3) ^ (rand_seed >> 5)) & 1;
    rand_seed = (rand_seed >> 1) | (bit << 15);
    return rand_seed;
}

void srand(u16 seed) {
    rand_seed = seed;
    if (rand_seed == 0) rand_seed = 0xACE1;  /* Avoid zero state */
}

/*============================================================================
 * Video Mode
 *============================================================================*/

void setMode(u8 mode, u8 flags) {
    /* BGMODE register format: 4321pmmm
     * 4,3,2,1 = BG tile size (0=8x8, 1=16x16)
     * p = BG3 priority in Mode 1 (0=normal, 1=high)
     * mmm = Mode (0-7)
     */
    REG_BGMODE = (flags & 0xF8) | (mode & 0x07);
}

/*============================================================================
 * VBlank Callback
 *============================================================================*/

/* Assembly helper to read REG_RDNMI - compiler optimizes away volatile reads */
extern void clearNmiFlag(void);

void nmiSetBank(VBlankCallback callback, u8 bank) {
    /* Disable NMI during pointer write to prevent partial reads */
    REG_NMITIMEN = 0;

    /* Store 24-bit callback pointer (little-endian: addr_lo, addr_hi, bank) */
    nmi_callback[0] = (u16)callback & 0xFF;
    nmi_callback[1] = ((u16)callback >> 8) & 0xFF;
    nmi_callback[2] = bank;
    nmi_callback[3] = 0x00;  /* Padding */

    /* Fast flag for NMI: skip callback entirely when default no-op */
    nmi_has_callback = 1;

    /* Clear NMI flag to prevent spurious interrupt */
    clearNmiFlag();

    /* Restore from the shadow — preserves any H/V timer IRQ bits enabled
     * via irqEnable() instead of clobbering them with a literal. */
    REG_NMITIMEN = nmitimen_shadow;
}

void nmiSet(VBlankCallback callback) {
    /* cc65816 passes 16-bit pointers — bank byte is always 0.
     * Use nmiSetBank() for callbacks in other banks. */
    nmiSetBank(callback, 0);
}

void nmiClear(void) {
    /* Restore default callback and clear fast flag */
    nmiSetBank((VBlankCallback)DefaultNmiCallback, 0);
    nmi_has_callback = 0;
}

/*============================================================================
 * Hardware IRQ (H/V timer) — see interrupt.h for the raw-handler contract
 *============================================================================*/

void irqSetBank(void *handler, u8 bank) {
    /* Mask timer IRQ sources during the pointer write so a mid-update
     * JML [irq_callback] can't read a half-written vector. NMI stays on. */
    REG_NMITIMEN = nmitimen_shadow & (u8)~(IRQ_HTIMER | IRQ_VTIMER);

    irq_callback[0] = (u16)handler & 0xFF;
    irq_callback[1] = ((u16)handler >> 8) & 0xFF;
    irq_callback[2] = bank;
    irq_callback[3] = 0x00;

    REG_NMITIMEN = nmitimen_shadow;
}

void irqSet(void *handler) {
    /* cc65816 code lives in bank 0 by default; use irqSetBank otherwise. */
    irqSetBank(handler, 0);
}

void irqClear(void) {
    irqSetBank((void *)DefaultIrqHandler, 0);
}

void irqSetHTimer(u16 h) {
    REG_HTIMEL = (u8)h;
    REG_HTIMEH = (u8)(h >> 8);
}

void irqSetVTimer(u16 v) {
    REG_VTIMEL = (u8)v;
    REG_VTIMEH = (u8)(v >> 8);
}

void irqEnable(u8 flags) {
    nmitimen_shadow = (nmitimen_shadow & (u8)~(IRQ_HTIMER | IRQ_VTIMER))
                    | (flags & (IRQ_HTIMER | IRQ_VTIMER));
    REG_NMITIMEN = nmitimen_shadow;
    unmaskIrq(); /* crt0 boots with SEI; timer IRQs also need the I flag clear */
}

void irqDisable(void) {
    nmitimen_shadow &= (u8)~(IRQ_HTIMER | IRQ_VTIMER);
    REG_NMITIMEN = nmitimen_shadow;
    /* Drop any already-latched timer IRQ so it can't fire one last time. */
    clearIrqFlag();
}

/*============================================================================
 * SETINI ($2133, write-only) — see video.h for the setter contracts
 *============================================================================*/

extern volatile u8 setini_shadow; /* crt0 sysvar, zeroed at boot */

static void setiniWrite(u8 bit, u8 on) {
    if (on)
        setini_shadow |= bit;
    else
        setini_shadow &= (u8)~bit;
    REG_SETINI = setini_shadow;
}

void videoSetInterlace(u8 on)    { setiniWrite(0x01, on); }
void videoSetObjInterlace(u8 on) { setiniWrite(0x02, on); }
void videoSetOverscan(u8 on)     { setiniWrite(0x04, on); }
void videoSetPseudoHires(u8 on)  { setiniWrite(0x08, on); }
