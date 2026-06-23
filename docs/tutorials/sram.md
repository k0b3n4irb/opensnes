# SRAM Save Tutorial {#tutorial_sram}

This tutorial covers SRAM — the battery-backed RAM region on a SNES
cartridge that survives power-off, used for save games on every cart
that has one (Zelda, Final Fantasy VI, Super Mario World, …). The lib
exposes a small set of byte-array helpers; the discipline lives in
treating SRAM as **untrusted** data that needs validation on every
load.

It assumes you have read the [Graphics](graphics.md) tutorial. The
[DMA tutorial](dma.md) is useful background but not required — SRAM
access goes through plain CPU loads/stores, not DMA.

## What SRAM actually is

A SNES cartridge contains:

- **ROM** (read-only, mapped to high banks) — your game code and
  assets.
- **SRAM** (read-write, battery-backed) — a small region of RAM on
  the cartridge, kept alive by a coin-cell battery soldered to the
  PCB. When the SNES powers off, the battery keeps the SRAM
  contents intact. When you turn the SNES on next, your save game
  is still there.

Sizes range from **2 KB** (early carts, Final Fantasy II) up to
**32 KB** (RPGs, Final Fantasy VI). The lib supports the standard
sizes via `SRAM_SIZE_2KB`, `SRAM_SIZE_4KB`, `SRAM_SIZE_8KB` (the
common case), `SRAM_SIZE_16KB`, `SRAM_SIZE_32KB`.

On real hardware, two failure modes:

1. **Battery dies** (typically after 10–20 years) → SRAM contents
   become garbage on next boot.
2. **Cartridge corruption** (cold-boot timing, marginal voltages) →
   SRAM contents become garbage *during* play.

Both are silent failures. Your code **must** validate SRAM on every
load — magic number, checksum, version field. A blind `sramLoad()`
into a struct gives you whatever was in that battery-backed memory,
correct or not.

## Memory map

The SNES MMU maps SRAM into bank space differently for LoROM vs HiROM:

| Layout | SRAM banks | SRAM addresses | Notes |
|---|---|---|---|
| **LoROM** | `$70`–`$7D` | `$70:0000–$77:7FFF` | 32 KB max (eight 4 KB regions). Most common. |
| **HiROM** | `$30`–`$3F` (mirror to `$B0`–`$BF`) | `$30:6000–$3F:7FFF` | 8 KB per bank in the lower half, 32 KB max. |
| **SA-1** | Different — see SA-1 chapter | — | SA-1 cart layouts depend on per-cart configuration. |

The lib's `sramSave`/`sramLoad` hide these details. You pass a
WRAM/RAM pointer and a byte count; the helper assembles the correct
24-bit SRAM address. Unless you're writing custom SRAM access code,
you don't need to know the bank/offset arithmetic.

## Build setup

Two changes to enable SRAM in a project:

### 1. Makefile

```makefile
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := mygame.sfc
ROM_NAME := MYGAME
USE_LIB    := 1
USE_SRAM   := 1            # turn on SRAM support
SRAM_SIZE  := 3            # 3 = 8 KB (the common choice)
LIB_MODULES := console sram dma background input
CSRC := main.c
include $(OPENSNES)/make/common.mk
```

The `SRAM_SIZE` numeric value matches the ROM header's `SRAMSIZE`
byte. Map:

| `SRAM_SIZE` | Cart SRAM | Header byte |
|:--:|:--:|:--:|
| `1` | 2 KB | `$01` |
| `2` | 4 KB | `$02` |
| `3` | 8 KB | `$03` |
| `4` | 16 KB | `$04` |
| `5` | 32 KB | `$05` |

Most homebrew picks 8 KB unless you genuinely need more. Larger SRAM
declarations affect emulator save-state allocation and on real flash
carts the actual reserved region.

### 2. ROM header

The build system fills the SNES ROM header automatically based on
`USE_SRAM` and `SRAM_SIZE`:

- `CARTRIDGETYPE` byte → `$02` (ROM + SRAM) or higher for chip+SRAM
  combos.
- `SRAMSIZE` byte → matches `SRAM_SIZE`.

Emulators read these bytes at ROM load to know how big to allocate
the SRAM buffer and where to persist the `.srm` file. Mismatched
declarations (Makefile says 8 KB, header says 2 KB) produce silent
"my saves disappear after 2 KB worth of data" bugs.

## The lib API

| Function | Purpose |
|---|---|
| `sramSave(data, size)` | Copy `size` bytes from WRAM `data` to SRAM offset 0. |
| `sramLoad(data, size)` | Copy `size` bytes from SRAM offset 0 to WRAM `data`. |
| `sramSaveOffset(data, size, offset)` | Save to SRAM at byte `offset` — used for multi-slot saves. |
| `sramLoadOffset(data, size, offset)` | Load from SRAM at byte `offset`. |
| `sramClear(size)` | Zero `size` bytes of SRAM starting at offset 0. Used for "delete save". |
| `sramChecksum(data, size)` | Compute an 8-bit XOR checksum of `data`. Use this for save-integrity validation. |

