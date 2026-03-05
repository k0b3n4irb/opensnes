# Sprite Examples

Learn SNES sprite (OBJ) handling from basic display to dynamic VRAM management.
The SNES supports 128 hardware sprites with configurable sizes, priorities, and
palette assignments.

## Examples

| Example | Difficulty | Description |
|---------|------------|-------------|
| [simple_sprite](simple_sprite/) | Beginner | Display and move a single sprite on screen |
| [animated_sprite](animated_sprite/) | Beginner | Frame-based sprite animation with horizontal flip |
| [object_size](object_size/) | Intermediate | OBJSEL sprite size configurations (8x8 through 64x64) |
| [metasprite](metasprite/) | Intermediate | Composing large characters from multiple hardware sprites |
| [dynamic_sprite](dynamic_sprite/) | Advanced | Dynamic VRAM tile streaming for many unique sprites |

## Key Concepts

### OAM Structure

128 sprites, 4 bytes each in the low table (512 bytes):
- Byte 0: X position (low 8 bits)
- Byte 1: Y position
- Byte 2: Tile number (low 8 bits)
- Byte 3: Attributes (vhoopppc -- flip, priority, palette, tile high bit)

High table (32 bytes): 2 bits per sprite for X high bit and size select.

### Sprite Sizes (OBJSEL $2101)

| Value | Small | Large |
|-------|-------|-------|
| 0 | 8x8 | 16x16 |
| 1 | 8x8 | 32x32 |
| 2 | 8x8 | 64x64 |
| 3 | 16x16 | 32x32 |
| 4 | 16x16 | 64x64 |
| 5 | 32x32 | 64x64 |

### Performance Tip

For many sprites per frame, write directly to the `oamMemory[]` buffer instead
of calling `oamSet()` repeatedly. The NMI handler DMAs the buffer to OAM hardware
when `oam_update_flag` is set.

---

Start with **simple_sprite** to understand OAM basics, then progress through
**animated_sprite** and **metasprite** toward **dynamic_sprite**.
