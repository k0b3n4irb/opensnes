# cc65816 ABI Reference

Calling convention, stack layout, and register usage for the OpenSNES
toolchain (cproc + QBE + WLA-DX targeting the WDC 65816).

This file is the **canonical, public** reference. Everything in here is
empirically verified against the generated assembly — examples are real
output from `bin/cc65816 input.c -o output.asm`. If you find a
discrepancy with `templates/crt0.asm` or `compiler/qbe/w65816/emit.c`,
trust the source files and update this doc.

---

## Quick reference

| Topic | Convention |
|-------|------------|
| **Argument push order** | LEFT-TO-RIGHT (rightmost arg ends closest to SP) |
| **First argument lives at** | `framesize + 4` from SP, in callee's frame |
| **Return value (≤ 16-bit)** | `A` register (16-bit) |
| **Return value (> 16-bit)** | passed via stack (caller-provided slot) |
| **Caller-saved registers** | A, X, Y, P, all `tcc__r*` direct-page slots |
| **Callee-saved registers** | none (cc65816 ABI saves nothing) |
| **Frame discipline** | callee adjusts SP by `framesize`; caller cleans up args via `plx` |
| **Stack growth** | down (pushing decreases SP) |

---

## Argument passing

**Important:** cc65816's push order **differs from tcc816 (PVSnesLib)** and from
classical C cdecl. PVSnesLib pushes right-to-left (standard cdecl); cc65816
pushes left-to-right. The stack layout the callee sees is therefore
**reversed**: ASM functions ported from PVSnesLib without flipping the
argument offsets read the wrong values.

### Caller side

For `f(a, b, c)`, cc65816 emits:

```asm
    pea.w a       ; push 1st (leftmost) arg first
    pea.w b       ; push 2nd
    pea.w c       ; push 3rd (rightmost) arg last → closest to SP
    jsl f
    plx           ; pop 3 args (6 bytes) using 3 plx
    plx
    plx
```

Each argument occupies 2 bytes on the stack regardless of declared C type:
`u8`, `u16`, and pointers are all 2-byte slots (a `u8` is zero-extended to
16 bits before pushing). `u32` and `s32` use 4 bytes (one 16-bit pea each).

### Callee side

After the callee's prologue (which subtracts `framesize` from SP for the
local frame), the stack looks like:

```
                          ┌────────────────────────────┐
SP + 1   .. SP + F        │  local frame (F bytes)     │
SP + F + 1               │  PCL  ┐                    │
SP + F + 2               │  PCH  │ JSL return (3 B)   │
SP + F + 3               │  PBR  ┘                    │
SP + F + 4               │  rightmost C arg (low)     │  ← "first IR slot"
SP + F + 5               │  rightmost C arg (high)    │
SP + F + 6               │  next C arg (low)          │
...                       │   ...                      │
SP + F + 4 + 2·(N-1)      │  leftmost C arg (low)      │  ← "last IR slot"
                          └────────────────────────────┘
```

The formula used by the QBE backend is `offset = framesize + 2 + (-slot)`
where `slot ∈ {-2, -4, -6, ...}` numbers the parameters in
declaration-order's reverse (rightmost = slot −2).

### Worked example

Source:

```c
unsigned short sub(unsigned short a, unsigned short b) { return a - b; }
```

Generated (frameless leaf, `F = 0`):

```asm
sub:
    rep #$20
    lda 6,s     ; load a (leftmost — at SP+F+6)
    sec
    sbc 4,s     ; subtract b (rightmost — at SP+F+4)
    rtl         ; return value in A
```

Three-argument case (`a - b - c`):

```c
unsigned short three(unsigned short a, unsigned short b, unsigned short c)
    { return a - b - c; }
```

```asm
three:
    rep #$20
    lda 8,s     ; a (leftmost, highest offset)
    sec
    sbc 6,s     ; b (middle)
    sec
    sbc 4,s     ; c (rightmost, lowest offset)
    rtl
```

### Porting from PVSnesLib

PVSnesLib's tcc816 pushes right-to-left, so its callee sees the **first** C
argument at `F+4` and the **last** at `F+4+2·(N-1)`. cc65816 reverses this.

Mechanical port checklist for an ASM function ported from PVSnesLib:

1. List the C arguments in declaration order: `a, b, c, ...`.
2. PVSnesLib offsets: `F+4`=a, `F+6`=b, `F+8`=c, ...
3. cc65816 offsets: `F+4`=last, `F+6`=second-to-last, ..., highest=a.
4. Swap every `lda N,s` / `sta N,s` / `cmp N,s` accordingly.

