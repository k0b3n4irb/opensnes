---
name: PVSnesLib example porting workflow + critical rules
description: Hard-to-derive workflow for porting PVSnesLib examples to OpenSNES. Local PVSnesLib clone is at /home/kobenairb/workspace/pvsneslib. Bare-metal register writes preferred for HDMA / window / colormath effects. Watch for BG mode mismatch (Mode 1 4bpp vs Mode 0 2bpp).
type: project
originSessionId: 7cf8aa6f-20b3-4e2f-81b4-25748e91b08f
---
When porting a PVSnesLib example to OpenSNES, the workflow below avoids
the most common failure modes (visual divergence with the original,
bank-byte mismatches, colormath inversion).

**Why**: PVSnesLib is the upstream reference for visual fidelity. Any
divergence in a port either reveals a real lib bug in OpenSNES or means
the asset pipeline / register sequence diverged. The historical pattern
is "I wrote a fresh asset" → port looks different from PVS → spend hours
debugging the lib when the real fix is "copy the original asset bytes".

**Local PVSnesLib clone**: `/home/kobenairb/workspace/pvsneslib`. Examples
under `snes-examples/`. Set `PVSNESLIB_HOME` env var if a script wants
it.

## Mandatory order

1. **Always copy source assets first** — `.png`, `.bmp` from
   `$PVSNESLIB_HOME/snes-examples/<example>/`. Don't recreate from a
   web fetch; the local clone is the canonical reference.
2. **Compare binary assets EARLY when visuals don't match** — diff the
   `.pic`, `.pal`, `.map` files between OpenSNES and PVSnesLib builds.
   `cmp -l a.pic b.pic | head` reveals byte-level divergence in
   seconds.
3. **Check BG mode before porting** — PVSnesLib examples mix Mode 1
   (4bpp BG1/BG2) and Mode 0 (2bpp). The two are not interchangeable —
   palette layout, tile size, and CGRAM offsets all differ.

## API / codegen rules learned the hard way

- **Bare-metal register writes preferred for HDMA / window / colormath
  effects.** The library wrappers (`windowEnable`,
  `colorMathSetSource`, etc.) compile to large frame-allocating code
  (`framesize=70` for `windowEnable`). Direct `REG_W12SEL` /
  `REG_TMW` / `REG_WOBJSEL` / `REG_CGWSEL` writes match PVSnesLib
  exactly and bypass the framesize cliff. Use the library wrappers for
  one-shot init, bare-metal for per-frame effects.
- **Non-const HDMA tables to avoid bank-byte mismatch in
  `hdmaSetup`.** Today's `hdmaSetup` hardcodes bank `$00` for ROM
  tables (catalogue B4 🟠). SUPERFREE'd const data may land in bank
  `$01+` → HDMA reads wrong bank → silent corruption. Either declare
  HDMA tables as non-const RAM (always bank `$00`) OR use
  `hdmaSetupBank()` with explicit bank. Affected examples to watch:
  window, parallax_scrolling, gradient_colors.

## What this rule does NOT cover

- New examples written from scratch (no PVSnesLib counterpart). The
  rule about "copy the original first" doesn't apply.
- Audio (SNESMOD `.it` modules behave identically across the two SDKs
  via the same `smconv` toolchain — copy the `.it` file, set
  `USE_SNESMOD := 1` and `SOUNDBANK_SRC := …`).
- 32-bit integer arithmetic (`s32`, `u32`) — broken today (chantier A7
  open). Avoid in ported examples until A7 ships, OR validate manually
  via the runtime fixture once Phase 1 lands.

## Cross-references

- `memory/gfx4snes_quantization.md` — the 2bpp pre-processing pattern
  is part of "always copy original first" applied to the asset
  pipeline.
- `KNOWN_LIMITATIONS.md` — bank-byte HDMA bug, framesize cliff, push
  order inversion vs PVSnesLib (`compiler/ABI.md` for the worked port
  checklist).
