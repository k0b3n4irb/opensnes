/*---------------------------------------------------------------------------------

    Transparency between 2 BGs demo
    Ported from PVSnesLib (alekmaul) to OpenSNES

    BG1 = land tiles (4bpp, palette slot 1)
    BG3 = cloud tiles (2bpp, palette slot 0), auto-scrolling
    Color math: add subscreen (BG3) to BG1 + backdrop

---------------------------------------------------------------------------------*/
#include <snes.h>

extern u8 land_tiles[], land_tiles_end[];
extern u8 land_map[], land_map_end[];
extern u8 land_pal[], land_pal_end[];
extern u8 cloud_tiles[], cloud_tiles_end[];
extern u8 cloud_map[], cloud_map_end[];
extern u8 cloud_pal[], cloud_pal_end[];

int main(void) {
    u16 scrX = 0;

    consoleInit();

    /* BG1 = land, 4bpp tiles at VRAM $0000, palette slot 1 (16 colors) */
    bgInitTileSet(0, land_tiles, land_pal, 1,
                  land_tiles_end - land_tiles,
                  land_pal_end - land_pal,
                  BG_16COLORS, 0x0000);

    /* BG3 = clouds, 2bpp tiles at VRAM $1000, palette slot 0 (4 colors) */
    bgInitTileSet(2, cloud_tiles, cloud_pal, 0,
                  cloud_tiles_end - cloud_tiles,
                  cloud_pal_end - cloud_pal,
                  BG_4COLORS, 0x1000);

    /* Load land tilemap at VRAM $2000 */
    bgSetMapPtr(0, 0x2000, BG_MAP_32x32);
    dmaCopyVram(land_map, 0x2000, land_map_end - land_map);

    /* Load cloud tilemap at VRAM $2400 */
    bgSetMapPtr(2, 0x2400, BG_MAP_32x32);
    dmaCopyVram(cloud_map, 0x2400, cloud_map_end - cloud_map);

    /* Mode 1 with BG3 high priority (clouds above land) */
    setMode(BG_MODE1, BG3_MODE1_PRIORITY_HIGH);

    /* Only BG1 and BG3 on main screen (disable BG2) */
    REG_TM = TM_BG1 | TM_BG3;
    setScreenOn();

    /* BG3 on sub screen for color math blending */
    REG_TS = TM_BG3;

    /*
     * Color math setup (direct register writes for clarity):
     *
     * REG_CGWSEL ($2130):
     *   bit 1 = 1: use sub screen BG/OBJ as color math source
     *
     * REG_CGADSUB ($2131):
     *   bit 7 = 0: add mode (not subtract)
     *   bit 5 = 1: apply to backdrop
     *   bit 0 = 1: apply to BG1
     *   = 0x21
     */
    REG_CGWSEL = 0x02;
    REG_CGADSUB = 0x21;

    while (1) {
        /* Auto-scroll clouds to the right */
        scrX++;
        bgSetScroll(2, scrX, 0);

        WaitForVBlank();
    }
    return 0;
}
