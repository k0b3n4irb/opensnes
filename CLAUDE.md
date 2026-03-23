# CLAUDE.md

## Build Commands

```bash
make                    # Build everything: compiler → tools → library → examples
make compiler           # Build cc65816 (cproc+QBE) and WLA-DX assembler/linker
make tools              # Build gfx4snes, font2snes, smconv
make lib                # Build OpenSNES library (LoROM + HiROM)
make examples           # Build all 52 example ROMs
make clean              # Clean all build artifacts
make clean && make      # Full rebuild (REQUIRED after compiler or runtime changes)
```

Build a single example:
```bash
cd examples/text/hello_world && make
```

## Testing

All testing goes through opensnes-emu (snes9x WASM):
```bash
cd tools/opensnes-emu && node test/run-all-tests.mjs --quick
```

Runs 7 phases: preconditions, compiler tests (60 C→ASM pattern checks), build, static analysis, runtime execution, visual regression, lag detection. 212 checks must pass.

See `.claude/rules/testing.md` for the mandatory test workflow.

## Compilation Pipeline

```
main.c → cproc (C11 frontend) → QBE w65816 (codegen) → wla-65816 (assembler) → wlalink (linker) → game.sfc
```

The `bin/cc65816` wrapper orchestrates cproc→QBE→wla-65816. QBE emits 65816 assembly, sed-transformed (`.byte`→`.db`, `.word`→`.dw`) before assembly.

## Code Style

- C: 4 spaces, K&R braces, snake_case functions/vars, UPPER_CASE constants, PascalCase types
- Use fixed-width types: `u8`, `u16`, `s16`, `u32` (from `snes.h`). `unsigned int` = 4 bytes, `unsigned long` = 8 bytes on this target.
- ASM: labels at column 0, instructions indented with tab, `.section` for organization
- Commits: [Conventional Commits](https://www.conventionalcommits.org/) — `feat(scope):`, `fix(scope):`, `perf(scope):`, etc.
- Scopes: `lib`, `compiler`, `runtime`, `tools`, `examples`, `build`
- IMPORTANT: Do NOT add `Co-Authored-By` trailers for AI tools in commit messages.

## Critical Constraints

IMPORTANT: These are **silent failures** — no error messages, just wrong behavior.

- **VRAM writes only work during VBlank or forced blank** — the PPU silently ignores writes during active display
- **VBlank DMA budget**: ~4KB max per frame. Larger transfers need force blank (`setScreenOff/On`) or multi-frame splitting
- **Bank $00 overflow**: `static const` arrays get SUPERFREE sections. If bank $00 fills (32KB), data silently spills to bank $01+ but C code reads from bank $00 → garbage. Combine related const arrays.
- **`sta.l $0000,x` always reads bank $00** — all C RAM must be below $2000
- **cc65816 pushes args LEFT-TO-RIGHT** (not right-to-left like tcc816/PVSnesLib) — ASM functions ported from PVSnesLib have swapped stack offsets
- **`data_init_end.o` MUST be linked last** — it's the sentinel for the DMA copy loop
- **Linker object order**: `combined.obj` (crt0+runtime+ASM) → C objects → library objects → `data_init_end.o`
- **WRAM data port ($2180-$2183) is NOT safe in NMI** — silent corruption if NMI fires mid-sequence
- **`volatile` in loops crashes QBE** — use globals instead of `volatile` keyword
