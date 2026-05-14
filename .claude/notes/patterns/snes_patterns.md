# SNES Development Patterns & Best Practices

Living document. Updated whenever new patterns are discovered through debugging.

**Rule**: When the user identifies a new pattern or best practice, update this document.

---

## Core Principle: CPU and PPU Are Concurrent

The SNES has no hardware double-buffering. The CPU and PPU share registers and
VRAM simultaneously. VBlank (~2.3ms) is the ONLY safe window to update hardware
without visual artifacts. Every pattern below derives from this constraint.

---

## Pattern 1: Prepare, Sync, Commit

**The fundamental pattern for ALL hardware updates.**

```
Prepare buffer (RAM)  →  WaitForVBlank (NMI DMA)  →  Write PPU registers
```

NEVER write PPU registers before the DMA that updates the corresponding data.
The PPU reads registers and OAM/VRAM during active display. Changing a register
before the data is in sync produces one frame of incoherent state.

**Example** (metasprite OBJSEL fix):
```c
// WRONG: old OAM + new OBJSEL = garbled sprites for one frame
WaitForVBlank();        // DMAs OLD OAM
REG_OBJSEL = new_mode;  // PPU reinterprets old sprites with new sizes
oamClear();             // too late — already rendered

// CORRECT: new OAM + new OBJSEL in same VBlank
oamClear();
drawSprites();          // buffer ready
WaitForVBlank();        // NMI DMAs new OAM
REG_OBJSEL = new_mode;  // still in VBlank, both consistent
```

**Applies to**: OBJSEL, BG mode changes, scroll + tilemap, palette + BG enable,
window registers (W12SEL, TMW) + HDMA table setup.

---

## Pattern 2: Atomic VBlank Transaction

Related state changes must land in the SAME VBlank. Never split across two.

```
// WRONG: 2 VBlanks, 1 frame of mismatch
Change A → WaitForVBlank → Change B → WaitForVBlank

// CORRECT: both in one VBlank
Prepare A + B → WaitForVBlank → Commit A + B
```

**Examples of coupled state**:
- OAM data + OBJSEL (sprite sizes)
- Tilemap DMA + scroll position
- HDMA table + window/colormath registers
- BG tile data + palette

---

## Pattern 3: Fresh VBlank Sync After Heavy Preparation

When preparing buffers takes significant time (oamClear + drawSprites, HDMA table
construction), a stale NMI can fire mid-preparation, DMA a partial buffer, and
set `vblank_flag`. The next `WaitForVBlank` sees the stale flag and returns
immediately (lag-frame shortcut), causing the commit to happen during active display.

**Fix**: Clear `vblank_flag` after preparation to force a fresh NMI.

```c
extern volatile u8 vblank_flag;

oamClear();           // sets oam_update_flag=1 (can trigger premature DMA)
drawSprites();        // buffer now complete
vblank_flag = 0;      // invalidate any stale NMI
WaitForVBlank();      // waits for FRESH NMI → DMAs complete buffer
REG_OBJSEL = new;     // safe: in VBlank, after correct DMA
```

**When to use**: Any time you call multiple buffer-modifying functions (oamClear,
oamSet, fillHdmaTable...) before WaitForVBlank, and the total preparation could
overlap with a VBlank boundary.

---

## Pattern 4: PPU Register Writes During VBlank Only

PPU registers ($2100-$213F) should only be written during VBlank or forced blank.
Writing during active display causes the change to take effect mid-scanline,
creating visible tearing or single-scanline artifacts.

```c
// WRONG: white line artifact on iris wipe
REG_W12SEL = 0x03;       // written during active display
REG_TMW = 0x01;          // PPU sees change mid-scanline
hdmaIrisWipe(6, ...);

// CORRECT: write during VBlank
WaitForVBlank();
REG_W12SEL = 0x03;       // VBlank — PPU not rendering
REG_TMW = 0x01;
hdmaIrisWipe(6, ...);
```

**Exception**: Forced blank (`REG_INIDISP = 0x80`) allows writes anytime but
causes a visible black frame.

---

## Pattern 5: Clear Before Draw (OAM Hygiene)

When switching between states that use different sprite counts, always clear
the full OAM buffer before drawing new sprites. Leftover sprites from the
previous state render as garbage with the new configuration.

```c
// Mode 0: 10 sprites (IDs 0-9)
// Mode 1: 8 sprites (IDs 0-7)
// If you only draw mode 1 sprites, IDs 8-9 retain old data

oamClear();       // hide ALL 128 sprites (Y=240, Xhi=1)
drawSprites();    // draw ONLY what's needed for current mode
```

For per-frame loops where sprite count doesn't change, oamClear every frame is
optional but defensive. For mode transitions, it's mandatory.

---

## Pattern 6: HDMA Tables in Bank $00 RAM

HDMA tables built via WRAM port ($2180) into bank $7E can cause frame-by-frame
blinking due to timing issues between WRAM port writes and HDMA reads.

**Safe approach**: Allocate tables in bank $00 RAM (RAMSECTION BANK 0 SLOT 1)
and fill them with C pointer writes. Use `hdmaSetupBank()` with explicit bank $00.

```c
// WRONG: WRAM port to bank $7E — prone to blinking
REG_WMADDL = addr & 0xFF;
REG_WMADDM = (addr >> 8) & 0xFF;
REG_WMADDH = 0x7E;
REG_WMDATA = value;  // timing-sensitive

// CORRECT: C pointer to bank $00 RAM
u8 *p = (u8 *)hdma_table;  // declared in RAMSECTION BANK 0
*p++ = count;
*p++ = value;
hdmaSetupBank(ch, mode, dest, hdma_table, 0x00);
```

**Exception**: Tables too large for bank $00 ($0000-$1FFF = 8KB shared with
C variables, stack, OAM) must stay in bank $7E. Use double-buffering with
careful VBlank synchronization.

---

## Pattern 7: VBlank DMA Budget (~5KB Max)

NMI handler overhead + OAM DMA + scroll writes consume ~60% of VBlank.
Remaining budget: ~5KB of DMA per frame with screen ON.

For larger transfers:
- **Split across multiple frames**: 1 page (2KB) per VBlank for tilemaps
- **Forced blank**: `REG_INIDISP = 0x80` before DMA, restore after (black frame)
- **Init-time only**: do bulk VRAM loads before `setScreenOn()`

---

## Anti-Patterns (Common Mistakes)

### Writing OBJSEL/mode registers BEFORE updating OAM/VRAM
Always update data first, then registers. See Pattern 1.

### Consuming a WaitForVBlank for nothing
Every WaitForVBlank burns one frame. If you call it twice without meaningful work
between, you've added 16ms of lag. Use exactly ONE WaitForVBlank per frame in the
main loop.

### Setting oam_update_flag during multi-step buffer preparation
oamClear() and oamSet() set `oam_update_flag=1` internally. If NMI fires
mid-preparation, it DMAs partial data. Use Pattern 3 (fresh VBlank sync).

### Force blank during gameplay
`REG_INIDISP = 0x80` hides the screen. Professional SNES games avoid this except
during scene transitions. Use VBlank DMA budgeting instead.

---

## Changelog

- 2026-03-08: Initial version from metasprite, HDMA helpers, and iris wipe fixes
