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

### 🟢 WRAM data port `$2180–$2183` race in NMI (caught at build time)
Main-thread code writes multi-byte sequences via `$2180` after setting an
address with `$2181-$2183`. If NMI fires mid-sequence and any code in the NMI
path touches those ports, the address pointer is silently corrupted and the
main thread resumes writing garbage to a wrong location.

**Mitigation (active since chantier E1, 2026-05-09):** `make/common.mk`
runs `devtools/check_nmi_wram_race.py` after every link. The lint walks
the call graph from every NMI callback root (NmiHandler + functions
registered via `nmiSet`/`nmiSetBank`) and **fails the build** if any
reachable function writes to `$2180-$2183`. Lib + crt0 are audited and
treated as a black box; the lint targets user code in NMI callbacks,
where the actual risk lives. Bypass for a single build with
`SKIP_NMI_RACE_CHECK=1`. Regression suite:
`python3 devtools/test_check_nmi_wram_race.py` (6 cases).

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

### 🟢 WLA-DX loses `.ACCU` / `.INDEX` tracking after branch merges (lint-enforced)
WLA-DX tracks the current accumulator/index register width across `rep`/`sep`
instructions, but merging branches resets that tracking. The next instruction
gets assembled at the wrong width — for example a `cpx #imm` becomes 2 bytes
instead of 3, and every subsequent address is off by one.

