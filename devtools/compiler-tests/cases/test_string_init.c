// =============================================================================
// Test: String literal initialization
// =============================================================================
// Prevents: Incorrect string reference in struct initializers
//
// This test verifies that string literals used in structure initializers
// are correctly referenced. The compiler must place the string data in
// ROM and generate a valid pointer to it.
// =============================================================================

// Structure with string pointer
typedef struct {
    const char *name;
    unsigned char id;
    unsigned char power;
} Item;

// Structure with multiple string pointers
typedef struct {
    const char *title;
    const char *author;
    unsigned short year;
} Book;

// Initialized items - strings should be in ROM
const Item sword = {"Sword", 1, 10};
const Item shield = {"Shield", 2, 5};
const Item potion = {"Potion", 3, 0};

// Array of items with strings
const Item inventory[3] = {
    {"Apple", 10, 1},
    {"Key", 20, 0},
    {"Gold", 30, 0}
};

// Multiple strings in one struct
const Book manual = {"SNES Dev Guide", "OpenSNES Team", 2026};

// Non-const items (RAM) with string pointers (ROM)
Item player_weapon = {"Rusty Sword", 100, 3};

void test_string_access(void) {
    // Access string through struct
    const char *name = sword.name;
    char first = name[0];  // Should be 'S'

    // Access through array
    const char *item_name = inventory[1].name;
    char c = item_name[0];  // Should be 'K'
}

void test_modify_non_const(void) {
    // Can modify non-string members of non-const struct
    player_weapon.power = 5;
    player_weapon.id = 101;
    // Note: Modifying name pointer would point to new string
}

int main(void) {
    test_string_access();
    test_modify_non_const();
    return (int)sword.name[0];  // Should return 'S' (83)
}