The byte arrays are flat — there's no filesystem on SRAM, just a
linear address space starting at offset 0. You pick the layout.

## The minimum viable save

Single save slot with magic-number validation:

```c
#include <snes.h>
#include <snes/sram.h>

typedef struct {
    u8  magic[4];      /* "SAVE" — detects valid save */
    u16 score;
    u8  level;
    u8  lives;
    u8  checksum;      /* XOR of preceding bytes */
} SaveData;

static SaveData save;

static void initDefaults(void) {
    save.magic[0] = 'S'; save.magic[1] = 'A';
    save.magic[2] = 'V'; save.magic[3] = 'E';
    save.score = 0;
    save.level = 1;
    save.lives = 3;
    save.checksum = 0;
}

static int loadSave(void) {
    sramLoad((u8 *)&save, sizeof(save));

    /* Validate magic */
    if (save.magic[0] != 'S' || save.magic[1] != 'A' ||
        save.magic[2] != 'V' || save.magic[3] != 'E') {
        return 0;   /* No valid save — first boot or corruption */
    }

    /* Validate checksum */
    u8 stored = save.checksum;
    save.checksum = 0;
    u8 computed = sramChecksum((u8 *)&save, sizeof(save));
    if (stored != computed) {
        return 0;   /* Corruption */
    }
    save.checksum = stored;
    return 1;       /* Valid save */
}

static void writeSave(void) {
    save.checksum = 0;
    save.checksum = sramChecksum((u8 *)&save, sizeof(save));
    sramSave((u8 *)&save, sizeof(save));
}

int main(void) {
    consoleInit();

    if (!loadSave()) {
        initDefaults();
    }

    /* Game loop … eventually call writeSave() on checkpoint */
}
```

The two non-obvious bits:

1. **Always validate**. SRAM contents on first boot are undefined —
   real hardware gives you whatever was in those memory cells when
   the cartridge was assembled. Magic-number-or-die is the canonical
   fix.
2. **Compute the checksum with the stored field zeroed**. Otherwise
   the very value you're computing depends on itself, and you'll
   produce a different checksum every save. The pattern: zero the
   field, compute, write the field, save.

## Multi-slot saves

For "Slot 1, Slot 2, Slot 3" RPG-style saves, use offset addressing:

```c
#define SLOT_SIZE   256
#define SLOT_OFFSET(n)  ((n) * SLOT_SIZE)

void writeSlot(u8 slot, SaveData *data) {
    sramSaveOffset((u8 *)data, sizeof(SaveData), SLOT_OFFSET(slot));
}

int loadSlot(u8 slot, SaveData *data) {
    sramLoadOffset((u8 *)data, sizeof(SaveData), SLOT_OFFSET(slot));
    /* … validate magic + checksum as above … */
}
```

The SDK's `examples/memory/save_game` is exactly this pattern: two
slots, distinct offsets, full struct round-trip with on-screen
verification of the loaded values.

## "Delete save"

Zero the relevant region:

```c
sramClear(sizeof(SaveData));        /* slot 0 only */
sramClear(N_SLOTS * SLOT_SIZE);     /* all slots */
```

The next load attempt fails magic-number validation and falls back
to defaults. (Don't try "delete by setting magic to invalid" — a
direct write is simpler and survives partial-write failures.)

## Worked pattern (the shipped example)