**Mitigation (active since v0.15.1):** `devtools/lint_asm.py` enforces an
explicit `.ACCU 8`/`.ACCU 16` and `.INDEX 8`/`.INDEX 16` after every
`rep`/`sep` and at every block boundary in hand-written ASM. The lint runs
in CI (`asm-markers` job in `.github/workflows/lint.yml`) and as part of the
Makefile pipeline. The SDK's templates and library `.asm` files all carry the
required markers (490 of them, added by the lint's `--fix` mode). If you
write your own ASM and CI starts flagging your file, run
`python devtools/lint_asm.py --fix path/to/your.asm` and inspect the diff
before committing.

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

### 🟢 SuperFX: snes9x does not detect the GSU chip — Mesen2-headless covers it in CI
The opensnes-emu CI test harness runs ROMs through snes9x's libretro core
in the main `Visual Regression` phase. For SuperFX examples, snes9x's
chip-detection logic does not pick up the GSU from our ROM header
configuration — the example boot path renders "GSU: NOT DETECTED" and
the actual GSU code never runs. Visual baselines of those screenshots
happily compare equal frame-to-frame, but the snes9x phase proves only
"boots in snes9x", not "runs SuperFX correctly".

**Mitigation (active since P3.4):** the test suite ships a separate
`Mesen2 Visual Regression` phase
(`tools/opensnes-emu/test/phases/visual-mesen2.mjs`) that runs the
vendored Mesen2 binary in `--testrunner` mode against the chip-using
examples (currently 4 SuperFX/SA-1 ROMs). Mesen2 detects the GSU
correctly and is the reference emulator for SuperFX. The phase is
gated by the presence of the Mesen2 binary
(`scripts/install-mesen2.sh` fetches it once per CI run, cached
locally), so it is a no-op on contributor machines that haven't
installed Mesen2 yet but mandatory in CI. Both phases together give
SuperFX ROMs end-to-end coverage. (An earlier version of this entry
blamed Mesen2 for a "backward-branch bug"; re-validation showed the
original observation conflated a snes9x mis-run with a Mesen2 bug.
Reference behaviour was Mesen2's all along.)

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

One known gap remains in cc65816 today (the lone `[KNOWN_BUG]` entry surfaced
by `--allow-known-bugs` in the test suite). Earlier entries that lived here —
tail call optimisation and A-cache-through-`pha` — have shipped (chantiers
C.1 / C.2.1 / C.2.2 wired TCO; C.6's audit confirmed A-cache through `pha`
already worked, the test had been stale).

### 🟡 Stale `leaf_opt=1` marker for non-leaf functions
Phase 5b removed the `leaf_opt` gate so the optimisations apply to non-leaf
functions too, but the comment marker in the generated ASM still says
`leaf_opt=0` even when the optimisation is active. Cosmetic — the optimisation
fires, only the diagnostic comment is wrong. Surfaces as a single `[KNOWN_BUG]`
in the compiler test phase (`nonleaf_frameless`).

**Mitigation:** none needed for codegen correctness. If you read the comment
markers to verify optimisation state, fall back to inspecting the actual ASM
shape (a non-leaf frameless function has no `sec`/`sbc.w` prologue and no
`adc.w`/`tas` epilogue, with arguments accessed via `4,s`/`6,s` directly).

---

## Type & ABI gotchas

### 🟢 `int` and `long` type sizes match the w65816 target (since chantier A1)
`sizeof(int) == 2`, `sizeof(unsigned int) == 2`, `sizeof(long) == 4`,
`sizeof(unsigned long) == 4`. `long long` stays at 8 per C99. These match the
canonical SNES expectation: `int` is the native 16-bit word, `long` is 32 bits.

Historical note: pre-A1, `int` was 4 bytes and `long` was 8 (cproc was inherited
as a host-target compiler with `sizeof(int) = 4` hardcoded). Code that expected
16-bit `int` would silently produce 32-bit arithmetic and double the work.
That trap is gone — bare `int` is now correct on this target.

`u8` / `u16` / `s16` / `u32` from `lib/include/snes/types.h` remain the
preferred types for **portability** (they make the code intent explicit and
work identically across compilers), but using bare `int` no longer doubles
your cycle cost.

### 🟡 Pointer IR size is still 8 bytes (chantier A6 — pending)
The companion fix for pointer types was deliberately deferred into its own
chantier (A6) because reducing pointer storage cascades through QBE w65816's
indirect-call emit pass: the bank byte for `jml [tcc__r9]` is hardcoded as
`lda #$00` instead of read from the pointer's actual storage. Until A6 lands,
function pointers are stored as 8 bytes (low + high + 4 bytes padding) and
indirect calls assume bank `$00` — which works because the SDK's framework
opt-ins (`scene`, `gameloop`) happen to dispatch to functions in bank `$00`
via the lib's SUPERFREE'd code that lands there.

**Mitigation (today):** function pointers continue to work as before. No user
code change needed.

**A6 plan:** pointer IR size 8 → 4 (24-bit address + 1 byte alignment),
indirect-call emit reads bank byte from pointer storage, lib API can drop
the `*Bank` variant pattern entirely. Tracked in the structural-defects
catalogue.

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

## Performance traps

### 🟡 `oamSet()` has framesize=158 — visible jitter past ~3 calls per frame
The QBE backend allocates 158 bytes of SSA temporary storage for every
`oamSet(id, x, y, attr, size, tile)` invocation, manipulated on the stack at
each call site. On a 3.58 MHz CPU that overhead is visible: more than ~3
`oamSet()` calls per frame in the main loop produces jerky movement on real
hardware (and shows up as lag-frame spikes in the test suite). The function
itself is correct — the cost lives in the calling convention, not in the OAM
write.

**Mitigation:** for any sprite that updates every frame, write directly to
the `oamMemory[]` shadow buffer and let the NMI handler DMA it to OAM. The
SDK ships two ergonomic helpers:

- `oamSetFast(id, x, y, attr, size, tile)` — drop-in macro replacement for
  `oamSet()` that compiles to direct memory writes (see
  `lib/include/snes/sprite.h`'s "Fast Macro Sprite API" section).
- `oamSetXYFast(id, x, y)` — position-only update, the most common
  per-frame case.

Both are documented in `sprite.h` next to the function form. Use the
function `oamSet()` for one-shot setup at scene init (where the 158-byte
frame is paid once and clarity wins); use the macros for the per-frame
update path. The breakout, collision_demo, animated_sprite, mouse, and
superscope examples all switched to direct `oamMemory[]` writes for this
reason — see their READMEs for worked examples.

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
