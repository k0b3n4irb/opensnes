// =============================================================================
// Test: Nested structure access
// =============================================================================
// Prevents: Incorrect offset calculation for nested structure members
//
// This test verifies that the compiler correctly calculates offsets when
// accessing members of structures that are nested within other structures.
// =============================================================================

typedef struct {
    unsigned char x;
    unsigned char y;
} Point;

typedef struct {
    Point position;      // offset 0, size 2
    Point velocity;      // offset 2, size 2
    unsigned char flags; // offset 4, size 1
} Entity;

typedef struct {
    Entity player;       // offset 0, size 5
    Entity enemies[4];   // offset 5, size 20 (4 * 5)
    unsigned short score;// offset 25, size 2
} GameState;

GameState game;

void test_nested_access(void) {
    // Direct nested member access
    game.player.position.x = 100;
    game.player.position.y = 80;
    game.player.velocity.x = 1;
    game.player.velocity.y = 0;
    game.player.flags = 0x01;

    // Array of nested structs
    game.enemies[0].position.x = 50;
    game.enemies[2].velocity.y = 255;

    // Sibling member after nested structs
    game.score = 1000;
}

int main(void) {
    test_nested_access();
    return (int)game.player.position.x;
}
