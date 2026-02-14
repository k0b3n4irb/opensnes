# Sprite Examples

Learn SNES sprite (OBJ) handling from basic display to dynamic VRAM management.

## Examples

| Example | Complexity | Description |
|---------|------------|-------------|
| [simple_sprite](simple_sprite/) | 1 | Basic sprite display and movement |
| [animated_sprite](animated_sprite/) | 2 | Frame-based sprite animation |
| [animation_system](animation_system/) | 3 | State machine animation system |
| [metasprite](metasprite/) | 3 | Multi-tile sprites (metasprites) |
| [dynamic_sprite](dynamic_sprite/) | 4 | Dynamic VRAM loading for many sprites |

## Key Concepts

### OAM Structure
128 sprites, 4 bytes each in low table:
- Byte 0: X position (low 8 bits)
- Byte 1: Y position
- Byte 2: Tile number
- Byte 3: Attributes (vhoopppc)

### Sprite Sizes (OBJSEL $2101)
| Value | Small | Large |
|-------|-------|-------|
| 0 | 8×8 | 16×16 |
| 1 | 8×8 | 32×32 |
| 2 | 8×8 | 64×64 |
| 3 | 16×16 | 32×32 |
| 4 | 16×16 | 64×64 |
| 5 | 32×32 | 64×64 |

### Important: Coordinate Pattern
Sprite coordinates MUST use struct with s16 members:
```c
typedef struct { s16 x, y; } Position;
Position player = {100, 100};
oamSet(0, player.x, player.y, ...);
```
