# Troubleshooting OpenSNES

Common issues and their solutions.

## Build Issues

### "OPENSNES not defined" or "No rule to make target 'common.mk'"

**Cause**: The Makefile can't find the OpenSNES installation.

**Fix**: Set `OPENSNES` in your Makefile or set the environment variable:

```bash
# Option 1: Set in Makefile (recommended for project portability)
OPENSNES := /path/to/opensnes

# Option 2: Set environment variable
export OPENSNES_HOME=/path/to/opensnes
```

### Empty `compiler/` directories or "cc65816 not found"

**Cause**: Submodules weren't cloned.

**Fix**:
```bash
cd /path/to/opensnes
git submodule update --init --recursive
make compiler
```

### "wla-65816: command not found"

**Cause**: Compiler toolchain not built.

**Fix**:
```bash
cd /path/to/opensnes
make compiler
```

### "clang: command not found" or "cc: command not found"

**Cause**: Missing host C compiler.

**Fix** (by platform):
```bash
# macOS
xcode-select --install

# Ubuntu/Debian
sudo apt install clang

# Fedora
sudo dnf install clang

# Windows MSYS2 (UCRT64)
pacman -S mingw-w64-ucrt-x86_64-clang
```

### Build fails with "unhandled op: XXX"

**Cause**: The QBE backend doesn't support a specific operation (usually 32-bit or complex expressions).

**Fix**: Simplify your code:
```c
// BAD: 32-bit operations are slow and may not work
u32 bigValue = x * 1000;

// GOOD: Use 16-bit when possible
u16 value = x * 10;

// BAD: Complex expressions in one line
result = (a * b + c) / d;

// GOOD: Break it down
u16 temp = a * b;
temp += c;
result = temp / d;
```

### "error: static variable 'X' initialized in ROM"

**Cause**: C static variables with initializers are placed in ROM (read-only).

**Fix**:
```c
// BAD: Initialized static goes to ROM
static u8 counter = 0;

// GOOD: Uninitialized static goes to RAM (C guarantees zero-init)
static u8 counter;
```

## Runtime Issues (Black Screen / Crash)

### ROM loads but screen is black

This is the most common issue. Causes and fixes:

**1. Missing `setScreenOn()`**
```c
// The screen is OFF by default. You must turn it on:
setScreenOn();
```

**2. No VBlank loop**
```c
// Without this, the CPU runs too fast and corrupts VRAM
while (1) {
    WaitForVBlank();  // Required!
    // Game logic here
}
```

**3. Console not initialized**
```c
// Always call consoleInit() first if using text/console functions
consoleInit();
```

**4. Memory overlap (WRAM mirror bug)**

Bank $00 addresses $0000-$1FFF mirror Bank $7E addresses $7E:0000-$1FFF. If you have variables in both ranges, they'll overwrite each other.

**Diagnose**:
```bash
python3 tools/symmap/symmap.py --check-overlap game.sym
```

**Fix**: The library uses `ORGA $0300` to avoid the overlap zone.

### ROM runs but crashes after a few seconds

**Causes**:
1. **Stack overflow** - Too many function calls or large local arrays
2. **DMA during active display** - Only do DMA in VBlank
3. **Uninitialized pointers** - C doesn't zero local variables

**Debug**:
1. Use Mesen's debugger to see where it crashes
2. Check the call stack depth
3. Look for NULL pointer dereferences

### Sprites don't appear

**Checklist**:
```c
// 1. Initialize OAM
oamInit();

// 2. Load sprite graphics to VRAM
oamInitGfxSet(sprite_gfx, sprite_gfx_end - sprite_gfx,
              sprite_pal, sprite_pal_end - sprite_pal,
              0, 0x0000, OBJ_SIZE8_L16);

// 3. Set sprite properties
oamSet(0, x, y, 3, 0, 0, 0, 0);

// 4. Make sure sprite is visible (not hidden)
// oamSetVisible(0, OBJ_SHOW);  // If needed

// 5. Update OAM (usually done in VBlank by the library)
// The NMI handler calls oamUpdate() automatically
```

### Controller input not working

**Checklist**:
```c
// Must call every frame AFTER WaitForVBlank
WaitForVBlank();
padUpdate();  // Read controllers

// Use the right functions
u16 pressed = padPressed(0);   // Just pressed this frame
u16 held = padHeld(0);         // Currently held
u16 released = padReleased(0); // Just released this frame

// Check correct button bits
if (pressed & KEY_A) { }    // A button (bit 7)
if (pressed & KEY_B) { }    // B button (bit 15)
```

### Audio not playing

**For basic audio**:
```c
audioInit();
audioPlaySample(channel, sample_data, sample_size, pitch);
```

**For SNESMOD**:
```c
snesmodInit();
snesmodSetSoundbank(soundbank);
snesmodLoadModule(MOD_MUSIC);
snesmodPlay(0);

// MUST call every frame:
while (1) {
    WaitForVBlank();
    snesmodProcess();  // Required!
}
```

## Graphics Issues

### Wrong colors / garbled palette

**Cause**: BGR555 format mismatch.

SNES uses BGR555 (5 bits blue, 5 green, 5 red), not RGB. Use gfx4snes to convert PNGs correctly:
```bash
gfx4snes -i sprite.png -o sprite
```

### Tiles appear corrupted

**Causes**:
1. Wrong tile format (2bpp vs 4bpp vs 8bpp)
2. VRAM address overlap
3. DMA transfer size mismatch

**Check your graphics mode**:
```c
// Mode 1: BG1/BG2 are 4bpp, BG3 is 2bpp
setMode(BG_MODE1, 0);

// Make sure you're loading the right format
bgInitTileSet(0, tiles_4bpp, ...);  // 4bpp for BG0 in Mode 1
```

### Scrolling is jerky

**Cause**: Updating scroll registers outside VBlank.

**Fix**:
```c
while (1) {
    WaitForVBlank();
    // Update scroll AFTER VBlank starts
    bgSetScroll(0, scrollX, scrollY);

    // Game logic
}
```

## Memory Issues

### "ROM too large" or linker errors about size

**Cause**: Too much data in your ROM.

**Check ROM size**:
```bash
ls -la game.sfc
```

**Solutions**:
1. Compress graphics
2. Use smaller audio samples
3. Split into multiple banks (advanced)

### Variables have wrong values

**Check symbol placement**:
```bash
grep "my_variable" game.sym
```

- Good (RAM): `00:0234 my_variable`
- Bad (ROM): `00:8234 my_variable`

If it's in ROM ($8000+), you can't write to it.

## Getting More Help

### Diagnostic Commands

```bash
# Check for memory overlaps
python3 tools/symmap/symmap.py --check-overlap game.sym

# View symbol map
cat game.sym | grep "variable_name"

# Inspect ROM header
xxd -s 0x7FC0 -l 32 game.sfc

# Check ROM size
ls -la game.sfc
```

### Using Mesen Debugger

1. Load ROM in Mesen
2. Tools → Debugger
3. Set breakpoint at your code
4. Step through and watch registers/memory

### Reporting Bugs

Include in your issue:
1. OpenSNES version (`git rev-parse HEAD`)
2. OS and compiler version
3. Minimal code that reproduces the issue
4. Error messages (full output)
5. What you expected vs what happened
