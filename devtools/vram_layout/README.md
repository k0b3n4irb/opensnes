# VRAM layout solver — PILOT (OR-Tools CP-SAT)

A proof-of-concept that constraint solving can auto-place an example's VRAM
regions (BG tilesets, tilemaps, sprite tiles) — satisfying the SNES hardware
alignment rules, never overlapping, fitting in 64 KB — and **validate** an
existing layout (a linter for the #1 silent VRAM bug: tile/map/sprite overlap the
PPU ignores with no error).

## Status: prototype / opt-in. NOT a build dependency.

Requires `ortools` (`pip install --user ortools`). The build stays stdlib-only —
this is a tool you run to (re)generate or check a layout, then commit the result.
The solver is pinned single-threaded with a fixed seed, so its output is
**reproducible** (mandatory for this project's deterministic builds).

```sh
python3 devtools/vram_layout/vram_layout.py
```

## What the pilot proved (on examples/basics/collision_demo)

The alignment granularities are read straight from the SDK source, so the model
can't drift from what the lib programs:

| region kind | align (words) | source |
|---|---|---|
| `bg_char` (tiles)   | `0x1000` | `background.c` bgSetGfxPtr → BG12NBA `vramAddr>>12` |
| `bg_map` (tilemap)  | `0x400`  | `background.c` bgSetMapPtr → BGnSC `(vramAddr>>8)&0xFC` |
| `obj` (sprite tiles)| `0x2000` | `sprite.h` OBJ_BASE `(vram_addr>>13)&0x07` |

Results:

1. **Linter** — validated collision_demo's hand-coded layout (bg_tiles `0x0000`,
   tilemap `0x0400`, sprites `0x4000`): VALID.
2. **Auto-placement** — the solver re-derived a valid layout from sizes+types
   alone and compacted it: sprites `0x0000`, tilemap `0x0400`, bg_tiles `0x1000`
   → high-water `0x4020` → `0x1010` words, **reclaiming ~24 KB of VRAM**.
3. **Functional equivalence (the decisive test)** — applying the auto layout to
   collision_demo (base pointers + DMA upload addresses updated consistently),
   rebuilding, and running luna's visual regression gave an **identical fbhash**
   (`COMPARE: 1/1 ok`). The auto-computed layout renders pixel-for-pixel like the
   hand-tuned one. (The example was reverted afterwards — this is a pilot.)

## Verdict / where this goes

Constraint solving is a genuine fit for the SDK's *placement/packing* problems,
not codegen. The natural next targets, in order:

- **VRAM layout** (this) — generate a per-example `vram_map.inc` of base
  addresses from a declared asset list; the visual probe already proves
  equivalence.
- **Bank-$00 packing** (the high-value sibling) — assign `.const` sections to ROM
  banks under the 32 KB-per-bank limit. **Gated on chantier A6**: until the
  compiler emits 24-bit far reads, C data can only live in bank $00, so a packer
  can optimise *within* bank $00 but not spread across banks. After A6 it becomes
  an optimal multi-bank allocator.

See the design discussion in the chantier notes for the full rationale.
