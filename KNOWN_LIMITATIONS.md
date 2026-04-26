# Known Limitations

This file is the canonical, **public** list of known traps and limitations of
the OpenSNES SDK. Skim it before you start a project; refer back when something
mysterious happens. Each entry has a severity, a description, and a mitigation
or workaround.

> **Why this exists.** OpenSNES inherited several "silent failure" footguns
> from the 65816/SNES architecture and from its custom toolchain. We document
> them honestly here rather than leaving readers to discover them at runtime.
> If you find a new one, append it.

---

## Severity legend

- **🔴 Silent corruption** — wrong behaviour with no error. The dangerous class.
- **🟠 Silent build issue** — bad ROM produced, no error at compile/link.
- **🟡 Toolchain quirk** — manageable once you know it; loud failure if you don't.
- **🟢 Mitigated** — was a footgun, now caught by build/CI/runtime.

---

## Runtime traps

### 🔴 VRAM writes outside VBlank are silently dropped
The PPU only accepts VRAM writes during VBlank (NMI) or forced blank
(`setScreenOff()`). Outside those windows, `dmaCopyVram()` and direct register
writes succeed without error but **do nothing** — you get garbage tiles or
nothing at all on screen.

**Mitigation:** wrap heavy VRAM work between `setScreenOff()` / `setScreenOn()`
during init, and use `WaitForVBlank()` before per-frame DMA. The `1-page-per-VBlank`
pattern (split tilemap DMAs into 2 KB chunks) is the canonical solution for
large transfers.

### 🔴 VBlank DMA budget is ~4 KB per frame
NMI runs for roughly 35,000 master cycles after vblank starts. DMA costs
~8 cycles per byte, leaving a budget of about **4 KB total** per VBlank for
OAM + tilemap + audio + scroll. Exceed it and the PPU starts displaying
mid-DMA → garbage tiles for one or more rows.

**Mitigation:** count your DMAs. If a frame needs > 4 KB, split across multiple
VBlanks (1-page pattern) or move the heavy load into a forced-blank window.

### 🔴 WRAM data port `$2180–$2183` is NOT safe in NMI
Main-thread code writes multi-byte sequences via `$2180` after setting an
address with `$2181-$2183`. If NMI fires mid-sequence and any code in the NMI
path touches those ports, the address pointer is silently corrupted and the
main thread resumes writing garbage to a wrong location.

**Mitigation:** the SDK's NMI handler in `templates/crt0.asm` never touches
`$2180-$2183`. If you write a custom `nmiSet()` callback, do not use any
function that goes through these ports.

### 🟢 Bank $00 ROM overflow → garbage const reads (caught at link time)
`static const` arrays each get a SUPERFREE section. The compiler emits 16-bit
addresses (`lda.l $XXXX,x` where `XXXX < $8000` is bank $00 implicitly), so if
bank $00's 32 KB ROM area fills, new const sections spill to bank $01+ but the
generated code still reads bank $00 → returns garbage data.

**Mitigation (active since P1.5):** `make/common.mk` runs
`devtools/symmap/symmap.py --check-bank0-overflow` after every link.
String-literal spills (the worst case) fail the build with a "Bank $00 ROM
overflow" message. Tight free-space (< 2 KB) prints a soft warning. Set
`SKIP_BANK0_CHECK=1` to bypass for debugging.

If you hit this:
- Combine related const arrays into one large array + offset macros
- Move large const data to RAM (drop the `const`)
- Use assembly with explicit bank addressing for very large blobs

### 🔴 All C RAM must live below $2000 (DP wraparound)
`sta.l $0000,x` and friends always access bank $00. If the linker places a
RAM section above $2000, C code reading via short addressing reads from bank
$00 instead — silent corruption.

**Mitigation:** the templates' memory maps reserve `$00:0000-$1FFF` for C RAM
and don't put C-accessible variables higher. Custom RAMSECTION users must respect
this. If you really need data above $2000, access it via assembly with explicit
bank prefix (`lda.l $7E:NNNN,x`).

---

## Build-time / linker traps

### 🟢 `data_init_end.o` MUST be linked last (enforced)
The data-init copy loop scans from `data_init_start.o` until the sentinel in
`data_init_end.o`. If the latter isn't last, init walks past valid data and
copies garbage into WRAM at boot.

**Mitigation:** `make/common.mk` builds the link list in fixed order with
`data_init_end.o` always at the end (`LINK_OBJS := ... data_init_end.o`).
Don't override `LINK_OBJS` from your example Makefile.

### 🟠 cc65816 pushes function args **LEFT-TO-RIGHT**
PVSnesLib (tcc816) pushes args right-to-left, the C convention. cc65816 pushes
**left-to-right**. ASM functions ported from PVSnesLib without changing the stack
offsets read the wrong arguments — the first arg might be at offset 8,s instead
of 6,s. The function compiles, links, and corrupts the stack at runtime.

**Mitigation:** the full calling convention is documented in
[`compiler/ABI.md`](compiler/ABI.md), with worked examples and a port checklist.
When porting an ASM function from PVSnesLib, walk through the offsets explicitly.
Function pointers called from C follow the same convention.

### 🟠 `volatile` in loops crashes QBE
QBE's SSA pass can't always handle `volatile`. Loops with `volatile` reads/writes
typically crash with a backend assertion or generate broken code.

**Mitigation:** for hardware-poll loops, use a regular global instead of
`volatile` — the SDK explicitly avoids `volatile` in flag handshakes (`vblank_flag`,
`oam_update_flag`). If you absolutely need MMIO semantics, use a small ASM
helper.

