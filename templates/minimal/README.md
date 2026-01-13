# Minimal Template

The simplest starting point for an OpenSNES project.

## Quick Start

1. Copy this folder to your projects directory
2. Edit `Makefile`:
   - Set `OPENSNES` to your SDK path
   - Change `TARGET` to your ROM name
   - Change `ROM_NAME` (must be <=21 characters)
3. Run `make`
4. Open the `.sfc` file in an emulator

## Files

| File | Purpose |
|------|---------|
| `main.c` | Your game code |
| `Makefile` | Build configuration |

## What the Template Shows

- Hardware register access
- VBlank waiting (60fps timing)
- Joypad reading
- Setting background color

## Controls

| Button | Action |
|--------|--------|
| D-pad Up/Down | Adjust green |
| D-pad Left/Right | Adjust red |
| Start | Reset to blue |

## Next Steps

1. **Add graphics**: See `examples/graphics/` for sprite and background examples
2. **Add sound**: See `examples/audio/` for sound playback
3. **Add game logic**: Check out `examples/games/1_pong/` for a complete game

## Common Modifications

### Adding another C file

```makefile
CSRC := main.c player.c enemies.c
```

### Using the library

Include the snes.h header:

```c
#include <snes.h>
```

Then use library functions like `WaitForVBlank()`, `padUpdate()`, etc.

## Troubleshooting

**Black screen?**
- Make sure `REG_INIDISP = 0x0F` is called to turn on display

**No input?**
- Enable auto-joypad with `REG_NMITIMEN = 0x81`

**Build error?**
- Check that `OPENSNES` path in Makefile is correct