`examples/memory/save_game` — two save slots, each holding a
`SaveState { posX, posY, camX, camY }`. Press **A** to save test
data to slot 1, **B** to load it back; **X**/**Y** for slot 2.
Loaded values are displayed as hex on screen so you can verify
round-trip integrity. The example exercises:

- `sramSaveOffset` / `sramLoadOffset` for multi-slot.
- Struct serialisation through a flat byte array.
- The Makefile + header pair (`USE_SRAM := 1`, `SRAM_SIZE := 3`) —
  this is the example to copy when starting a new save-supporting
  project.

It does **not** implement magic-number / checksum validation —
that's left as the production-readiness step the tutorial above
documents. For real games, always add it.

## Gotchas

### 🔴 SRAM contents on first boot are undefined

Real hardware: whatever was in the cells at cart assembly. Emulators:
typically zero-filled, but flash carts and some emulator
configurations preserve previous content across cart swaps. Your code
**must** validate before trusting:

```c
if (save.magic != "SAVE" || sramChecksum(...) != save.checksum) {
    initDefaults();
}
```

This is *not optional*. A `sramLoad` into a struct without validation
gives you garbage and your game silently boots into corrupted state.
The save_game example (which doesn't validate) is fine for a tech
demo; production code is not.

### 🔴 Battery dies → silent corruption

A 10–20-year-old cartridge's coin-cell battery can fail. When it
does, SRAM contents drift toward "all $00" or "all $FF" or
random patterns. Your validation must catch both:

- **Magic number**: deterministic — fails if SRAM is all `$00`,
  fails if SRAM is all `$FF`.
- **Checksum**: deterministic — fails on partial corruption (one
  bad byte mid-struct).
- **Version field** (advanced): track save format version so old
  saves from a previous game build can be migrated or rejected.

### 🔴 `SRAM_SIZE` mismatch between Makefile and code

The lib's `sramSave/sramLoad` will happily write past the size
declared in the ROM header. On emulators, the `.srm` file may be
truncated; on real hardware, writes past the actual chip size wrap
around or vanish. Always set `SRAM_SIZE` in your Makefile to match
or exceed the largest offset your code writes.

A common bug: `SRAM_SIZE := 1` (2 KB) in Makefile, then code uses
`SLOT_SIZE = 1024` × 4 slots = 4 KB. Slots 2 and 3 silently fail
to persist on hardware that respects the header.

### 🟠 SRAM access requires no special timing

Unlike VRAM/CGRAM/OAM (which need VBlank or force blank), SRAM is
plain RAM mapped into the CPU's address space. You can read or
write it at any time during active display. The lib's helpers do
not call `WaitForVBlank()` and don't need to.

This is good news ergonomically — `sramSave` mid-frame is fine —
but newcomers from a "VBlank everything" mental model sometimes
add unnecessary `WaitForVBlank()` calls around SRAM operations.

### 🟠 Cannot store pointers in SRAM

A pointer in WRAM points to bank `$00` (or `$7E`) at some specific
address; that address is meaningful only while the cartridge runs
this exact ROM. If you save a pointer and reload it on a future
boot, the pointer might be valid (same ROM) or not (different game
or emulator's memory layout). Always serialise structs as
plain-old-data: counts, flags, indices, NEVER pointers.

### 🟠 Endianness — but you don't have to think about it

The SNES is little-endian (low byte first). `sramSave/sramLoad`
copies raw bytes; struct fields land in the order the C compiler
laid them out. Same compiler reading and writing → bit-perfect
round-trip. If you ever migrate save data between PVSnesLib and
OpenSNES (or any two compilers with different alignment rules),
**check struct layout first** — padding bytes can shift between
compilers.

### 🟡 Emulator persistence varies

- **snes9x** and **Mesen2**: persist SRAM to a `.srm` file alongside
  the ROM. Save game survives reset and emulator restart.
- **luna** (the test harness): SRAM is in-memory (captured in save-states; cross-run `.srm` persistence is a luna roadmap item) —
  only — does not persist across runs by default. The test harness
  resets SRAM between tests.
- **Real hardware via FXPak Pro**: persists to the SD card's
  per-ROM `.srm`. Survives power-off.

If you write a tutorial / demo that "saves on cycle X and loads on
boot", expect different behaviour across platforms. The
save_game example uses two slots specifically so you can verify
round-trip in a single boot cycle without depending on persistence.

### 🟡 `sramClear` is a memset, not a write-protect

`sramClear(size)` writes zeros to SRAM. It does **not** disable
SRAM, prevent future writes, or "format" the chip. Re-saving after
clear works as normal. The function is for "wipe save data" UI
flows, not for "secure erase".

## Cycle cost

SRAM access is plain CPU loads/stores via the cartridge bus, around
8 master cycles per byte. A 256-byte save:

- `sramSave(buffer, 256)`: ~256 × 8 = ~2 K cycles, plus a few
  hundred for setup. Well under 1 ms total. Imperceptible.

A full 8 KB save: ~64 K cycles, ~3 % of a 60 Hz frame. Still small
enough to do during gameplay if you have to (auto-save).

There's no DMA path for SRAM; the lib uses CPU MVN/MVP block-move
instructions which are fast enough that DMA wouldn't materially
help.

## See also

- `lib/include/snes/sram.h` — full API reference.
- `lib/source/sram.asm` — implementation (331 LOC, all assembly).
- [`examples/memory/save_game`](../../examples/memory/save_game/README.md) — multi-slot save demo with on-screen
  hex verification.
- [`examples/memory/hirom_demo`](../../examples/memory/hirom_demo/README.md) — companion read for the HiROM bank
  layout that affects SRAM mapping.
- [Graphics tutorial](graphics.md) — for the BG/text rendering used to
  show save state on screen.
- [`KNOWN_LIMITATIONS.md`](../../KNOWN_LIMITATIONS.md) — covers the
  bank-mapping rules and the emulator persistence variance.
- [SA-1 tutorial](sa1.md) — relevant if your project uses SA-1; SA-1
  cart layouts handle SRAM differently per cartridge.
