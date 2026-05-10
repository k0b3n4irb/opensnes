# hdmaWave Library Bug — TODO

## Bug
`hdmaWaveH()` / `hdmaWaveUpdate()` in `lib/source/hdma.c` produce corrupted output:
- Double-buffering via WRAM data port ($2180-$2183) causes massive visual corruption
- Image flickers between normal and heavily distorted frames
- Likely timing issue: table swap during active HDMA read, or WRAM port addressing bug

## Current State
- `examples/graphics/effects/hdma_wave/` uses self-contained version (pre-computed tables, works)
- PVS port (`pvsneslib_examples/graphics/effects/waves/`) used buggy lib functions (broken)
- Also had API mismatch: `hdmaWaveH(ch, bg=0, ...)` but lib expects bg=1 for BG1

## TODO
1. Fix `hdmaWaveH()` / `hdmaWaveUpdate()` / `fillWaveTable()` in `lib/source/hdma.c`
2. Port the REAL PVSnesLib Waves example from `/home/kobenairb/workspace/pvsneslib/snes-examples/graphics/Effects/Waves`
3. Replace `examples/graphics/effects/hdma_wave/` with the working PVS port

## Root Cause Investigation
- `fillWaveTable()` uses WRAM port to write to bank $7E tables
- `hdmaSetTable()` swaps the HDMA source pointer each frame
- Possible issues: writing to wrong address, swap timing vs HDMA read, table format mismatch
- Compare with PVSnesLib's `setModeHdmaWaves()` implementation for reference
