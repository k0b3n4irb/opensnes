/**
 * @file main.c
 * @brief DSP-1 ground: the Super Mario Kart floor — per-scanline Mode 7
 *        matrices streamed by the coprocessor's Raster command.
 * @ingroup examples
 *
 * mode7/perspective fakes a receding floor with a precomputed M7A/M7D table;
 * this example asks the DSP-1 for the real thing. Each frame the CPU sends
 * the camera (position, height, heading, tilt) with dsp1Parameter, then
 * dsp1Raster streams one 2x2 matrix per screen line — A, B, C and D, so the
 * floor rotates with the heading for free — straight into two HDMA payloads.
 * The HDMA channels replay them next frame while the CPU already computes
 * the frame after (double buffering). Above the horizon the same HDMA split
 * as mode7/perspective shows a Mode 3 sky.
 *
 * The three numbers dsp1Parameter hands back beside the matrices are the
 * whole geometry: Vva says on which raster the horizon sits, Cx/Cy say which
 * ground point is under the screen centre (they go to M7X/M7Y), and the
 * Mode 7 scroll registers just pin that centre to the middle of the screen.
 *
 * @par SNES Concepts
 * - DSP-1 Raster: open-ended command streaming Mode 7 matrices per raster
 * - Parameter outputs (Vof, Vva, Cx, Cy) driving M7X/M7Y and the HDMA split
 * - HDMA_MODE_2REG_2X repeat blocks feeding M7A/M7B and M7C/M7D
 * - Double-buffered HDMA tables swapped with hdmaSetTable in VBlank
 * - dsp1Triangle as the movement sin/cos
 *
 * @par What to Observe
 * - A textured ground plane receding to a horizon under a sky
 * - D-pad Left/Right turn the camera: the floor rotates in true perspective
 *   and the sky pans with the heading (one turn = the 512-px sky map)
 * - D-pad Up/Down drive forward/backward along the heading
 *
 * @par Modules Used
 * console, dma, background, input, mode7, hdma, dsp1
 *
 * @warning Needs the DSP-1 firmware in luna (dsp1b.rom) — see the README.
 * @see snes/dsp1.h, docs/tutorials/dsp1.md, examples/mode7/perspective
 */
#include <snes.h>
#include <snes/dsp1.h>

/** @name Assets (data.asm, ASSET_SECTION — any bank, DMA reads the far pointer)
 *  @{ */
extern u8 ground_tiles[], ground_tiles_end[];
extern u8 ground_map[], ground_map_end[];
extern u8 ground_pal[], ground_pal_end[];
extern u8 sky_tiles[], sky_tiles_end[];
extern u8 sky_map[], sky_map_end[];
/** @} */

/** @brief Camera height above the ground plane (DSP-1 integer units). Lower
 *  = more dramatic near-row magnification; SMK sits low like this. */
#define CAM_HEIGHT   80
/** @brief Base point → viewpoint distance; the viewer sits this far behind
 *  the camera point along the view line. */
#define CAM_LFE      96
/** @brief Viewpoint → screen distance: 256 gives the 50-degree horizontal
 *  FOV of a 256-pixel screen, and it is the vertical focal length that turns
 *  the tilt into a horizon raster (Vva = -256*tan(tilt)). */
#define CAM_LES      256
/** @brief Zenith angle of the view line: 0x4000 is horizontal (horizon at the
 *  screen centre); 0x4000 - 655 tilts down 3.6 degrees so the horizon lands
 *  16 lines above centre (line 96) and the ground below fits one 126-line
 *  HDMA repeat block. */
#define CAM_AZS      (0x4000 - 655)
/** @brief Heading change per frame while turning (16-bit angle, 2^16 = 360). */
#define TURN_STEP    0x0100
/** @brief Forward speed in ground units per frame. */
#define MOVE_STEP    4

/** @brief Largest HDMA repeat block (7-bit count). The ground uses one. */
#define GROUND_MAX   127
/** @brief Payload offset in a matrix table: [sky count][4 sky bytes][repeat header]. */
#define TABLE_HDR    6
/** @brief One matrix table: sky entry, repeat block of GROUND_MAX rasters, terminator. */
#define TABLE_SIZE   (TABLE_HDR + GROUND_MAX * 4 + 1)

/** @brief HDMA channels: BGMODE switch, TM switch, M7A/M7B, M7C/M7D.
 *  1-4 like mode7/perspective (0 is the general-DMA channel, 7 the OAM one). */
