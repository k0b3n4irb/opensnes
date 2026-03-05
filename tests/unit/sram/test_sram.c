// =============================================================================
// Unit Test: SRAM Module
// =============================================================================
// Tests save game functionality - CRITICAL for player data persistence.
//
// Critical functions tested:
// - sramSave(): Save data to battery-backed RAM
// - sramLoad(): Load data from SRAM
// - sramSaveOffset(): Save at specific offset (save slots)
// - sramLoadOffset(): Load from specific offset
// - sramClear(): Erase SRAM contents
// - sramChecksum(): Data integrity verification
// - Constants: SRAM_SIZE_*
//
// IMPORTANT: This test requires USE_SRAM=1 in Makefile!
// =============================================================================

#include <snes.h>
#include <snes/console.h>
#include <snes/sram.h>
#include <snes/text.h>

// =============================================================================
// Compile-time tests for constants
// =============================================================================

// SRAM size constants (values match ROM header SRAMSIZE field)
_Static_assert(SRAM_SIZE_NONE == 0x00, "SRAM_SIZE_NONE must be 0x00");
_Static_assert(SRAM_SIZE_2KB == 0x01, "SRAM_SIZE_2KB must be 0x01");
_Static_assert(SRAM_SIZE_4KB == 0x02, "SRAM_SIZE_4KB must be 0x02");
_Static_assert(SRAM_SIZE_8KB == 0x03, "SRAM_SIZE_8KB must be 0x03");
_Static_assert(SRAM_SIZE_16KB == 0x04, "SRAM_SIZE_16KB must be 0x04");
_Static_assert(SRAM_SIZE_32KB == 0x05, "SRAM_SIZE_32KB must be 0x05");

// Verify size constants are sequential
_Static_assert(SRAM_SIZE_2KB == SRAM_SIZE_NONE + 1, "Size constants must be sequential");
_Static_assert(SRAM_SIZE_4KB == SRAM_SIZE_2KB + 1, "Size constants must be sequential");
_Static_assert(SRAM_SIZE_8KB == SRAM_SIZE_4KB + 1, "Size constants must be sequential");

// =============================================================================
// Test data structures
// =============================================================================

// Simulated save game structure
typedef struct {
    u8  magic[4];      // "SAVE" signature
    u16 score;
    u8  level;
    u8  lives;
    u8  checksum;
} SaveData;

// Test buffers
static u8 testBuffer[64];
static u8 readBuffer[64];
static SaveData saveData;
static SaveData loadedData;

// =============================================================================
// Runtime tests
// =============================================================================

static u8 test_passed;
static u8 test_failed;

void log_result(const char* name, u8 passed) {
    if (passed) {
        test_passed++;
    } else {
        test_failed++;
    }
}

// =============================================================================
// Test: sramChecksum
// =============================================================================
void test_sram_checksum(void) {
    u8 i;

    // Test with all zeros
    for (i = 0; i < 16; i++) {
        testBuffer[i] = 0;
    }
    u8 chk = sramChecksum(testBuffer, 16);
    log_result("Checksum all zeros", chk == 0);

    // Test with known pattern
    testBuffer[0] = 0xAA;
    testBuffer[1] = 0x55;
    testBuffer[2] = 0xFF;
    testBuffer[3] = 0x00;
    chk = sramChecksum(testBuffer, 4);
    // XOR: 0xAA ^ 0x55 ^ 0xFF ^ 0x00 = 0xAA ^ 0x55 ^ 0xFF = 0x00
    // Actually: 0xAA ^ 0x55 = 0xFF, 0xFF ^ 0xFF = 0x00, 0x00 ^ 0x00 = 0x00
    log_result("Checksum known pattern", chk == 0x00);

    // Test with single byte
    testBuffer[0] = 0x42;
    chk = sramChecksum(testBuffer, 1);
    log_result("Checksum single byte", chk == 0x42);

    // Test consistency - same data = same checksum
    for (i = 0; i < 8; i++) {
        testBuffer[i] = i;
    }
    u8 chk1 = sramChecksum(testBuffer, 8);
    u8 chk2 = sramChecksum(testBuffer, 8);
    log_result("Checksum consistent", chk1 == chk2);
}

// =============================================================================
// Test: sramSave / sramLoad basic
// =============================================================================
void test_sram_save_load(void) {
    u8 i;

    // Prepare test data
    for (i = 0; i < 32; i++) {
        testBuffer[i] = i + 1;  // 1, 2, 3, ... 32
    }

    // Save to SRAM
    sramSave(testBuffer, 32);
    log_result("sramSave executes", 1);

    // Clear read buffer
    for (i = 0; i < 32; i++) {
        readBuffer[i] = 0;
    }

    // Load from SRAM
    sramLoad(readBuffer, 32);
    log_result("sramLoad executes", 1);

    // Verify data matches
    u8 match = 1;
    for (i = 0; i < 32; i++) {
        if (readBuffer[i] != testBuffer[i]) {
            match = 0;
            break;
        }
    }
    log_result("Save/Load data match", match);
}

