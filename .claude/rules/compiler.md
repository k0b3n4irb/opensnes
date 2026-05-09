---
paths:
  - "compiler/**/*"
  - "bin/cc65816"
---

# Compiler Rules

## Architecture

Three git submodules:
- `cproc/` — C11 frontend, parses C into QBE IR
- `qbe/` — Code generator with custom 65816 backend, emits .asm
- `wla-dx/` — WLA-DX assembler (wla-65816) and linker (wlalink)

The `bin/cc65816` wrapper orchestrates the pipeline:
```
.c → cproc → QBE IR → qbe w65816 → .asm → sed transform → wla-65816 → .obj
```

The sed transform converts QBE output syntax to WLA-DX syntax (`.byte`→`.db`, `.word`→`.dw`, etc.).

## Build

```bash
make compiler       # Build all three submodules
```

This is a Class A change — requires `make clean && make` + full test suite + Mesen2 on ALL examples.

## Critical Constraints

- **Bank $00 is 32KB max**: `static const` arrays get SUPERFREE sections. If bank $00 fills, data spills to bank $01+ but C code reads from bank $00 → garbage.
- **`sta.l $0000,x` always reads bank $00**: all C RAM must be below $2000.
- **LEFT-TO-RIGHT argument push**: cc65816 pushes function args left-to-right, NOT right-to-left like tcc816/PVSnesLib.
- **`volatile` is honoured** (since chantier A2, 2026-05-09): cproc tags volatile loads/stores with a `volat` IR keyword that QBE's loadopt/promote/gcm passes respect. The SDK still favours plain globals for NMI handshake patterns for cycle-cost equivalence, but user code can use `volatile` for MMIO without silent coalescing.
- **`unsigned int` = 2 bytes, `unsigned long` = 4 bytes** on this target (since chantier A1, 2026-05-08).

## After Any Compiler Change

1. `make clean && make` — full rebuild (stale objects cause phantom bugs)
2. `cd tools/opensnes-emu && node test/run-all-tests.mjs --quick` — all 390+ checks
3. Ask user for Mesen2 validation on representative examples
4. Check for regressions in code generation with compiler test patterns
