# OpenSNES Code Style Guide {#code_style}

Consistent code style makes the codebase easier to read and maintain.

## C Code

### General

- **Indentation**: 4 spaces (no tabs)
- **Line length**: 80 characters soft limit, 100 hard limit
- **Braces**: K&R style (opening brace on same line)

```c
// Good
if (condition) {
    do_something();
} else {
    do_other();
}

// Bad
if (condition)
{
    do_something();
}
```

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Functions | camelCase, the module first | `playerSpawn()`, as the SDK's `oamSet()` |
| Variables | snake_case | `player_x` |
| Constants | UPPER_CASE | `MAX_SPRITES` |
| Types | PascalCase | `SpriteData` |
| Macros | UPPER_CASE | `DMA_ENABLE` |
| Enums | UPPER_CASE | `SPRITE_SIZE_8X8` |

### Types

Use fixed-width types from `snes.h`:

```c
// Good
u8 byte_value;
u16 word_value;
s16 signed_value;
s32 long_value;

// Bad
unsigned char byte_value;
int value;
```

### Function Documentation

Use Doxygen-style comments:

```c
/**
 * @brief Initialize a sprite
 *
 * Sets up sprite with default values at the specified position.
 *
 * @param id Sprite ID (0-127)
 * @param x Initial X position
 * @param y Initial Y position
 * @return TRUE on success, FALSE if id is invalid
 *
 * @code
 * if (playerSpawn(0, 100, 50)) {
 *     playerSetTile(0, PLAYER_TILE);
 * }
 * @endcode
 */
u8 playerSpawn(u8 id, u16 x, u8 y);
```

### Error Handling

```c
// Return FALSE/NULL on error
u8 playerSpawn(u8 id, u16 x, u8 y) {
    if (id >= MAX_SPRITES) {
        return FALSE;
    }
    // ...
    return TRUE;
}

// Check return values
if (!playerSpawn(id, x, y)) {
    // Handle error
}
```

### Memory

```c
// No heap: memory is a global, a static or a local of known size

// Zero-initialize structures
SpriteData sprite;
memset(&sprite, 0, sizeof(sprite));
```

## Assembly (65816 - WLA-DX)

### General

- **Indentation**: Labels at column 0, instructions indented with tab
- **Comments**: Explain non-obvious operations
- **Sections**: Use `.section` for code organization

### Format

```asm
;============================================================================
; Function: playerUpdate
; Description: Update all active sprites
; Input: None
; Output: None
; Clobbers: A, X, Y
;============================================================================
.section ".text" superfree

playerUpdate:
    php                     ; Save processor status
    rep #$20                ; 16-bit accumulator
    .ACCU 16                ; tell WLA-DX: it does not follow rep/sep across branches

    ldx #0                  ; Start at sprite 0
-   lda player_active,x     ; Check if active
    beq +                   ; Skip if inactive

    jsr _update_single      ; Update this sprite

+   inx
    inx                     ; Next sprite (16-bit index)
    cpx #MAX_SPRITES * 2
    bne -

    plp                     ; Restore processor status
    rtl

.ends
```

### Register Documentation

```asm
; Document register state changes, and say each one to the assembler
    rep #$20                ; A = 16-bit
    .ACCU 16
    sep #$10                ; X/Y = 8-bit
    .INDEX 8

; Document expected register values
; Input: A = sprite ID (0-127)
; Output: X = OAM offset
```

### Labels

```asm
; Public labels: the C name, as the header declares it (no underscore)
playerSpawn:

; Local labels: use anonymous (+ -)
    bne +           ; Forward reference
    bra -           ; Backward reference

; Named local: .local_name
.calculate_offset:
```

## File Organization

### Headers (.h)

```c
/**
 * @file sprite.h
 * @brief Sprite management API
 */

#ifndef OPENSNES_SPRITE_H
#define OPENSNES_SPRITE_H

#include <snes.h>

/* Constants */
#define MAX_SPRITES 128

/* Types */
typedef struct {
    u16 x;
    u8 y;
    u16 tile;
} SpriteData;

/* Functions */
u8 playerSpawn(u8 id, u16 x, u8 y);
void playerUpdate(void);

#endif /* OPENSNES_SPRITE_H */
```

### Source (.c)

```c
/**
 * @file sprite.c
 * @brief Sprite management implementation
 */

#include "sprite.h"

/* Private data */
static SpriteData sprites[MAX_SPRITES];

/* Public functions */
u8 playerSpawn(u8 id, u16 x, u8 y) {
    // Implementation
}
```

## Formatting Tools

Consider using:
- Consistent editor settings (`.editorconfig`)
