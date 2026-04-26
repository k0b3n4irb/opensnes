# Debugging Methodology (Auto-loaded)

## Golden Rule: Fix the ROOT CAUSE, never the symptom

When a bug appears in an example, the problem is almost NEVER in the example code itself.
Think **globally** — the real cause is usually in one of these layers:

| Layer | Examples | How to check |
|-------|----------|-------------|
| **Compiler** | Wrong codegen, bad stack offsets, misaligned instructions | Compare generated ASM with expected; disassemble ROM bytes |
| **Library** | Wrong constant values, ASM/C mismatch, bad LUT tables | `grep` the constant in BOTH `.asm` and `.h` files |
| **Templates** | crt0.asm init order, memory map, RAMSECTION layout | Check `.sym` file for variable addresses |
| **Build system** | Wrong gfx4snes flags, missing modules, link order | Compare Makefile with PVSnesLib original |

**NEVER** add workarounds in example code to "make it work" — this hides the systemic bug
and breaks other examples or future code that hits the same path.

## Mandatory Debugging Steps

### 1. Reproduce with PVSnesLib original
Before any fix attempt, compare with the PVSnesLib original side-by-side in Mesen2.
Path: `$PVSNESLIB_HOME/snes-examples/` (set `PVSNESLIB_HOME` to your local PVSnesLib clone)

### 2. Identify the LAYER, not just the symptom
Ask: "Does this work in another example?" If yes → example-specific (Class C).
If no → library/compiler/template issue (Class A/B). Fix at the correct layer.

### 3. Verify assembled output — don't trust source alone
When source code looks correct but behavior is wrong:
```bash
# Find function address
grep 'functionName' example.sym

# Disassemble ROM bytes at that address
xxd -s <rom_offset> -l 64 example.sfc
```
Compare actual instruction bytes with expected. This catches:
- ASM/C constant mismatches (e.g., `$60` vs `3`)
- WLA-DX `.ACCU` tracking bugs (wrong operand sizes)
- Linker address resolution errors

### 4. Use Mesen2 debugger for runtime verification
- **VRAM breakpoints** (write) to verify DMA actually targets expected addresses
- **Watch panel** to monitor variable values (`[$B0]` for address $00B0)
- **Tile Viewer** to compare VRAM contents with PVSnesLib
- **Sprite Viewer** to check OAM entries (tile numbers, sizes, positions)

### 5. Cross-check ASM/C constant definitions
After ANY change to shared constants:
```bash
# Verify ASM and C definitions match
grep -n 'CONSTANT_NAME' lib/source/*.asm lib/include/snes/*.h
```
PVSnesLib uses **register values** (pre-shifted). OpenSNES C headers use **index values**.
ASM `.EQU` constants MUST match the C `#define` values, NOT PVSnesLib's convention.

## Anti-Patterns (NEVER do these)

| Anti-pattern | Why it's wrong | What to do instead |
|-------------|---------------|-------------------|
| Add `extern` + manual write in example to fix a library variable | Hides the real bug in the library ASM | Fix the library ASM |
| Change `BG_MODE` or VRAM addresses to work around corruption | The corruption has a root cause elsewhere | Find and fix the root cause |
| Add extra `WaitForVBlank` or `dmaCopyVram` to paper over timing | Timing issues indicate a systemic DMA/queue bug | Fix the DMA engine |
| Duplicate constants with different values in ASM vs C | Silent mismatch causes impossible-to-trace bugs | Single source of truth |
