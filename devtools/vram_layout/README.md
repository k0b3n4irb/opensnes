# VRAM layout — declarative spec → generated bases → build-gated

Replaces hand-typed VRAM magic numbers with a tiny per-example **`vram.spec`**
(asset manifest) from which a CP-SAT solver generates a **`vram_map.h`** of base
addresses that satisfy the SNES hardware alignment rules, never overlap, fit in
64 KB, and minimise the VRAM high-water mark. The committed header is what the
build consumes — **ortools is not a build dependency** — and a pure-stdlib gate
re-validates it on every CI run.

## Workflow

```
vram.spec  ──(ortools, opt-in)──>  vram_map.h  ──#include──>  main.c
   │                                    │
   └──────────── make lint-vram ────────┘   (stdlib: alignment + no-overlap + fit)
```

1. Declare regions in `examples/<path>/vram.spec`:
   ```
   # name      kind     size_words
   bg_tiles    bg_char  0x10
   bg_tilemap  bg_map   0x400
   obj_tiles   obj      0x20
   ```
   `kind` → hardware alignment, read from the SDK source so the model can't drift:
   `bg_char` 0x1000 (background.c BG12NBA), `bg_map` 0x400 (BGnSC),
   `obj` 0x2000 (sprite.h OBJ_BASE).

2. Generate (needs `ortools`, run by a dev, output committed):
   ```sh
   python3 devtools/vram_layout/vram_layout.py --emit examples/<path>
   #   --keep-low <region>   pin a region at 0x0000 (e.g. an OBJSEL base the code hardcodes)
   ```

3. `main.c` does `#include "vram_map.h"` and uses `VRAM_<NAME>` for every base
   and base-relative offset — no magic numbers.

4. Build gate (`make lint-vram`, pure stdlib, in CI): re-validates each committed
   `vram_map.h` against its `vram.spec` (aligned, non-overlapping, fits) so a
   stale or hand-edited header can't ship a silently broken layout.

## In production (proven, identical fbhash under luna)

| example | result |
|---|---|
| `examples/basics/collision_demo` | 3 regions; solver compacted high-water `0x4020 → 0x1010` (**~24 KB reclaimed**); visual regression identical. |
| `examples/graphics/sprites/metasprite` | obj sheet (2 name pages, `--keep-low`) + font + tilemap; high-water `0x6C00 → 0x2C00` (**~16 KB reclaimed**); visual regression identical. |

Functional equivalence is guaranteed by the project's own gate: VRAM addresses are
internal, so a valid re-placement renders pixel-for-pixel — `luna_runner.py
--compare` proves it against the committed baseline.

## Scope / limits

- **Top-level regions only.** Sub-sheet packing inside an `obj` region (e.g.
  metasprite's hero32/16/8) stays manual — the example lays those out at fixed
  offsets from the generated base. Modeling sub-sheets is a possible extension.
- **ortools = generation only.** The solver is pinned single-threaded with a
  fixed seed → reproducible output (same spec ⇒ same header). The build/CI never
  needs it.
- **Sibling target: bank-$00 packing** — same technique on `.const`→bank
  assignment, but gated on chantier A6 (until the compiler emits 24-bit far
  reads, C data can only live in bank $00).

## Files

- `vram_spec.py` — stdlib: spec/header parse, `validate()`, `emit_header()`.
- `vram_layout.py` — ortools: CP-SAT `solve()` + `--emit`.
- `../check_vram_layout.py` — the build gate (alignment scan + spec validation).
