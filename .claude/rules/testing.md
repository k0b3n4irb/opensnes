# Testing Workflow (Auto-loaded)

IMPORTANT: Every change must pass this workflow before commit.

## Test Command

```bash
cd tools/opensnes-emu && node test/run-all-tests.mjs --quick
```

This runs 7 phases (212 checks total):
1. **Preconditions** — toolchain and dependencies present
2. **Compiler tests** — 60 C→ASM pattern checks
3. **Build** — all 52 examples compile cleanly
4. **Static analysis** — ROM headers, memory maps, symbol overlaps
5. **Runtime execution** — emulated run of each ROM
6. **Visual regression** — screenshot comparison against baselines
7. **Lag detection** — frame timing verification

## Change Classification

| Class | What changed | Required validation |
|-------|-------------|-------------------|
| **A** | Compiler (cproc/qbe/wla-dx) or runtime (crt0, runtime.asm) | `make clean && make` + full test suite + Mesen2 on ALL affected examples |
| **B** | Library module (lib/source/) | `make lib` + test suite + Mesen2 on examples using that module |
| **C** | Single example or new example | Build that example + test suite + Mesen2 on that example |
| **D** | Docs, Makefile, tools only | Test suite only (no Mesen2 needed) |

## 3-Pillar Validation

Every non-trivial change requires all 3 pillars:

1. **opensnes-emu** (automated) — `node test/run-all-tests.mjs --quick`
2. **Mesen2** (manual) — ask the user to validate visually in Mesen2 emulator
3. **Full rebuild** — `make clean && make` must succeed with zero warnings

## Before Commit Checklist

1. `make clean && make` — zero warnings
2. `cd tools/opensnes-emu && node test/run-all-tests.mjs --quick`
3. **MANDATORY: List ALL impacted examples by path** — determine which examples use
   the changed modules/files, list them explicitly, and ask the user to validate
   each one in Mesen2. Do NOT commit until the user confirms every impacted example.
4. **For library changes (Class B)**: grep all example Makefiles for the changed
   module name in LIB_MODULES to find impacted examples. List them ALL.
5. **NEVER commit without user validation.** Do not assume examples work because
   they compiled. Visual validation in Mesen2 is required.
6. Conventional Commits format in message

## Debugging Ported Examples

When a ported example has visual bugs:
1. **Compare with the PVSnesLib original** — build the original PVSnesLib example
   and compare the generated ASM output (stack offsets, VRAM layout, register values)
2. **Check VRAM layout** — sprite tiles, font tiles, and tilemaps must not overlap
3. **Check OBJSEL** — Name Base and Name Select gap determine where tiles 256+ are read
4. **Never guess** — use Mesen2 debugger (VRAM viewer, OAM viewer) to verify actual state
