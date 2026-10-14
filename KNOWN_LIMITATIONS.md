# Known Limitations {#known_limitations}

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
- **🟢 Mitigated** — the trap is real, and the build, the library or the
  compiler catches it or hides it.

Bugs that have been fixed are not listed here: `CHANGELOG.md` records them.

---

## Runtime traps

### 🔴 VRAM writes outside VBlank are silently dropped
The PPU only accepts VRAM writes during VBlank (NMI) or forced blank
(`setScreenOff()`). Outside those windows, `dmaCopyVram()` and direct register
writes succeed without error but **do nothing** — you get garbage tiles or
nothing at all on screen. Writes are ignored during H-blank too, and the
VRAM address in VMADD still increments on an ignored write — so a transfer
that runs past the end of VBlank resumes at the wrong address and shifts
everything written after it.

**Mitigation:** wrap heavy VRAM work between `setScreenOff()` / `setScreenOn()`
during init, and use `WaitForVBlank()` before per-frame DMA. The `1-page-per-VBlank`
pattern (split tilemap DMAs into 2 KB chunks) is the canonical solution for
large transfers.

### 🔴 VBlank DMA budget is ~4 KB per frame
VBlank lasts 37 scanlines on NTSC with the standard 224-line display —
50,468 master cycles, 48,988 of them outside the DRAM-refresh pause (22
lines with overscan; the figures and their sources are in
[docs/PERF.md](docs/PERF.md)). DMA costs 8 master
cycles per byte, so the raw ceiling is ~6 KB; after the NMI handler's
own work (OAM DMA, scroll sync, joypad, callback) the practical budget
is about **4 KB total** per VBlank for tilemap + audio + scroll.
Exceed it and the PPU starts displaying mid-DMA → garbage tiles for
one or more rows.

**Mitigation:** count your DMAs. If a frame needs > 4 KB, split across multiple
VBlanks (1-page pattern) or move the heavy load into a forced-blank window.

### 🟢 WRAM data port `$2180–$2183` race in NMI (caught at build time)
Main-thread code writes multi-byte sequences via `$2180` after setting an
address with `$2181-$2183`. If NMI fires mid-sequence and any code in the NMI
path touches those ports, the address pointer is silently corrupted and the
main thread resumes writing garbage to a wrong location.

**Mitigation:** the post-link check (`opensnes-rom check`, run by
`make/common.mk` after every link) walks the call graph from every NMI
callback root (the NMI handler and the functions registered with `nmiSet`)
and **fails the build** if any reachable function writes to `$2180-$2183`. Lib + crt0 are audited and
treated as a black box; the lint targets user code in NMI callbacks,
where the actual risk lives. Bypass for a single build with
`SKIP_NMI_RACE_CHECK=1`.

### 🟢 Bank $00 ROM: code only — C const data lives in the asset banks
`static const` arrays, string literals and initialised const structs are
emitted as `.SECTION ".rodata.N" SEMISUPERFREE BANKS ASSET_BANKS`: the
linker places them in the memory map's asset banks (highest first) and
bank $00 keeps the code. This is safe because every way C reaches const
data carries the bank:

- **Passed to a lib DMA/asset function — any bank.** `dmaCopyVram`,
  `dmaCopyCGram`, `dmaCopyVramMode7`, `lzssDecodeVram`, `mapLoad`, etc. take a
  4-byte pointer whose high byte is the real bank, and the asm reads it
  (`dmaCopyVram` does `lda 11,s → sta.l $4304`, the DMA source-bank register).
- **Dereferenced in C (`tab[i]`, `*p`, `p->field`) — any bank.** A read
  through a `const` pointee is compiled with far addressing:
  `lda.l sym`, `lda.l sym,x` or `[tcc__r9]`.
- **The one bank-blind path left:** casting the `const` away and reading
  through a plain pointer (`*(u8 *)tab`) — the compiler then emits the
  bank-$00-implicit `lda.l $0000,x`. The link fails on a symbol in bank
  $01+ read with bank-$00 addressing (`opensnes-rom check`), which
  catches the read that names its symbol. It cannot see a plain pointer
  that travels — stored, or passed to a function, then read there: that
  read is silently wrong. The cast is the only way in: a string literal
  is `const char[]`, so the compiler refuses every implicit loss of
  `const`.

HiROM: every HiROM unit carries `.BASE $C0` so a datum at offset $0000 of
a high bank is addressed in the full 64 KB view (`$Cn:0000`); the wlalink
fork keeps that base off RAMSECTION labels.

