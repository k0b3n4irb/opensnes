# hdmaWave Library Bug — ARCHIVED

> **📁 ARCHIVED 2026-09-02 — cause documented, workaround permanent.**
> The buggy implementation this note describes (bank-$7E tables written
> through the WRAM data port $2180 with double buffering) no longer
> exists: `hdmaWaveH()` / `hdmaWaveUpdate()` were rewritten with tables
> in a bank-$00 RAMSECTION written by plain C (`lib/source/hdma.c`,
> "Tables live in bank $00 RAMSECTION" comment), and work. The hardware
> constraints the old investigation was groping toward are now
> documented by arbiter sources (snesdev-wiki Errata / sfc-dev-wiki
> register notes): general DMA cannot copy RAM→RAM through $2180, and
> $2181-$2183 are open bus on read. The TODO below (port the PVSnesLib
> Waves example, fix the old functions) is moot — kept for history.
> Cross-ref: `.claude/notes/chantiers/hardware_docs_audit.md`
> (retrospective section) and `.claude/rules/hardware_claims.md`.

## Bug (historical)
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
