// =============================================================================
// Test: Struct pointer initialization
// =============================================================================
// Prevents: QBE static buffer bug (February 2026)
//
// This test verifies that when a structure contains a pointer to another
// structure, the compiler correctly generates the symbol reference in the
// initialization data.
//
// BUG HISTORY:
// The QBE emit.c code stored a pointer to the parser's static buffer
// (tokval.str) instead of copying the string. When subsequent tokens were
// parsed, the buffer was overwritten, corrupting the symbol name.
//
// Expected ASM: .dl myFrames+0
// Bug output:   .dl z+0 (or other corrupted name)
// =============================================================================

typedef struct {
    unsigned char data[3];
} Frame;

typedef struct {
    Frame *frames;
    unsigned char count;
} Animation;

// Initialize Frame with data
Frame myFrames = {{10, 20, 30}};

// Initialize Animation with pointer to Frame - THIS IS THE BUG TRIGGER
Animation myAnim = {&myFrames, 3};

int main(void) {
    // Access through pointer must work correctly
    unsigned char val = myAnim.frames->data[0];
    return (int)val;
}