**When bank $00 is full, code goes to the next banks by itself.** A C
function and a library routine are `SUPERFREE` sections reached by `jsl`,
with function pointers that carry their bank: the linker places in bank
$01 and up what bank $00 cannot hold, and nothing in the code is tied to
its bank (a `switch` is a chain of compares, const tables are read far,
plain RAM is read through the data bank). The link report says how much
code is there; `BANK0_FAIL_THRESHOLD` (0, never, by default) makes the
build fail under a number of free bytes, for a project that wants its code
bank watched. What cannot move
is what is pinned (`BANK 0`): the startup code, the interrupt handlers, a
hand-written section that says so — if those do not fit, the linker fails.

To keep bank $00 for what must be there:
- Declare `.incbin` / `.db` payload with `ASSET_SECTION` (`templates/assets.inc`)
  instead of a bank-$00 `.SECTION` — it takes the same asset banks the
  compiler uses.
- Payload handed to the lib needs no bank of its own: every `dmaCopy*`
  helper and `dmaTransfer()` read the bank from the far pointer; a bank
  computed at runtime is a pointer built from it,
  `(const u8 *)((u32)bank << 16 | addr)`.
- Read `opensnes-rom budget game.sfc`: it lists what fills each bank.

### 🟢 Vertical scroll is off by one: the PPU never outputs scanline 0 (hidden by the lib)
The OBJ data of every scanline is fetched during the previous line, so the
PPU renders scanline 0 but never outputs it: a raw `BGnVOFS` of 0 shows
tilemap lines 1-224, and the bottom picture line is the first line of the
29th tile row. A program that fills 28 rows (a screen) leaves that line to
whatever VRAM held at power-on — a strip of garbage on real hardware,
invisible on an emulator that zero-fills VRAM. "Many games set their
vertical scroll values to -1 rather than 0" (anomie-regs, BG Scrolling).

**Mitigation (lib):** every vertical scroll the lib writes
is `y - 1` — the NMI shadow sync (`bgSetScroll`/`bgSetScrollY`), the map
module, `mode7SetScroll`, and the reset default — so `y = 0` means "tilemap
row 0 on the first picture line". **Test:** the lib fixture reads
`bgs.1.v_scroll == 76` after `bgSetScrollY(1, 77)` on luna's PPU view
(`testing/fixtures/libtests/test_libtest.py`), and the `backgrounds_mode6*.toml`
manifests pin the one exception (BG3 as the offset-per-tile table, written
raw). PVSnesLib writes the raw value; when
porting, do not subtract 1 yourself. The one path the lib cannot cover is
data you hand to the hardware directly, such as an HDMA table on
`BGnVOFS`: apply the -1 in the table.

**Except BG3 in Modes 2, 4 and 6.** There BG3 is not
displayed: it is the offset-per-tile table, and BG3VOFS selects which of its
rows the PPU reads (row VOFS / 8 for the horizontal offsets, the next for the
vertical ones; the screen line plays no part — snesdev-wiki,
*Offset-per-tile*). `setMode` records the mode, and in those three the NMI
writes BG3's VOFS raw, so `bgSetScroll(2, 0, 0)` reads rows 0 and 1.

### 🟡 A CGRAM write during the picture lands on the wrong entry
CGRAM is reachable from the CPU only during V-blank, H-blank or forced
blank; a write to CGDATA while the PPU is drawing "will write the data to
the wrong CGRAM address" (snesdev-wiki, PPU registers / CGDATA; luna
models it). This is the trap of
every per-scanline palette stream: an H-timer IRQ fires at H = HTIME + 3.5
dots (fullsnes), the handler's prologue then costs dots, and the DMA must
start after H = 274 (hblank flag) and finish before dot 22 of the next
line. In `examples/color/hicolor_1792`, whose handler saves registers and
latches the V counter, the clean window is HTIME 80..175 (measured on
luna); it runs at 128.

**Mitigation:** put the CGRAM DMA in H-blank and measure the window on
luna after any change to the handler — a lenient emulator does not show
the corruption. `dmaCopyCGram()` / `setPalette*()` from
the main loop are safe: they run in VBlank or forced blank.

