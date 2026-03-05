# symmap.py — WLA-DX Symbol Map Analyzer

Parses `.sym` files produced by wlalink and checks for memory layout issues
that cause silent data corruption on real SNES hardware.

## When to use

- **Before every commit** — `validate_examples.sh` calls this automatically
- **Debugging garbled graphics or crashes** — check for WRAM overlap or bank $00 overflow
- **After modifying `crt0.asm` or RAMSECTION layout** — verify no collisions

## Dependencies

None (Python stdlib only).

## Usage

```bash
# Check WRAM mirror overlaps (main use case)
python3 devtools/symmap/symmap.py --check-overlap game.sym

# Check for C-generated data spilling out of bank $00
python3 devtools/symmap/symmap.py --check-bank0-overflow game.sym

# Show full memory layout
python3 devtools/symmap/symmap.py -v game.sym

# Find a symbol by name (fuzzy match)
python3 devtools/symmap/symmap.py --find monster_x game.sym

# Export memory map to JSON
python3 devtools/symmap/symmap.py --export-json game.sym
```

## Exit codes

| Code | Meaning |
|------|---------|
| 0 | No issues found |
| 1 | Collision or overflow detected (CI should fail) |
| 2 | Below warning threshold (informational) |

## What it checks

### WRAM mirror overlap (`--check-overlap`)

Bank $00 addresses `$0000-$1FFF` mirror Bank $7E `$0000-$1FFF`.
If the linker places variables in both ranges, they silently overwrite each other.

### Bank $00 ROM overflow (`--check-bank0-overflow`)

The compiler generates `lda.l $0000,x` which always reads bank $00.
If bank $00 ROM fills up, `static const` data spills to bank $01+ and reads
return garbage. Use `--warn-threshold N` to set the free-space warning level
(default: 2048 bytes).