### 🟠 WLA-DX loses `.ACCU` / `.INDEX` tracking after branch merges
WLA-DX tracks the current accumulator/index register width across `rep`/`sep`
instructions, but merging branches resets that tracking. The next instruction
gets assembled at the wrong width — for example a `cpx #imm` becomes 2 bytes
instead of 3, and every subsequent address is off by one.

**Mitigation:** add an explicit `.ACCU 8`/`.ACCU 16` and `.INDEX 8`/`.INDEX 16`
after every `rep`/`sep` and at every block boundary in hand-written ASM. The
SDK's templates and library `.asm` files follow this rule. If you write your
own ASM, audit for missing markers when you see misaligned-instruction bugs.

---

## Toolchain & emulator coverage

### 🟡 SA-1 SIWP register init is an unsourced assumption
`templates/crt0.asm:557+` initialises the SA-1 by writing `$FF` to register
`$002229` (SIWP) with the comment "*maybe bit=1 means WRITABLE*". This came
from observation, not from a published spec. If a future SA-1 cartridge revision
behaves differently, the init silently fails and the chip is half-configured.

**Mitigation:** none currently. If your project depends on SA-1 in a serious
way, exercise it on real hardware before shipping. We track this in
`memory/enhancement_chips_research.md`.

### 🟡 SuperFX: Mesen2 has a confirmed backward-branch bug
Mesen2 is the de-facto SNES dev emulator, but its SuperFX core mis-emulates
backward branches in some cases — a ROM that runs correctly on real hardware
or in bsnes can hang or render incorrectly in Mesen2.

**Mitigation:** for SuperFX work, **bsnes is the reference emulator**. The
opensnes-emu test harness uses snes9x's libretro core which is also fine for
SuperFX. Mesen2 is still useful for debugging — just don't trust it as the
source of truth for SuperFX behaviour.

### 🟡 SuperFX C support is intentionally absent
The GSU has its own RISC ISA with no C compiler. All SuperFX code must be
written in GSU assembly (built via the `wla-superfx` assembler). The C side
of a SuperFX project orchestrates GSU jobs and reads results — it doesn't
generate GSU code.

**Mitigation:** documented in `docs/tutorials/sa1.md` and the SuperFX example
under `examples/memory/superfx_*`. Plan accordingly: heavy compute lives in
GSU assembly, not in your C main.

### 🟢 Compiler submodule drift (caught by verify-toolchain)
`compiler/{cproc,qbe,wla-dx}` are forks with downstream patches. A
`git submodule update --remote` would advance them past tested commits and
could re-introduce silent codegen regressions (this has happened: `mktype()`
UB, struct-init UB, etc.).

**Mitigation (active since P2.1):** `compiler/PINS.md` records the canonical
SHA for each submodule. `make verify-toolchain` (and CI) compare current HEADs
against the file and fail on drift.

---

## Compiler optimisation gaps

These are **planned but not yet implemented** in cc65816. The five `[KNOWN_BUG]`
entries in the test suite (`--allow-known-bugs`) correspond to these:

### 🟡 Tail call optimisation (TCO) not wired
QBE upstream supports `jml` instead of `jsl + plx + plx + rtl` for pure
pass-through wrappers, but the OpenSNES backend doesn't emit it. Wrapper
functions pay the full call+return overhead. Fixes 3 compiler tests when
implemented (`tail_call`, `lazy_rep20`, `plx_cleanup`).

### 🟡 A-cache through `pha` not implemented
The A-register cache (Phase 1) invalidates on every call. Pushing the same
local through `pha` for two consecutive calls reloads it from the stack each
time. A single A-cache extension across pure pushes would skip the second
load. Fixes 1 compiler test.

### 🟡 Stale `leaf_opt=1` marker for non-leaf functions
Phase 5b removed the `leaf_opt` gate so the optimisations apply to non-leaf
functions too, but the comment marker in the generated ASM still says
`leaf_opt=0` even when the optimisation is active. Cosmetic — the optimisation
fires, just the diagnostic comment is wrong. Fixes 1 compiler test.

---

## Type & ABI gotchas

### 🟡 `int` is 32 bits, `long` is 64 bits on cc65816
Counter-intuitive on a 16-bit CPU. cproc was originally a host-target compiler
and `sizeof(int)` in the IR is 4. Code that expects 16-bit `int` (e.g. ports
from PVSnesLib's tcc816 which uses 16-bit `int`) will silently produce 32-bit
arithmetic and double the work.

**Mitigation:** always use the SDK's fixed-width types: `u8`, `u16`, `s16`,
`u32`. Never use bare `int`/`long`/`short` in performance-sensitive code.
The SDK's `lib/include/snes/types.h` provides the canonical typedefs.

### 🟡 Sprite palettes start at CGRAM offset 128, not 0
Standard SNES quirk that surprises everyone once. The first 128 colours of
CGRAM are BG palettes (palettes 0-7); sprite palettes start at offset 128
(palettes 0-7 mapped to colours 128-255). Loading a sprite palette via
`dmaCopyCGram(pal, 0, ...)` puts it where BG palette 0 lives — wrong colours
for sprites.

**Mitigation:** use `OBJ_CGRAM_BASE` (= 128) and the `OBJ_CGRAM_PAL(n)` helper
defined in `lib/include/snes/sprite.h`. The naming convention separates BG
(`PAL_n`) from OBJ (`OBJ_PAL_n`) palettes.

---

## Where to add new entries

When you discover a new silent failure or hard-to-debug constraint:

1. Add an entry here under the right category, with a severity marker.
2. Note any active mitigation (link to the file/check that catches it).
3. Cross-reference from `CLAUDE.md` if the issue is relevant to AI-assisted
   work, but **always** keep the canonical entry in this file.

If the issue gets a permanent automated mitigation (build-time check, CI gate,
runtime guard), update the severity from 🔴/🟠/🟡 to 🟢 and describe the
mitigation alongside the original problem.
