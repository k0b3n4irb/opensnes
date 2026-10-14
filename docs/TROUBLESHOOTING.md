# Troubleshooting OpenSNES {#troubleshooting}

From a symptom to its cause. The SNES fails quietly, so many symptoms below
lead to an entry of @ref known_limitations, the catalogue of silent failures:
this page names the trap, that one explains it.

## Build Issues

### "OPENSNES not defined" or "No rule to make target 'common.mk'"

**Cause**: The Makefile can't find the OpenSNES installation.

**Fix**: Set `OPENSNES` in your Makefile, or pass it on the command line:

```bash
# Option 1: in the Makefile (recommended for project portability)
OPENSNES := /path/to/opensnes

# Option 2: on the command line (make reads variables, not OPENSNES_HOME)
make OPENSNES=/path/to/opensnes
```

`OPENSNES_HOME` is read by the `opensnes` CLI only, not by `make`.

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

### Build fails with "refusing to emit silently-wrong code"

**Cause**: The code uses a C feature the 65816 backend does not lower. The
message names it: a struct passed or returned by value, a struct copied to
or from a `FAR` object, a variadic function, or inline assembly (`asm`).
Plain struct assignment (`a = b;`) works.

**Fix**: pass a pointer to the struct, copy it field by field (or with a
loop), give the function a fixed argument list, or move the assembly into a
`.asm` file of the project. 32-bit arithmetic (`u32`, `s32`, `*`, `/`, `%`,
shifts) is supported and tested — prefer `u16` for speed, not for
correctness.

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

**4. A DMA under `setBrightness(0)`**

`setBrightness(0)` darkens the picture but is not forced blank: the PPU still
rejects VRAM writes. Upload under `setScreenOff()`.

**5. C variables past the 8 KB RAM band**

Plain C variables must sit in `$0000-$1FFF`; a large buffer belongs in far
RAM (`FAR`, see [Far RAM](tutorials/far_ram.md)). The build checks it after
every link and stops when the band overflows; `opensnes-rom check game.sfc`
prints how much of it is left, and `opensnes-rom budget game.sfc` what uses
it.

### A big buffer behaves strangely

Same cause as above: a plain C global past `$2000` is wrong-banked. Declare
bulk buffers `FAR` (@ref tutorial_far_ram; the RAM entry of
@ref known_limitations).

### A `const` array reads garbage

The `const` was cast away and the data read through a plain pointer. C const
data lives in the asset banks and every C read of it is a far read; a plain
pointer reads bank $00 instead. Keep the `const`, or hand the data to a library
function that takes a far pointer. The build's bank-blind read check fails the
link on this pattern; the bank $00 entry of @ref known_limitations has the
details.

### ROM runs but crashes after a few seconds

**Causes**:
1. **Stack overflow** - Too many function calls or large local arrays
2. **DMA during active display** - Only do DMA in VBlank
3. **Uninitialized pointers** - C doesn't zero local variables

**Debug**:
1. Use luna to see where it crashes (`run_until_pc` / `state` over MCP —
   see the [debugging tutorial](tutorials/debugging.md))
2. Check the call stack depth
3. Look for NULL pointer dereferences

### Sprites don't appear

**Checklist**:
```c
// 1. Initialize OAM
oamInit(OAM_DEFAULT_SIZE, OAM_DEFAULT_TILE_BASE);

// 2. Load sprite graphics to VRAM
oamInitGfxSet(sprite_gfx, sprite_gfx_end - sprite_gfx,
              sprite_pal, sprite_pal_end - sprite_pal,
              0, 0x0000, OBJ_SIZE8_L16);

// 3. Set sprite properties — oamSet(id, x, y, tile, palette, priority, flags)
oamSet(0, x, y, 0, 0, 3, 0);

// 4. Visibility is the Y position itself: a valid on-screen Y (set by
//    oamSet above) shows the sprite; oamHide() parks it at Y=240.

// 5. Update OAM (done in VBlank by the library)
// The NMI handler uploads the OAM shadow when a sprite call changed it
// (oam_update_flag); a direct write into oamMemory[] is not seen
```

