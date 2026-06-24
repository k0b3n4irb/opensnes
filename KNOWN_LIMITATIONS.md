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

### 🟢 Bank $00 ROM overflow → garbage const reads **when dereferenced in C** (caught at link time)
The limit is narrower than it looks. `static const` arrays each get a SUPERFREE
section; if bank $00's 32 KB ROM area fills, new const sections spill to bank
$01+. What breaks depends on **how you read the data**:

- **Passed to a lib DMA/asset function — works in ANY bank.** `dmaCopyVram`,
  `dmaCopyCGram`, `dmaCopyVramMode7`, `LzssDecodeVram`, `mapLoad`, etc. take a
  4-byte pointer whose high byte carries the real bank, and the asm reads it
  (`dmaCopyVram` does `lda 11,s → sta.l $4304`, the DMA source-bank register). So
  tiles / palettes / maps / fonts can live in any bank — this is the common case.
- **Dereferenced directly in C (`ptr[i]`, `*ptr`) — bank $00 only.** The compiler
  still emits `lda.l $XXXX,x` (implicit bank $00) for a C deref, so a `static
  const` array you index in C must fit bank $00 or it returns garbage. (This is
  the remaining half of chantier A6.)

**Mitigation (active since P1.5):** `make/common.mk` runs
`devtools/symmap/symmap.py --check-bank0-overflow` after every link.
String-literal spills (the worst case) fail the build with a "Bank $00 ROM
overflow" message. Tight free-space (< 2 KB) prints a soft warning. Set
`SKIP_BANK0_CHECK=1` to bypass for debugging.

If you hit this:
- **Prefer the DMA path:** keep big assets `const` and feed them to the lib's
  DMA/asset functions — they already work from any bank (see above). For a
  runtime-computed bank, the explicit `dmaCopyVramBank(src, bank, …)` /
  `dmaCopyCGramBank(src, bank, …)` variants take the bank as a parameter.
- Combine related const arrays read in C into one array + offset macros.
- Move large C-dereferenced const data to RAM (drop the `const`).
- Use assembly with explicit bank addressing for very large blobs.

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

### 🟢 `volatile` is preserved through QBE (since chantier A2, 2026-05-09)
The C `volatile` qualifier on a load or store now survives the cproc → QBE
pipeline. cproc tags the instruction with a `volat` keyword in the
intermediate IR; QBE's `loadopt` (load forwarding), `promote` (alloca-to-
register promotion), and `gcm` (dead-code elimination of unused loads) all
honour the marker and leave volatile accesses intact, so each access to a
`volatile` object produces its own load/store as the C standard requires.