A function that compiled and ran "fine" until you fed it actual data is the
canonical symptom of having missed this swap.

---

## Return values

### Up to 16 bits (u8, u16, s8, s16, pointer)

Returned in `A`. The callee leaves `A` set; the caller reads it after `jsl`.

For functions that need to clean up a frame, the epilogue uses `tax / tsa /
adc.w #F / tas / txa` (save A in X, fix SP, restore A) — never `tsa` then
`txa` directly because `tsa` would overwrite A.

### 32-bit values (u32, s32, struct {u16, u16})

Caller reserves 4 stack bytes; callee writes the result there before
returning. Implementation detail: the runtime helpers `__mul32`, `__div32`
etc. read operands from `SP+5..SP+8` and write the 32-bit result to the
caller's reserved slot.

### Larger structs

Returned via a hidden first pointer argument. Treat them as if you wrote a
function returning `void` and passed `result*` as the first parameter — the
backend handles this transparently in C-to-C calls; if you call such a C
function from ASM, you must allocate the slot yourself and pass its address.

### void

Nothing to do. Callee restores SP and returns; A is undefined on exit.

---

## Direct page layout

`templates/crt0.asm` reserves the bottom of bank `$00` for compiler scratch
registers. Layout:

```
$00:0000-001F   tcc__r0 ..  tcc__r5h     (6 register pairs, low+high)
$00:0028-002B   tcc__r9, tcc__r9h        (kept separate from r0-r5)
$00:002C-002F   tcc__r10, tcc__r10h
$00:0030-007F   System variables (vblank_flag, oam_update_flag, frame_count, …)
$00:0080        nmi_callback pointer
$00:0100 page   tcc__nmi_registers       (DP-isolated copy for NMI callback)
$00:0200+       free for application use (above the SDK's reservation)
```

`tcc__r9` and `tcc__r10` are the **scratch registers** used for inline
multiply, function pointers, and similar idioms. Treat all `tcc__r*` slots
as **caller-saved**: any function call may clobber them.

The NMI handler runs with its direct page set to `tcc__nmi_registers` (a
page-aligned mirror of the layout above). This isolates NMI-side
DP-relative accesses from main-thread state — a significant cycle saving
because the handler doesn't have to push/pop the main DP. **If you call C
from your `nmiSet` callback, do not assume DP variables hold main-thread
values.**

---

## Calling C from assembly

Declare the C function as `extern` in the assembly side (or just by symbol
name). To call `extern void setColor(u8 idx, u16 color)`:

```asm
    pea.w idx_value      ; push 1st arg (u8 zero-extended to 16)
    pea.w color_value    ; push 2nd arg (last → at F+4 in callee)
    jsl setColor         ; long call — bank does not need to be $00
    plx                  ; pop 2 args (4 bytes)
    plx
```

If the C function returns a value, read it from `A` immediately after
`jsl`, before any instruction that would clobber it (notably `plx` does
**not** touch A; `pla` does).

---

## Calling assembly from C

Implement the function in `.asm` with a `.SECTION` so it lands in ROM, and
declare it in your header with the matching C prototype:

```c
/* In your_module.h */
extern unsigned short asm_helper(unsigned short a, unsigned short b);
```

```asm
.SECTION ".text.asm_helper" SUPERFREE
.ACCU 16
.INDEX 16
asm_helper:
    rep #$20
    .ACCU 16             ; explicit — WLA-DX loses tracking after merges
    lda 6,s              ; a (leftmost, higher offset)
    clc
    adc 4,s              ; b (rightmost, lower offset)
    rtl                  ; return in A
.ENDS
```

Constraints to respect:

- **Register width on entry**: A is 16-bit (`rep #$20`) and X/Y are 16-bit
  (`rep #$10`). The C caller has already issued `rep #$20` before the
  `jsl`. If your function manipulates 8-bit mode, restore 16-bit before
  `rtl` (the convention is "callee returns in 16-bit mode").
- **Direct page**: assume DP = 0. C code does not touch DP, so it stays
  where crt0 set it.
- **Data Bank Register (DBR)**: assume DBR = 0. Use `lda.l` for
  cross-bank loads.
- **All `tcc__r*` slots are clobberable**. The C caller treats them as
  scratch; your ASM can use any of them, just don't assume they hold
  values across your own internal calls.