### 🟡 BG1 scroll and Mode 7 matrix share one write-twice latch
`$210D`/`$210E` are dual registers (BG1 scroll in modes 0-6, Mode 7 scroll
in mode 7), and together with the Mode 7 registers `$211B`-`$2120` they go
through a **single shared write-twice latch**. If an HDMA channel or an IRQ
handler writes a BG1 scroll register between the two writes of a Mode 7
register, the 16-bit value — and the Mode 7 multiplier result MPY
(`$2134`-`$2136`) — is silently corrupted
([snesdev-wiki Errata](https://snes.nesdev.org/wiki/Errata)).

The lib itself never reads MPY (it uses the CPU multiplier at `$4202`), so
lib code is safe. The exposure is user code combining the `mode7` module
with HDMA on BG1 scroll — including the per-scanline M7A/M7D perspective
pattern from the Mode 7 tutorial.

**Mitigation:** keep Mode 7 matrix writes and MPY reads out of any frame
region where HDMA drives BG1 scroll — do matrix updates during VBlank, or
disable the scroll HDMA channel while rewriting the matrix. See the gotchas
in `docs/tutorials/mode7.md` and `docs/tutorials/hdma.md`.

### 🟡 Plain C RAM lives below $2000; `FAR` is the way above it
`sta.l $0000,x` and friends always access bank $00, so a plain C global
must sit in the 8 KB band `$00:0000-$1FFF` (shared with the stack and the
direct page). A hand-placed RAM section above $2000 reached through a
plain pointer reads bank $00 instead — silent corruption.

**Escape hatch:** declare the object `FAR`
(`FAR u8 buf[4096];`, header `snes/types.h`). It is placed in
`$7E:2000-$FFFF` (56 KB) and every access — direct, `buf[i]`, or through a
`u8 FAR *` — is compiled bank-honouring; initialisers work and the band is
zeroed at boot. The qualifier rides on the type like `const`: passing a
`FAR` pointer where a plain pointer is expected is a compile error, and
`T FAR *` converts to `const T *` freely, so the DMA/HDMA/asset API takes
`FAR` buffers as they are. Cost: `buf[i]` is cheaper than the bank-0 form,
a pointer walk is within 4 %. Tutorial: `docs/tutorials/far_ram.md`. The
one remaining silent path is an explicit cast that drops `FAR`.

**Mitigation for the plain band (checked at link time):**
the templates' memory maps reserve `$00:0000-$1FFF` for C RAM, and
`make/common.mk` runs `opensnes-rom check` after every link — the
RAM twin of the bank $00 ROM ratchet. A bank-$00 RAM section crossing or
placed past $2000 fails the build; free space below `RAM_FAIL_THRESHOLD`
(default 512 bytes) fails too, and below `RAM_WARN_THRESHOLD` (default
1024) prints the largest RAM sections as refactor candidates. Both bands'
free-byte counts print at every link, so approaching a ceiling is a
visible number, not a surprise. Set `SKIP_RAM_CHECK=1` to bypass for
debugging.

---

## Build-time / linker traps

### 🟢 The data-init terminator must close `.data_init` (checked after every link)
The data-init copy loop scans from `DataInitStart` until the 5-byte
terminator record that `data_init_end.o` appends to the `.data_init`
section (`APPENDTO`). A record landing past the terminator would leave its
initialised globals with whatever WRAM held at boot, silently.

wlalink sorts appended sections by priority, then by size, largest first
(`wlalink/analyze.c`, `_compare_sections`), and every record is at least
6 bytes (5 of header plus the data), so the 5-byte terminator sorts after
them whatever the object order. **Test:** every link runs
`opensnes-rom check`, which fails when the `DataInitEnd` label does not
close the `.data_init` section; a wlalink change to that sort would stop
there, not in a game.

### 🟠 cc65816 pushes function args **LEFT-TO-RIGHT**
PVSnesLib (tcc816) pushes args right-to-left, the C convention. cc65816 pushes
**left-to-right**. ASM functions ported from PVSnesLib without changing the stack
offsets read the wrong arguments — the first arg might be at offset 8,s instead
of 6,s. The function compiles, links, and corrupts the stack at runtime.

**Mitigation:** the full calling convention is documented in
[`compiler/ABI.md`](https://github.com/k0b3n4irb/opensnes/blob/develop/compiler/ABI.md), with worked examples and a port checklist.
When porting an ASM function from PVSnesLib, walk through the offsets explicitly.
Function pointers called from C follow the same convention.

### 🟢 `volatile` is honoured
Each load and store of a `volatile` object produces its own instruction, as
the C standard requires: none is forwarded, merged or removed. Use it for
MMIO polling, hardware-register reads, and any variable an interrupt
changes. The runtime itself uses plain globals for its NMI handshakes
(`vblank_flag`, `oam_update_flag`), at the same cost. **Test:** the
compiler suite's `test_volatiles.c` (three reads of `hw_status` give three
loads).

### 🟢 WLA-DX loses `.ACCU` / `.INDEX` tracking after branch merges (lint-enforced)
WLA-DX tracks the current accumulator/index register width across `rep`/`sep`
instructions, but merging branches resets that tracking. The next instruction
gets assembled at the wrong width — for example a `cpx #imm` becomes 2 bytes
instead of 3, and every subsequent address is off by one.

**Mitigation:** the SDK's lint (`devtools/lint_asm.py`) enforces an
explicit `.ACCU 8`/`.ACCU 16` and `.INDEX 8`/`.INDEX 16` after every
`rep`/`sep` and at every block boundary in hand-written ASM. The lint runs
on the SDK's own templates and library `.asm` files, which all carry the
markers. In your own assembly, write `.ACCU 8` / `.ACCU 16` (and `.INDEX`)
after every `rep` / `sep` and after every label a branch reaches.

---

## Toolchain & emulator coverage

### 🟡 `fixMul()` / `fixLerp()` are not safe inside nmiSet() callbacks

They use the hardware multiplier ($4202-$4217) plus shared WRAM
temporaries, and the unit is not reentrant: an NMI that lands between a
main-thread multiply's write to `$4203` and its read of `$4216` and
multiplies in the callback destroys the main thread's result. NMI is not
maskable, so no wait fixes that. Plain C `*` / `/` / `%` **are** callback-safe —
the compiler runtime detects the NMI context (`in_nmi_ctx`) and switches
to software paths. Symptom if ignored: silently wrong fixed-point values,
only when computed inside the callback. Mitigation: compute fixed-point
math in the main loop, or use plain integer operators in the callback.
(The headers carry the same `@warning`.)

### 🟢 SA-1 SIWP/CIWP write-protection polarity: bit = 1 enables writes
`templates/crt0.asm` enables SA-1 I-RAM access by writing `$FF` to the SIWP
register `$002229`. The
[Super Famicom Dev Wiki](https://wiki.superfamicom.org/sa-1-registers) says
bit = 1 *protects* a page: **that page is wrong**, and `$FF` (bit = 1 = write
enable) is correct:

- fullsnes ([SA-1 memory control](https://problemkaputt.de/fullsnes.htm#snescartsa1memorycontrol)):
  SIWP bits are write **enable** flags for eight 256-byte chunks
  (0=Protect, 1=Write Enable).
- The official Nintendo development manual (book 2, §4.1.25 SIWP):
  Setting 0 = write disable, 1 = write enable.
- nocash on real-hardware debugging
  ([nesdev p=237542](https://forums.nesdev.org/viewtopic.php?p=237542#p237542)):
  unlocking is done by *setting* the write-enable bits, not clearing them.
- The wiki page contradicts itself: its own `$2226`/`$2227` entries use the
  0=Protect / 1=Write-enabled convention.

**Test:** the SA-1 fixture `testing/fixtures/libtests_sa1_sram` reads back a
byte the SA-1 wrote to BW-RAM from its boot stub, and the crt0 self-test
stays as a runtime net: `sa1IsReady()` returns false unless the SA-1
reaches `$A5`.

### 🟡 SuperFX C support is intentionally absent
The GSU has its own RISC ISA with no C compiler. All SuperFX code must be
written in GSU assembly (built via the `wla-superfx` assembler). The C side
of a SuperFX project orchestrates GSU jobs and reads results — it doesn't
generate GSU code.

**Mitigation:** documented in `docs/tutorials/superfx.md` and the SuperFX example
under `examples/chips/superfx_*`. Plan accordingly: heavy compute lives in
GSU assembly, not in your C main.

### 🟡 The multitap (5-player) path is present but cannot be switched on
`templates/crt0.asm` carries a complete `ScanMPlay5` routine — it bit-bangs
pads 3 and 4 through `$4017` after flipping the multitap to its second
controller pair, and takes pad 2 from the auto-joypad result. The NMI handler
calls it whenever `snes_mplay5` is non-zero, and skips the mouse and Super
Scope while it is (the devices are mutually exclusive).

Nothing ever sets `snes_mplay5`. There is no detection routine, and `input.h`
exposes no function to enable it, so the flag stays 0 for the life of every
ROM and the routine never executes. Pads 3, 4 and 5 are unreachable. The
routine is not even linked unless something names it: the handler calls it
through the pointer `mplay5_reader`, which whoever sets `snes_mplay5` must
set to `ScanMPlay5` first.

**Mitigation:** none — write for two players. Closing this needs three things
together: a detection routine (the protocol is on the SNES Development Wiki),
a public API to arm it, and emulator support to test it — luna models
`pad`, `mouse` and `superscope` on each port, but not a multitap.

## Type & ABI gotchas

### 🟢 `int` is 16 bits, `long` 32, pointers 4 bytes
`sizeof(int) == 2`, `sizeof(long) == 4`, the native 16-bit word and a
32-bit long, and every `long` operator keeps its 32 bits (add and subtract
with carry, shifts across the halves, multiply and divide through the
runtime, compares high half first). `long long` is 8 bytes but has no
arithmetic: a `long long`, `float` or `double` computed at run time is
refused by the compiler; constants it folds are fine (`-2147483648`,
`(int)(1.5 * 256)`).

A pointer is 4 bytes: a 24-bit address (16 bits and a bank byte) and one
byte of padding. A function pointer carries its bank, so an indirect call
reaches a function in any bank, and every pointer the library takes is a
far pointer.

**Test:** `devtools/compiler-tests/cases/type_sizes.c` pins every size with
a `_Static_assert`; the `test_long_*` cases pin the `long` semantics, and
the ABI lint holds every hand-written assembly function to the 4-byte
pointer slot. `u8` / `u16` / `s16` / `u32` (`snes/types.h`) remain the
clearest way to say what a variable holds.

### 🟢 A C identifier is at most 79 characters (refused, never truncated)
QBE keeps names in a fixed buffer (`NString`, 80), so a global, a function or a
static whose IR name — the C name, plus `.file` for a file-scope static — is
longer than 79 characters is refused: `qbe:main.c (IR line N): identifier too
long: 79 characters at most, starting …`. Nothing is truncated or renamed
in silence.

### 🟢 A local read before any store is refused, with an IR-level message
QBE's `promote` (`mem.c`) refuses a stack slot that is loaded and never
stored: `qbe:main.c (IR line N): slot %.1 is read but never stored to`. In C
that is a local variable declared, never assigned, and read — `u16 k;
(void)k;`, or a value returned from a branch that never set it — whose
value is indeterminate; refusing it is right, but the message names an IR
slot, not the C variable. Assign the variable, or remove the read.

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

### 🟡 Struct parameters and struct returns by value are refused; a FAR struct is copied field by field
cc65816 has no lowering for a struct passed or returned by value (QBE
`parc` / `argc`). Whole-struct assignment (`a = b;`) works for objects in
bank $00 or in ROM read through a pointer; a copy from or to a `FAR` struct
or array is refused — "copy field by field". The build stops with
`cc65816/qbe: unhandled IR op N (parc) … struct parameters and struct
returns by value are not supported on w65816; pass a pointer` — it never
emits code for them.

**Mitigation:** pass `struct T *`, return through an out-pointer, and copy
a `FAR` struct field by field. Pinned by
`devtools/compiler-tests/cases/negative/`.

### 🟡 Variadic functions are refused
cproc parses `__builtin_va_start` / `__builtin_va_arg`, but the w65816
backend has no `vastart` / `vaarg` lowering, and the SDK ships no
`<stdarg.h>`. The build stops with `… (vastart) … variadic functions … are
not supported on w65816; pass an array and a count`.

**Mitigation:** an array plus a count, or a small struct of arguments by
pointer. `PHILOSOPHY.md` rules out `printf` in the core lib for the same
reason. Pinned by `cases/negative/varargs.c`.

### 🟡 Inline assembly is refused
cproc has no `asm` statement: `__asm__("nop")` stops the build with
`inline assembly is not yet supported`.

**Mitigation:** put the assembly in a `.asm` file (assembled by
`wla-65816`, listed in the example's `ASMSRC`) and call it through the C
ABI — `compiler/ABI.md` gives the stack layout, `lib/source/*.asm` the
pattern. Pinned by `cases/negative/inline_asm.c`.

## Where to add new entries

When you discover a new silent failure or hard-to-debug constraint:

1. Add an entry here under the right category, with a severity marker.
2. Note any active mitigation (link to the file/check that catches it).
3. Cross-reference from `CLAUDE.md` if the issue is relevant to AI-assisted
   work, but **always** keep the canonical entry in this file.

If the issue gets a permanent automated mitigation (build-time check, CI gate,
runtime guard), update the severity from 🔴/🟠/🟡 to 🟢 and describe the
mitigation alongside the problem. When a bug is fixed, its entry leaves this
page: the CHANGELOG records the fix.
