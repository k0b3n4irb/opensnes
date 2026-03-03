# check_mvn.py — MVN/MVP Operand Linter

Scans `.asm` source files for raw `MVN`/`MVP` (block move) instructions
outside of known safe macros. Flags suspicious patterns where bank operands
may be swapped.

## When to use

- **After modifying library assembly** — `validate_examples.sh` runs this automatically
- **Porting PVSnesLib assembly** — catch swapped MVN/MVP operands early

## Dependencies

None (Python stdlib only).

## Usage

```bash
# Lint all library assembly files
python3 devtools/check_mvn/check_mvn.py lib/source/*.asm

# Strict mode: exit with error on any raw MVN outside macros
python3 devtools/check_mvn/check_mvn.py --strict lib/source/*.asm

# Verbose: show all MVN/MVP occurrences
python3 devtools/check_mvn/check_mvn.py -v lib/source/*.asm
```

## Exit codes

| Code | Meaning |
|------|---------|
| 0 | No issues (or warnings only in non-strict mode) |
| 1 | Errors found (raw MVN outside macros in strict mode) |

## Background

The 65816 `MVN src,dst` and `MVP src,dst` instructions take bank bytes as
operands. The source/destination order is unintuitive and varies between
assembler syntaxes, making it easy to accidentally swap them when writing
or porting assembly code.