#define CH_MODE  HDMA_CHANNEL_1
#define CH_TM    HDMA_CHANNEL_2
#define CH_AB    HDMA_CHANNEL_3
#define CH_CD    HDMA_CHANNEL_4

/** @brief Double-buffered M7A/M7B tables: HDMA reads one while the DSP-1 fills the other. */
static FAR u8 tab_ab[2][TABLE_SIZE];
/** @brief Double-buffered M7C/M7D tables. */
static FAR u8 tab_cd[2][TABLE_SIZE];
/** @brief BGMODE table: Mode 3 for the sky lines, then Mode 7 for the rest. */
static u8 tab_mode[5];
/** @brief TM table: BG2 (sky) for the sky lines, then BG1 (ground). */
static u8 tab_tm[5];

/** @brief Camera position on the ground plane (wraps with the 1024-unit texture). */
static s16 cam_x, cam_y;
/** @brief Heading: 0 looks up the map (-Y), 0x4000 looks to +X. */
static u16 cam_aas;
/** @brief Geometry from the last dsp1Parameter: imaginary-centre raster, horizon raster. */
static s16 vof, vva;
/** @brief Ground point under the imaginary centre (last dsp1Parameter). */
static s16 cx, cy;
/** @brief Screen line of the first ground raster and the block length. */
static u8 sky_lines, ground_lines;
/** @brief Table set being filled this frame (the other one is on screen). */
static u8 back;

/** @brief 1 when the DSP-1 answered the known-answer probe (luna manifest asserts it). */
volatile u16 dsp1_ok = 0;
/** @brief Rasters streamed in the last dsp1Raster call (luna manifest asserts it). */
volatile u16 raster_lines = 0;

/**
 * @brief Build one matrix table skeleton: a sky entry holding the identity
 *        matrix, then the repeat-block header the DSP-1 payload sits under.
 * @param t     table to initialise
 * @param sky   scanlines above the ground (non-repeat entry, written once)
 * @param lines ground rasters (repeat entry: 4 bytes written every line)
 * @param diag  value of the diagonal term in the sky entry (A or D)
 */
static void tableInit(u8 FAR *t, u8 sky, u8 lines, u16 diag) {
    t[0] = sky;
    t[1] = (u8)diag;  t[2] = (u8)(diag >> 8);   /* A (or C): identity 1.0 / 0 */
    t[3] = 0;         t[4] = 0;                 /* B (or D) sky value: see caller */
    t[5] = (u8)(0x80 | lines);                  /* repeat: the DSP-1 payload */
    t[TABLE_HDR + (u16)lines * 4] = 0;          /* terminator */
}

/**
 * @brief Re-run the projection for the current camera and cache what the
 *        rest of the frame needs (horizon raster, ground centre).
 */
static void cameraUpdate(void) {
    dsp1Parameter(cam_x, cam_y, CAM_HEIGHT, CAM_LFE, CAM_LES, cam_aas, (u16)CAM_AZS);
    vof = dsp1_o0;
    vva = dsp1_o1;
    cx  = dsp1_o2;
    cy  = dsp1_o3;
}