Pre-A2 the SDK avoided `volatile` entirely (a comment in
`compiler/cproc/qbe.c` literally said *"For now, treat volatile stores like
normal stores for SNES hw access"* — the qualifier was discarded). The
runtime still favours plain globals for handshake patterns (`vblank_flag`,
`oam_update_flag`) because the cycle cost is the same and the contract is
clearer for NMI-vs-main races, but user code is now free to use `volatile`
for MMIO polling, hardware-register reads, and any pattern where the C
standard's "each access is a side effect" semantics matter.

The compiler-test phase enforces this with assertion counts on
`test_volatiles.c` — three sequential reads of `hw_status` must produce at
least three `lda.w hw_status` instructions, and so on.

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

### 🟡 SA-1 SIWP/CIWP write-protection polarity is disputed
`templates/crt0.asm` enables SA-1 I-RAM access by writing the SIWP register
`$002229` (twice: early init ~`:519-526`, and the SA-1 boot block
~`:636-642`), value `$FF`. **The polarity of this register is genuinely
contested between documentation and emulators:**

- The [Super Famicom Dev Wiki](https://wiki.superfamicom.org/sa-1-registers)
  and fullsnes say each bit *enables* write-protection for one 256-byte
  I-RAM page (bit=1 protects, bit=0 writable), i.e. `$00` = all writable.
- **Mesen2** (this project's accuracy reference) and **snes9x** behave the
  *opposite* way. Tested empirically (2026-06-20): with `$FF` the crt0
  I-RAM self-test passes (`sa1_status=$A5`); with `$00` the SNES-CPU write
  to I-RAM is blocked and the self-test fails (`sa1_status=$FF`).

OpenSNES writes `$FF` because that is what works on the emulators we
validate against. A `$00` "fix" (matching the wiki) **breaks** SA-1 in
Mesen2/snes9x and was reverted — do not re-apply it without a real-hardware
test that proves the wiki polarity.

**Mitigation:** the crt0 self-test (`:639-647`: write `$42` to I-RAM, read
back, fail to `sa1_status=$FF`) means a wrong choice is **detected** at
runtime — `sa1IsReady()` returns false unless the SA-1 reaches `$A5`, so
the failure is not silent. If your project depends on SA-1, verify on a
real cartridge before shipping. Background + sources:
`.claude/notes/tech/enhancement_chips_research.md`.

### 🟢 SuperFX / SA-1 chips: covered natively (resolved by the luna migration)
**Resolved 2026-06-20.** The previous harness (snes9x compiled to WASM) could
not detect the GSU from our ROM header — SuperFX examples rendered "GSU: NOT
DETECTED" and the chip code never ran, so the suite needed a separate vendored
**Mesen2** phase under `xvfb` just for the 4 chip ROMs. The test harness now
runs on [**luna**](https://github.com/k0b3n4irb/luna), a cycle-accurate native
emulator that detects and executes **SA-1, Super FX (GSU) and DSP-1** directly
(verified: `superfx_hello` → "ALL TESTS PASSED", `superfx_3d` → GSU-rendered 3D
cube, `sa1_hello`/`sa1_starfield` → `sa1_status=$A5`). The chip-ROM side channel
and the whole snes9x-WASM + Mesen2 + xvfb stack are gone. See
`.claude/notes/chantiers/luna_migration.md`.

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

**None as of chantier A3 (2026-05-09).** The compiler-test phase runs
clean without the `--allow-known-bugs` escape that used to gate tail
call optimisation on wrappers, A-cache-through-`pha`, lazy `rep #$20`
emission, and the `leaf_opt=1` marker on non-leaf functions.
Investigation found every optimisation had already shipped (chantiers
C.1 / C.2.1 / C.2.2 / C.6); the test markers were stale, not the
codegen. The CI workflow no longer passes `--allow-known-bugs`; any
regression on these surfaces immediately as a hard failure. The
`leaf_opt=N` comment marker in generated ASM is also accurate — it
reports the actual optimisation state per function (not always 1
or always 0), so reading it is reliable.

MSYS2-specific cproc segfaults (struct pointer init, nested structs,
unions, string initializers, `.rodata`-vs-`RAMSECTION` flake) remain
behind `knownBug()` calls in `compiler-tests.mjs`, gated on the
`r.segfault` signal — they only fire on Windows / MSYS2 builds and
are tracked in `.github/workflows/msys2_cproc_diagnostic.yml`. Linux
and macOS CI never trigger them.

---

## Type & ABI gotchas

### 🟢 `int` and `long` sizes AND semantics match the w65816 target

**Sizes** (since chantier A1, 2026-05-08):
`sizeof(int) == 2`, `sizeof(unsigned int) == 2`, `sizeof(long) == 4`,
`sizeof(unsigned long) == 4`. `long long` stays at 8 per C99. These match the
canonical SNES expectation: `int` is the native 16-bit word, `long` is 32 bits.

**Semantics** (since chantier A1-followup, 2026-05-16):
`long` arithmetic flows through the QBE w65816 backend's Kl-class handlers,
not the silently-truncating Kw path. Every operator — add/sub with carry,
shifts with cross-half rol, multiply via `__mul32`, divide via
`__[s]divmod32`, cascaded compares high→low, bitwise, sign/zero extend —
preserves full 32-bit width.

Historical traps that are GONE:
- Pre-A1, `int` was 4 bytes and `long` was 8 (cproc was inherited as a
  host-target compiler with `sizeof(int) = 4` hardcoded). Code that
  expected 16-bit `int` silently produced 32-bit arithmetic and doubled
  the work.
- Pre-A1-followup, `long` carried the correct *size* (4 bytes) but
  cproc mapped it to QBE class `'w'` (16-bit Kw), so every arithmetic
  op silently truncated to 16 bits. Five reproductions are kept in
  the chantier note (`(long)40 << 16` folding to zero, `long *= long`
  routing to `__mul16`, etc.). All five are now closed.

`u8` / `u16` / `s16` / `u32` from `lib/include/snes/types.h` remain the
preferred types for **portability** (they make the code intent explicit
and work identically across compilers), but using bare `int` / `long`
is correct on this target.

### 🟢 Pointer IR size is 4 bytes (chantier A6+A7, 2026-05-15)

Pointers are now QBE class Kl: 24-bit address (low 16 + bank byte) + 1 byte
alignment, 4 bytes total. The indirect-call emit pass reads the bank byte
from the pointer's high half — `jml [tcc__r9]` after `sta.b tcc__r9` (low
16) + `sta.b tcc__r9+2` (bank byte) — so function pointers in any bank
work without a `*Bank` API variant. Shipped in v0.19.0 (2026-05-15).

Historical note: pre-A6, function pointers were 8 bytes (low + high + 4
bytes padding) and indirect calls hardcoded `lda #$00` for the bank byte.
Framework opt-ins (`scene`, `gameloop`) worked because their dispatch
targets happened to land in bank 0 via SUPERFREE'd lib code. Any
pointer to a bank-1+ function would have silently jumped to bank 0.
That trap is gone.

### 🟡 C function returning `long` does not propagate the high half through the call

A latent issue surfaced (but not fixed) during the A1-followup chantier
(Session 7, 2026-05-16). For a C function `long f(...)`:

- Cproc emits `Jretl` which `emitload`s only the low 16 of the return
  value into `A` before `rtl`.
- The caller's `emitstore(i->to)` after the `jsl` only writes A to the
  low half of the destination Kl temp. The high half is left at whatever
  the slot previously contained.

In practice no shipping SDK code returns a `long` whose high half is
read by the caller (lib functions return `s16` / `u16` / `fixed`; the
A1-followup test harness exercises Omul / Odiv via runtime helpers, not
return values). The five originally-tracked A1-followup bugs all pass
because they go through the helper path (which uses named scratch slots
for the high half) or only inspect the low half.

**Mitigation today:** don't write a `long`-returning C function whose
high half is consumed across the call. If you need a 32-bit return,
either inline the math at the call site, route it through a runtime
helper that stores into a named bank-0 slot, or pass a `long *out`
parameter.

**Proper fix (when this becomes a real problem):** extend `Jretl` to
also store the high half to a known location (e.g., `tcc__r0+2`) before
`rtl`, and have the Ocall path read it back. Estimated 1-2 days of
chantier work; not yet tracked in `STRUCTURAL_DEFECTS.md` because no
shipping code triggers it.

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

### 🟢 `oamSet()` framesize cliff — RESOLVED via ASM rewrite (2026-03-03)
The QBE backend used to allocate 158 bytes of SSA temporary storage for
every `oamSet(id, x, y, attr, size, tile)` invocation, manipulated on the
stack at each call site. More than ~3 calls per frame produced visible
jitter on real hardware.

**Fix shipped** in commit `39dbff8` (2026-03-03): `oamSet()` was rewritten
in hand-optimised assembly (`lib/source/sprite_oamset.asm`) with
**framesize=0** — direct stack-relative addressing, no SSA temps,
~100 cycles saved per call. The function is fully ABI-compatible; user
code is unchanged.

The macro-form helpers `oamSetFast()` / `oamSetXYFast()` are still
shipped for callers that want explicit "no function call at all" intent
(zero JSL + zero RTL + zero frame), but the perf cliff that made them
mandatory for per-frame paths no longer exists. The breakout, mouse,
and similar examples continue to use direct `oamMemory[]` writes by
preference, not necessity.

**Lingering observation (informational, 🟢 severity)**: other multi-arg
C helpers still carry larger-than-ideal framesizes — `oamSetX` (148),
`oamDrawMeta` (142), `oamDrawMetaFlip` (200), `collideRectEx` (176),
`hdmaColorGradient` (162). A 2026-05-13 audit confirmed none of these
are per-frame hot paths in shipping examples (`oamSetX` has 0 example
callers; the others have 0–1). If one becomes hot for a future user,
the same tactic (ASM rewrite) is available, OR the catalogued QBE
coalescer chantier (`.claude/STRUCTURAL_DEFECTS.md` A4) can be revived.

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
