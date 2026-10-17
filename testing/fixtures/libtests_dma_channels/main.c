/**
 * @file main.c
 * @brief Fixture: the NMI's DMAs leave the HDMA channels alone.
 *
 * A Super FX letterbox runs on HDMA channel 1 (gsuSetupHdmaBlanking), the
 * game runs five HDMA channels of its own (2 to 6), and every frame the NMI
 * flushes text and uploads four 32x32 dynamic sprites. Until 2026-10-17 the
 * text flush programmed channel 1 as a general DMA to $2118, and the sprite
 * uploads channels 1 to 4: at the next frame the letterbox read the text
 * buffer as its table and the screen stayed in forced blank (audit 1.0,
 * 01-1). The DMA registers are readable; after forty frames of all this,
 * the fixture reads back each HDMA channel's mode, destination and table.
 */
#include <snes.h>
#include <snes/superfx.h>

extern u8 gsu_hdma_table[];

/** @brief one HDMA entry: 1 line of COLDATA = 0 (no colour), then the end */
static const u8 coldata_table[] = { 1, 0xE0, 0 };

/** @brief tiles for the dynamic sprites: their content does not matter */
static const u8 sprite_tiles[2048] = { 0x55 };

/** @brief per channel: (BBAD << 8) | DMAP, read back from $43n0/$43n1 */
u16 r_ctrl[7];
/** @brief per channel: A1T, the table address the channel was given */
u16 r_table[7];
/** @brief 1 when every HDMA channel still holds what its owner set */
u16 r_channels_ok;
/** @brief frames the loop ran */
u16 r_frames;
/** @brief marker: the run reached its end */
u16 r_done;

static void read_channels(void) {
    u16 ch, ok = 1;
    for (ch = 1; ch <= 6; ch++) {
        volatile u8 *r = (volatile u8 *)(0x4300 + (ch << 4));
        r_ctrl[ch] = (u16)r[0] | ((u16)r[1] << 8);
        r_table[ch] = (u16)r[2] | ((u16)r[3] << 8);
    }
    if (r_ctrl[1] != 0x0000 || r_table[1] != (u16)(u32)gsu_hdma_table)
        ok = 0;
    for (ch = 2; ch <= 6; ch++)
        if (r_ctrl[ch] != 0x3200 || r_table[ch] != (u16)(u32)coldata_table)
            ok = 0;
    r_channels_ok = ok;
}

int main(void) {
    static const OamDynamicConfig dyn = {
        .vramLarge = 0x0000, .vramSmall = 0x1000,
        .slotLargeInit = 0, .slotSmallInit = 0,
        .sizeMode = OBJ_SIZE16_L32,
    };
    u16 id;

    consoleInit();
    setMode(BG_MODE1, 0);
    textInit(0x5000, 0, 0);
    oamDynamicInit(&dyn);
    for (id = 0; id < 4; id++) {
        oambuffer[id].oamx = 32 + (id << 5);
        oambuffer[id].oamy = 100;
        oambuffer[id].oamframeid = 0;
        oambuffer[id].oamattribute = OBJ_PRIO(3);
        oambuffer[id].oamrefresh = 1;
        OAM_SET_GFX(id, sprite_tiles);
    }
    gsuSetupHdmaBlanking(16, 16);
    for (id = HDMA_CHANNEL_2; id <= HDMA_CHANNEL_6; id++) {
        hdmaSetup((u8)id, HDMA_MODE_1REG, 0x32, coldata_table);
        hdmaEnable((u8)id);
    }
    setMainScreen(LAYER_BG1 | LAYER_OBJ);
    setScreenOn();

    for (r_frames = 0; r_frames < 40; r_frames++) {
        textPrintAt(0, 0, (r_frames & 1) ? "HUD" : "hud");
        for (id = 0; id < 4; id++) {
            oambuffer[id].oamrefresh = 1;
            oamDynamicDraw(id);
        }
        WaitForVBlank();
    }
    read_channels();
    r_done = 0xD0E5;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