int main(void) {
    u16 pad;
    s16 h;

    consoleInit();
    setScreenOff();

    /* Ground texture: Mode 7 interleaved VRAM (map in low bytes, pixels in
     * high bytes), 256-colour palette. Sky: Mode 3 BG2 tiles at $5000, map
     * at $4000, palette 0 = the first 16 ground colours. */
    dmaCopyVramMode7(ground_map, ground_map_end - ground_map,
                     ground_tiles, ground_tiles_end - ground_tiles);
    dmaCopyCGram(ground_pal, 0, ground_pal_end - ground_pal);
    dmaCopyVram(sky_map, 0x4000, sky_map_end - sky_map);
    dmaCopyVram(sky_tiles, 0x5000, sky_tiles_end - sky_tiles);
    REG_BG2SC = 0x41;      /* BG2 map at $4000, 64x32 */
    REG_BG12NBA = 0x50;    /* BG1 tiles $0000 (Mode 7), BG2 tiles $5000 */

    setMode(BG_MODE7, 0);
    mode7Init();

    dsp1Init();
    dsp1_ok = dsp1Present();
    cam_x = 512; cam_y = 512; cam_aas = 0;
    cameraUpdate();

    /* The tilt is fixed, so the horizon raster is too: split the screen once.
     * Raster Vva+1 is the singular horizon line and Vva+2 the first finite
     * ground raster; rasters are relative to the imaginary centre (line
     * 112 + Vof). */
    h = 112 + vof + vva + 2;
    if (h < 1) h = 1;
    if (h > 223) h = 223;
    sky_lines = (u8)h;
    ground_lines = (u8)(224 - h);
    if (ground_lines > GROUND_MAX) ground_lines = GROUND_MAX;

    tab_mode[0] = sky_lines; tab_mode[1] = 0x09;   /* Mode 3, BG1 16px (unused) */
    tab_mode[2] = 1;         tab_mode[3] = 0x07;   /* Mode 7 */
    tab_mode[4] = 0;
    tab_tm[0] = sky_lines;   tab_tm[1] = 0x02;     /* BG2 = sky */
    tab_tm[2] = 1;           tab_tm[3] = 0x01;     /* BG1 = ground */
    tab_tm[4] = 0;
    tableInit(tab_ab[0], sky_lines, ground_lines, 0x0100);
    tableInit(tab_ab[1], sky_lines, ground_lines, 0x0100);
    tableInit(tab_cd[0], sky_lines, ground_lines, 0);
    tableInit(tab_cd[1], sky_lines, ground_lines, 0);
    tab_cd[0][3] = 0; tab_cd[0][4] = 1;            /* sky D = 1.0 */
    tab_cd[1][3] = 0; tab_cd[1][4] = 1;

    /* First frame's matrices, straight into buffer 0 (screen is still off). */
    dsp1Raster(&tab_ab[0][TABLE_HDR], &tab_cd[0][TABLE_HDR], vva + 2, ground_lines);
    raster_lines = ground_lines;
    back = 1;

    hdmaSetup(CH_MODE, HDMA_MODE_1REG,    HDMA_DEST_BGMODE, tab_mode);
    hdmaSetup(CH_TM,   HDMA_MODE_1REG,    HDMA_DEST_TM,     tab_tm);
    hdmaSetup(CH_AB,   HDMA_MODE_2REG_2X, HDMA_DEST_M7A,    tab_ab[0]);
    hdmaSetup(CH_CD,   HDMA_MODE_2REG_2X, HDMA_DEST_M7C,    tab_cd[0]);
    hdmaEnable((1 << CH_MODE) | (1 << CH_TM) | (1 << CH_AB) | (1 << CH_CD));

    /* Pin the ground point under the imaginary centre to the screen middle:
     * M7X/M7Y = Cx/Cy, and the scroll puts screen (128, 112+Vof) on it. The
     * D values the chip streams are slopes relative to that line. */
    mode7SetCenter(cx, cy);
    mode7SetScroll(cx - 128, cy - (112 + vof));
    setMainScreen(LAYER_BG1);   /* the TM HDMA takes over per line */
    setScreenOn();

    while (1) {
        pad = padHeld(0);
        if (pad & KEY_LEFT)  cam_aas -= TURN_STEP;
        if (pad & KEY_RIGHT) cam_aas += TURN_STEP;
        if (pad & (KEY_UP | KEY_DOWN)) {
            /* forward = (sin, -cos) of the heading on the ground plane */
            dsp1Triangle(cam_aas, MOVE_STEP);
            if (pad & KEY_UP)  { cam_x += dsp1_o0; cam_y -= dsp1_o1; }
            else               { cam_x -= dsp1_o0; cam_y += dsp1_o1; }
            cam_x &= 1023;   /* the texture repeats every 1024 units */
            cam_y &= 1023;
        }

        /* Active display: the DSP-1 streams next frame's floor into the
         * back buffers while HDMA replays the front ones. ~50 us per raster. */
        cameraUpdate();
        dsp1Raster(&tab_ab[back][TABLE_HDR], &tab_cd[back][TABLE_HDR], vva + 2, ground_lines);
        raster_lines = ground_lines;

        WaitForVBlank();
        /* VBlank: swap tables (takes effect at the next HDMA init) and move
         * the Mode 7 centre with the camera. Mode 7 registers must not be
         * touched during active display here — the HDMA channels own the
         * write-twice latch (see KNOWN_LIMITATIONS: shared M7 latch). */
        hdmaSetTable(CH_AB, tab_ab[back]);
        hdmaSetTable(CH_CD, tab_cd[back]);
        mode7SetCenter(cx, cy);
        mode7SetScroll(cx - 128, cy - (112 + vof));
        /* The sky pans with the heading: 2^16 angle units over the 512-px map. */
        bgSetScroll(1, cam_aas >> 7, 0);
        back ^= 1;
    }
    return 0;
}