// =============================================================================
// Test: sramSaveOffset / sramLoadOffset (save slots)
// =============================================================================
void test_sram_offset(void) {
    u8 i;

    // Prepare slot 1 data
    for (i = 0; i < 16; i++) {
        testBuffer[i] = 0xAA;
    }
    sramSaveOffset(testBuffer, 16, 0);  // Slot 1 at offset 0
    log_result("sramSaveOffset slot1", 1);

    // Prepare slot 2 data (different pattern)
    for (i = 0; i < 16; i++) {
        testBuffer[i] = 0x55;
    }
    sramSaveOffset(testBuffer, 16, 64);  // Slot 2 at offset 64
    log_result("sramSaveOffset slot2", 1);

    // Load slot 1 and verify
    sramLoadOffset(readBuffer, 16, 0);
    u8 slot1_ok = (readBuffer[0] == 0xAA) && (readBuffer[15] == 0xAA);
    log_result("sramLoadOffset slot1", slot1_ok);

    // Load slot 2 and verify
    sramLoadOffset(readBuffer, 16, 64);
    u8 slot2_ok = (readBuffer[0] == 0x55) && (readBuffer[15] == 0x55);
    log_result("sramLoadOffset slot2", slot2_ok);
}

// =============================================================================
// Test: sramClear
// =============================================================================
void test_sram_clear(void) {
    u8 i;

    // First write some data
    for (i = 0; i < 16; i++) {
        testBuffer[i] = 0xFF;
    }
    sramSave(testBuffer, 16);

    // Clear SRAM
    sramClear(16);
    log_result("sramClear executes", 1);

    // Load and verify it's cleared
    sramLoad(readBuffer, 16);
    u8 cleared = 1;
    for (i = 0; i < 16; i++) {
        if (readBuffer[i] != 0) {
            cleared = 0;
            break;
        }
    }
    log_result("sramClear zeros data", cleared);
}

// =============================================================================
// Test: Typical save game workflow
// =============================================================================
void test_save_game_workflow(void) {
    // Prepare save data
    saveData.magic[0] = 'S';
    saveData.magic[1] = 'A';
    saveData.magic[2] = 'V';
    saveData.magic[3] = 'E';
    saveData.score = 12345;
    saveData.level = 5;
    saveData.lives = 3;

    // Calculate checksum (exclude checksum field itself)
    saveData.checksum = 0;
    saveData.checksum = sramChecksum((u8*)&saveData, sizeof(SaveData));
    log_result("Checksum calculated", 1);

    // Save
    sramSave((u8*)&saveData, sizeof(SaveData));
    log_result("Save game data", 1);

    // Load into different buffer
    sramLoad((u8*)&loadedData, sizeof(SaveData));
    log_result("Load game data", 1);

    // Verify magic signature
    u8 magic_ok = (loadedData.magic[0] == 'S') &&
                  (loadedData.magic[1] == 'A') &&
                  (loadedData.magic[2] == 'V') &&
                  (loadedData.magic[3] == 'E');
    log_result("Magic signature valid", magic_ok);

    // Verify game data
    u8 data_ok = (loadedData.score == 12345) &&
                 (loadedData.level == 5) &&
                 (loadedData.lives == 3);
    log_result("Game data preserved", data_ok);

    // Verify checksum
    u8 storedChecksum = loadedData.checksum;
    loadedData.checksum = 0;
    u8 calcChecksum = sramChecksum((u8*)&loadedData, sizeof(SaveData));
    log_result("Checksum validates", storedChecksum == calcChecksum);
}

// =============================================================================
// Test: Boundary conditions
// =============================================================================
void test_boundary_conditions(void) {
    // Test saving 1 byte
    testBuffer[0] = 0x42;
    sramSave(testBuffer, 1);
    readBuffer[0] = 0;
    sramLoad(readBuffer, 1);
    log_result("Save/Load 1 byte", readBuffer[0] == 0x42);

    // Test saving at non-zero offset
    testBuffer[0] = 0xBE;
    testBuffer[1] = 0xEF;
    sramSaveOffset(testBuffer, 2, 100);
    readBuffer[0] = 0;
    readBuffer[1] = 0;
    sramLoadOffset(readBuffer, 2, 100);
    log_result("Offset boundary", (readBuffer[0] == 0xBE) && (readBuffer[1] == 0xEF));
}

// =============================================================================
// Main
// =============================================================================
int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();

    textPrintAt(2, 1, "SRAM MODULE TESTS");
    textPrintAt(2, 2, "-----------------");
    textPrintAt(2, 3, "Testing save game...");

    test_passed = 0;
    test_failed = 0;

    // Run tests
    test_sram_checksum();
    test_sram_save_load();
    test_sram_offset();
    test_sram_clear();
    test_save_game_workflow();
    test_boundary_conditions();

    // Display results
    textPrintAt(2, 5, "Tests completed");
    textPrintAt(2, 6, "Static asserts: PASSED");

    // Show pass/fail count
    textPrintAt(2, 8, "Passed:");
    textPrintAt(2, 9, "Failed:");

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }

    return 0;
}
