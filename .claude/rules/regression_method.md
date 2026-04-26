# Regression Debugging Method (Auto-loaded)

IMPORTANT: When a bug is introduced, use bisection — do NOT guess.

## Mandatory Bisection Workflow

1. **Identify the last known good state** — which commit or version last worked?
2. **Binary search with git bisect**:
   ```bash
   git bisect start
   git bisect bad              # current commit is broken
   git bisect good <commit>    # last known working commit
   # At each step: make clean && make && test the failing example
   git bisect good/bad
   git bisect reset            # when done
   ```
3. **Isolate the minimal change** — once the offending commit is found, read the diff
4. **Verify with a targeted test** — reproduce the exact failure before attempting a fix

## Diagnostic Tools

| Tool | Command | Use case |
|------|---------|----------|
| Symbol overlap check | `python devtools/symmap/symmap.py --check-overlap <rom>.sym` | Bank $00 overflow detection |
| ROM hex dump | `xxd <rom>.sfc \| head -100` | Verify ROM header, vectors |
| Visual regression | `cd tools/opensnes-emu && node test/run-all-tests.mjs --quick` | Automated screenshot comparison |
| Mesen2 debugger | Open ROM in Mesen2 → Debug → Breakpoints | Step-through debugging |

## Common Regression Patterns

| Symptom | Likely cause | Diagnostic |
|---------|-------------|------------|
| Black screen | Missing `consoleInit()` or `setScreenOn()` | Check init order in main.c |
| Garbage tiles | VRAM write outside VBlank | Check DMA timing, add `WaitForVBlank()` |
| Wrong colors | Palette at wrong CGRAM offset | Sprites start at CGRAM 128, not 0 |
| Crash/freeze | Bank $00 overflow | Run symmap.py --check-overlap |
| One example fails, others OK | Likely a data change | Diff the example's assets and code |
| ALL examples fail | Compiler or runtime change | Class A change — bisect in compiler/ or templates/ |

## Rules

- NEVER commit a fix without first identifying the root cause via bisection
- NEVER apply a workaround without documenting why the real fix isn't possible yet
- When in doubt, `make clean && make` first — stale objects cause phantom bugs