### A sprite renders as background garbage

`oamInit`'s second argument is a name-base page number (0-7), not a VRAM
address. Pass `OBJ_NAME_BASE(addr)` rather than the address.

### A sprite appears where no entity is

Off-camera OAM coordinates wrap around the screen. Hide a sprite that left
the view with `oamHide()`; `oamPlaceWorld()` culls for you
(@ref tutorial_sprites).

### Sprites have the wrong colours

Sprite palettes start at CGRAM entry 128, not 0: palette 0 of the sprites is
entries 128-143. A palette loaded at 0 colours the backgrounds instead.

### Controller input not working

**Checklist**:
```c
// Input is read automatically by the NMI handler every VBlank.
// Just call the query functions after WaitForVBlank:
WaitForVBlank();
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
audioLoadSample(0, sample_brr, sample_brr_end - sample_brr, 0); // id, BRR data, size, loop point
audioPlaySample(0);                                             // play sample id 0
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

SNES uses BGR555 (5 bits blue, 5 green, 5 red), not RGB. Let the converter write the palette from an indexed PNG:
```bash
opensnes-sprite sheet res/sprite.png --size 16 --colors 16    # sprites
opensnes-tileset convert res/bg.png --colors 16               # backgrounds
```
(or the same settings in `res/sprite.png.toml`, which the build runs for you).

### Colours shift when you add a tile

The converter derives the whole palette again from the new art. Author a fixed
palette and impose it: `palette = "fixed.pal"` in the asset's settings file
(@ref tools_conventions).

### Tiles appear corrupted, or an upload only partly lands

**Causes**:
1. VRAM written during the picture: the PPU drops it without an error. Upload
   in VBlank or under `setScreenOff()`
2. Over the ~4 KB of DMA a VBlank holds: split the upload across frames or
   do it under forced blank
3. Wrong tile format (2bpp vs 4bpp vs 8bpp)
4. VRAM address overlap
5. DMA transfer size mismatch

The first two are the first two entries of @ref known_limitations.

**Check your graphics mode**:
```c
// Mode 1: BG1/BG2 are 4bpp, BG3 is 2bpp
setMode(BG_MODE1, 0);

// Make sure you're loading the right format
bgInitTileSet(0, tiles_4bpp, ...);  // 4bpp for BG0 in Mode 1
```

### Scrolling is jerky or tears

**Cause**: Writing `BGnHOFS` / `BGnVOFS` directly during the frame: the top
of the picture uses the old value, the bottom the new one.

**Fix**: use `bgSetScroll()`. It only records the position, and the NMI
handler writes it during VBlank. Set it before `WaitForVBlank()`, with the
rest of the frame's logic, so it is shown on the next frame:

```c
while (1) {
    scrollX += 1;                       // game logic
    bgSetScroll(0, scrollX, scrollY);
    WaitForVBlank();                    // the NMI commits the scroll
}
```

See [Scrolling](tutorials/scrolling.md) for per-line scroll (HDMA parallax).

### Everything is one line off vertically

The library already subtracts the 1 that the PPU's missing scanline 0
requires (`bgSetScroll`, the map module, `mode7SetScroll`). Pass the row you
want; keep your own -1 only in data that reaches the register without the
library, such as an HDMA table on `BGnVOFS`. A PVSnesLib port that passed
`y - 1` itself ends up one line off. The vertical scroll entry of
@ref known_limitations has the details.

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
# The post-link checks: bank $00, the RAM bands, bank-blind reads
opensnes-rom check game.sfc

# What the game uses of ROM, RAM, VRAM and CGRAM
opensnes-rom budget game.sfc

# View symbol map
cat game.sym | grep "variable_name"

# Inspect ROM header
xxd -s 0x7FC0 -l 32 game.sfc

# Check ROM size
ls -la game.sfc
```

### Interactive debugging with luna

See the [debugging tutorial](tutorials/debugging.md) for the full workflows
(who-writes-this-variable, break-at-function, OAM shadow-vs-hardware, poke
and observe). A third-party GUI debugger also works:

### Using Mesen Debugger (alternative)

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
