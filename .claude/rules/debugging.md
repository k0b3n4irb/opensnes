# Debugging and regressions (always loaded)

## Fix the root cause, never the symptom
A bug seen in an example is almost never in the example. Name the layer before touching anything:

| Layer | Look at | How |
|---|---|---|
| Compiler | generated ASM, stack offsets | compare with what you expect; disassemble the ROM bytes |
| Library | constants, ASM/C mismatches, LUTs | `grep` the constant in BOTH `lib/source/*.asm` and `lib/include/snes/*.h` |
| Templates | crt0 init order, memory map, RAMSECTIONs | `.sym` addresses |
| Build | converter flags, missing modules, link order | the example's Makefile against `make/common.mk` |

Never add a workaround to an example to "make it work" (an `extern` plus a manual write, a changed BG mode or VRAM address, an extra `WaitForVBlank` or DMA to paper over a timing bug, a duplicated constant): it hides a systemic bug. "Does this work in another example?" Yes: example-specific. No: library, compiler or template, fixed there.

## A regression is bisected, never guessed
1. Last known good commit; `git bisect start` / `bad` / `good <commit>`; at each step `make clean && make` then the failing case; `git bisect reset`.
2. Read the diff of the commit found, reproduce the exact failure, then fix. Never commit a fix without the root cause; a workaround is documented with why the real fix is not possible yet.
3. All examples fail: compiler or runtime change (Class A), bisect in `compiler/` or `templates/`. One fails: look at its data. Stale objects cause phantom bugs: `make clean && make` first.

## Verify the assembled output, not the source
`grep functionName x.sym`, then `xxd -s <offset> -l 64 x.sfc`: catches ASM/C constant mismatches (`$60` vs `3`), WLA-DX `.ACCU` tracking, wrong operand sizes, linker address errors. PVSnesLib's constants are register values (pre-shifted); OpenSNES C headers use index values, and ASM `.EQU`s must match the C `#define`s.

## Tools
- luna (`docs/tutorials/debugging.md`): `run_until_mem_write` (who writes this buffer), `peek_memory` / `state`, `assets-dump` against the PVSnesLib original, `oam.json` / `ppu.oam_full`. A figure read over MCP depends on what the session did before; a number for a manifest or a baseline is taken again at the CLI from power-on.
- `python devtools/symmap/symmap.py --check-overlap x.sym` (bank $00 overflow), `python3 testing/luna_runner.py --compare` (framebuffer), `xxd x.sfc | head` (header, vectors).
- Black screen: init order (`consoleInit()`, `setScreenOn()`). Garbage tiles: VRAM write outside VBlank. Wrong colours: sprite palettes start at CGRAM 128. Crash: bank $00 overflow.
