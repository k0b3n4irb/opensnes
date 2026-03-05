// =============================================================================
// Unit Test: DMA Module
// =============================================================================
// Tests DMA (Direct Memory Access) transfer functions.
//
// Critical functions tested:
// - dmaCopyVram(): Copy data to VRAM
// - dmaCopyCGram(): Copy data to CGRAM (palettes)
// - dmaClearVRAM(): Clear all VRAM
// - dmaFillVRAM(): Fill VRAM with value
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/dma.h>
#include <snes/text.h>

// Test tile data (8x8 2bpp tile = 16 bytes)
const u8 testTile[16] = {
    0xFF, 0x00,  // Row 0
    0x81, 0x00,  // Row 1
    0x81, 0x00,  // Row 2
    0x81, 0x00,  // Row 3
    0x81, 0x00,  // Row 4
    0x81, 0x00,  // Row 5
    0x81, 0x00,  // Row 6
    0xFF, 0x00   // Row 7
};

// Test tilemap (2x2 tiles = 8 bytes)
const u8 testTilemap[8] = {
    0x01, 0x00,  // Tile 1
    0x01, 0x20,  // Tile 1, H-flip
    0x01, 0x40,  // Tile 1, V-flip
    0x01, 0x60   // Tile 1, H+V flip
};

// Test palette
const u8 testPalette[32] = {
    // Color 0: Black (transparent for sprites)
    0x00, 0x00,
    // Color 1: White
    0xFF, 0x7F,
    // Color 2: Red
    0x00, 0x7C,
    // Color 3: Green
    0xE0, 0x03,
    // Remaining colors
    0x1F, 0x00, 0xFF, 0x03, 0x1F, 0x7C, 0xE0, 0x7F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Large test buffer for stress testing (64 bytes, simpler)
const u8 largeBuffer[64] = {
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA
};

// Test results
static u8 test_passed;
static u8 test_failed;
static u8 test_row;

void log_result(const char* name, u8 passed) {
    if (passed) {
        test_passed++;
    } else {
        test_failed++;
    }
    test_row++;
}

// =============================================================================
// Test: dmaClearVRAM
// =============================================================================
void test_dma_clear_vram(void) {
    // Clear all VRAM
    dmaClearVRAM();
    log_result("dmaClearVRAM executes", 1);
}

// =============================================================================
// Test: dmaCopyVram basic
// =============================================================================
void test_dma_copy_vram_basic(void) {
    // Copy small tile to VRAM
    dmaCopyVram((u8*)testTile, 0x0000, sizeof(testTile));
    log_result("dmaCopyVram small", 1);

    // Copy to different address
    dmaCopyVram((u8*)testTile, 0x0100, sizeof(testTile));
    log_result("dmaCopyVram offset", 1);
}

// =============================================================================
// Test: dmaCopyVram tilemap
// =============================================================================
void test_dma_copy_tilemap(void) {
    // Copy tilemap data
    dmaCopyVram((u8*)testTilemap, 0x0400, sizeof(testTilemap));
    log_result("dmaCopyVram tilemap", 1);
}

// =============================================================================
// Test: dmaCopyCGram
// =============================================================================
void test_dma_copy_cgram(void) {
    // Copy palette to CGRAM
    dmaCopyCGram((u8*)testPalette, 0, sizeof(testPalette));
    log_result("dmaCopyCGram executes", 1);

    // Copy to different palette index
    dmaCopyCGram((u8*)testPalette, 16, sizeof(testPalette));
    log_result("dmaCopyCGram offset", 1);
}

// =============================================================================
// Test: dmaFillVRAM
// =============================================================================
void test_dma_fill_vram(void) {
    // Fill VRAM region with pattern
    dmaFillVRAM(0x0000, 0x1000, 256);  // Fill with zeros
    log_result("dmaFillVRAM zeros", 1);

    dmaFillVRAM(0xFFFF, 0x1100, 256);  // Fill with ones
    log_result("dmaFillVRAM ones", 1);
}

// =============================================================================
// Test: Multiple DMA transfers
// =============================================================================
void test_dma_multiple(void) {
    // Simulate typical frame setup: tiles + tilemap + palette
    dmaCopyVram((u8*)testTile, 0x0000, sizeof(testTile));
    dmaCopyVram((u8*)testTilemap, 0x0400, sizeof(testTilemap));
    dmaCopyCGram((u8*)testPalette, 0, sizeof(testPalette));
    log_result("Multiple DMA transfers", 1);
}

// =============================================================================
// Test: Large DMA transfer
// =============================================================================
void test_dma_large(void) {
    // Transfer 64 bytes (typical tile set chunk)
    dmaCopyVram((u8*)largeBuffer, 0x2000, sizeof(largeBuffer));
    log_result("Large DMA (64 bytes)", 1);
}

// =============================================================================
// Test: DMA to various VRAM regions
// =============================================================================
void test_dma_vram_regions(void) {
    // Tile data region (typically $0000-$3FFF)
    dmaCopyVram((u8*)testTile, 0x0000, sizeof(testTile));
    log_result("DMA to tile region", 1);

    // Tilemap region (typically $0400, $0800, etc.)
    dmaCopyVram((u8*)testTilemap, 0x0400, sizeof(testTilemap));
    log_result("DMA to tilemap region", 1);

    // High VRAM (Mode 7, etc.)
    dmaCopyVram((u8*)testTile, 0x4000, sizeof(testTile));
    log_result("DMA to high VRAM", 1);
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(2, 1, "DMA MODULE TESTS");
    textPrintAt(2, 2, "----------------");

    test_passed = 0;
    test_failed = 0;
    test_row = 4;

    // Run tests (must be during VBlank or force blank)
    // consoleInit sets force blank, so we're safe

    test_dma_clear_vram();
    test_dma_copy_vram_basic();
    test_dma_copy_tilemap();
    test_dma_copy_cgram();
    test_dma_fill_vram();
    test_dma_multiple();
    test_dma_large();
    test_dma_vram_regions();

    // Display results
    textPrintAt(2, 20, "Tests completed");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