- **`.ACCU 16`/`.INDEX 16` markers**: WLA-DX loses register-width
  tracking after branch merges. Add explicit markers after every
  `rep`/`sep` and at the top of every label that may be branched to.
  See `KNOWN_LIMITATIONS.md` (toolchain section).
- **No `volatile` in C declaration** of the helper: cproc accepts it but
  QBE's SSA pass crashes on volatile in loops. Use a plain global to
  signal hardware-poll completion.

---

## Worked example: `oamSet` (real SDK function)

`lib/include/snes/sprite.h` declares:

```c
void oamSet(u8 id, u16 x, u16 y, u16 attr, u8 size, u16 tile);
```

A C call:

```c
oamSet(0, 100, 50, 0x30, 0, 0x100);
```

Generates:

```asm
    pea.w 0          ; id (zero-extended u8)
    pea.w 100        ; x
    pea.w 50         ; y
    pea.w 48         ; attr (0x30)
    pea.w 0          ; size (zero-extended u8)
    pea.w 256        ; tile
    jsl oamSet
    plx              ; pop 6 args (12 bytes total)
    plx
    plx
    plx
    plx
    plx
```

Inside `oamSet`'s ASM (with framesize F):

| Offset | Param |
|--------|-------|
| `F+4`  | tile (rightmost arg, low byte) |
| `F+6`  | size (zero-extended u8 — only low byte meaningful) |
| `F+8`  | attr |
| `F+10` | y |
| `F+12` | x |
| `F+14` | id (zero-extended u8) |

For the assembly source, see `lib/source/sprite_oamset.asm`.

---

## Type sizes

| C type | Size (bytes) | Notes |
|--------|:------------:|-------|
| `u8` / `s8` / `bool` / `_Bool` | 1 (storage), 2 (stack slot) | Always zero/sign-extended when passed as argument. |
| `u16` / `s16` / pointer | 2 | Native word size. |
| `int` / `unsigned int` | **4** | Surprising on a 16-bit target. Use `s16`/`u16` instead. |
| `long` / `unsigned long` | **8** | Almost certainly not what you want. Avoid. |
| `u32` / `s32` | 4 | Stack-passed and stack-returned. |
| `float` / `double` | 4 / 8 | Software floats — extremely slow, used only by `printf`-style helpers. Avoid in game code. |

For all SDK code, use `u8`, `u16`, `s16`, `u32` from `lib/include/snes/types.h`.

---

## Frame layout (callee)

Most functions have `framesize > 0` (locals + spill slots). The prologue:

```asm
my_function:
    rep #$20             ; A is 16-bit
    tsa                  ; A = SP
    sec
    sbc.w #framesize     ; carve out frame
    tas                  ; SP = A
@start:
    ...
```

Leaf functions with no locals and aliased parameters can be **frameless**
(`framesize = 0`): no `sbc`/`tas`, no epilogue beyond `rtl`. This is one of
the optimisations documented in `~/.claude/.../memory/compiler_optimizations.md`
(Phase 3, Phase 5b).

Epilogue for non-void return:

```asm
    sta N,s              ; (sometimes — store result in local for now)
    tax                  ; preserve return value
    tsa
    clc
    adc.w #framesize
    tas                  ; restore SP
    txa                  ; restore A (return value)
    rtl
```

The `tax / txa` dance exists because `tsa` would clobber A. Don't shortcut
it unless you know A doesn't carry the return.

---

## What the backend does NOT support

- **Variadic functions** (`...`): no support. Wrappers like `printf` use
  fixed-prototype variants.
- **Nested function definitions** (GCC extension): not supported.
- **`alloca`** (variable-length stack allocation): not supported. Loops
  with `volatile` crash the SSA pass; use globals instead.
- **`__attribute__((interrupt))`**: not recognised. The NMI handler is
  hand-written assembly in `templates/crt0.asm` and dispatches to a C
  callback registered via `nmiSet()`.
- **Inline assembly** (`asm volatile (...)`): not supported. Write the
  routine in a separate `.asm` file and call it via the convention
  documented above.

---

## See also

- `templates/crt0.asm` — system-variable layout, NMI handler, data-init copy
- `compiler/qbe/w65816/emit.c` — the actual code generator
- `KNOWN_LIMITATIONS.md` — silent failures, including ABI traps that
  bite when porting from PVSnesLib
- `compiler/PINS.md` — pinned commits of cproc/qbe/wla-dx; this ABI
  reference applies to those specific versions
- `make/common.mk` — build pipeline (preprocess → cproc → qbe → wla-65816 → wlalink)
