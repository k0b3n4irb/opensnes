# Platformer Template

A side-scrolling platformer game template for OpenSNES.

## Features

- Player movement (walk, run, jump)
- Gravity and collision detection
- Scrolling background
- Animated sprites
- Simple enemy AI

## Building

```bash
# Set SDK path
export OPENSNES_HOME=/path/to/opensnes

# Build
make

# Run in emulator
make run
# or
mesen platformer.sfc
```

## Project Structure

```
platformer/
├── Makefile        # Build configuration
├── src/
│   ├── main.c      # Entry point and main loop
│   ├── player.c    # Player logic
│   ├── enemy.c     # Enemy logic
│   ├── level.c     # Level/collision handling
│   └── hdr.asm     # ROM header
├── data/
│   ├── tiles.asm   # Tile graphics data
│   ├── sprites.asm # Sprite graphics data
│   └── palette.asm # Color palettes
└── assets/
    ├── player.png  # Player sprite sheet
    ├── enemies.png # Enemy sprites
    └── tiles.png   # Background tiles
```

## Controls

| Button | Action |
|--------|--------|
| D-Pad | Move left/right |
| B | Jump |
| Y | Run (hold) |
| Start | Pause |

## Customizing

### Changing the Player

1. Edit `assets/player.png` (16x16 sprites recommended)
2. Run asset converter: `make assets`
3. Modify `src/player.c` for different physics

### Adding Enemies

1. Add sprite to `assets/enemies.png`
2. Create enemy type in `src/enemy.c`
3. Place in level data

### Creating Levels

1. Use Tiled map editor to create `.tmx` files
2. Convert with `tmx2snes` tool
3. Load in `src/level.c`

## Memory Map

| Address | Usage |
|---------|-------|
| $7E0200 | Game variables |
| $7E1000 | Player state |
| $7E2000 | Enemy array |
| $7F0000 | Level data buffer |

## Credits

Template by OpenSNES Team
