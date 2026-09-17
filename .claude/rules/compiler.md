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
.c → cc -E → cproc → QBE IR → qbe w65816 → .asm → wla-65816 → .obj
```

The qbe w65816 backend emits WLA-DX syntax directly (`.db`, `.dw`, `.SECTION`)
— the historical sed post-transform is gone.

## Build

```bash
make compiler       # Build all three submodules
```

This is a Class A change — requires `make clean && make` + full test suite (luna) on ALL examples.

## Critical Constraints

- **Bank $00 ROM is code only** (since #127.3, v0.41.0): C const data is placed in the asset banks and every C read of it is a far read; `devtools/check_bank_reads.py` fails the link on a bank-blind read. The bank $00 free-space ratchet still guards the code bank.
- **Plain C RAM lives below $2000; `FAR` is the way above it** (since chantier B2): `sta.l $0000,x` reads bank $00, so a plain global sits in `$00:0000-$1FFF`; `FAR` objects go to `$7E:2000-$FFFF` with bank-honouring codegen.
- **Refused, never miscompiled**: variadic functions, struct parameters / returns / assignment by value, inline assembly. cc65816 stops with a message naming the feature; `devtools/compiler-tests/cases/negative/` pins the refusal, `KNOWN_LIMITATIONS.md` (Type & ABI gotchas) gives the workaround.
- **LEFT-TO-RIGHT argument push**: cc65816 pushes function args left-to-right, NOT right-to-left like tcc816/PVSnesLib.
- **`volatile` is honoured** (since chantier A2, 2026-05-09): cproc tags volatile loads/stores with a `volat` IR keyword that QBE's loadopt/promote/gcm passes respect. The SDK still favours plain globals for NMI handshake patterns for cycle-cost equivalence, but user code can use `volatile` for MMIO without silent coalescing.
- **`unsigned int` = 2 bytes, `unsigned long` = 4 bytes** on this target (since chantier A1, 2026-05-08).

## After Any Compiler Change

1. `make clean && make` — full rebuild (stale objects cause phantom bugs)
2. `make tests` — luna coverage + visual regression + probes
3. Present the triaged representative examples to the user (luna visual
   pass is the reference; interactive spot-check via `luna mcp` / luna GUI
   if needed — see docs/tutorials/debugging.md)
4. Check for regressions in code generation with compiler test patterns
