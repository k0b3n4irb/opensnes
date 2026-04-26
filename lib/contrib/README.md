# lib/contrib — Non-core engine modules

Files in this directory are **not part of the OpenSNES core hardware library**.
They are higher-level engine code (game-entity systems, gameplay helpers) that
historically shipped alongside the SDK because PVSnesLib bundled them. The
audit (`~/opensnes_audit_2026-04-26.md` §3.1) flagged the size mismatch:
`object.asm` alone is 3 124 LOC out of `lib/source/`'s ~17 000 — keeping it
under the same roof as `dma.asm` or `sprite.asm` blurs the meaning of "core
SDK".

## Current contents

| File | Lines | Purpose | Used by |
|------|------:|---------|---------|
| `object.asm` | 3 124 | Game-object engine: linked-list management of up to 80 entities, type-based dispatch (init/update/refresh callbacks), workspace pattern for Bank-$7E objects, fixed-point physics, object-vs-object and object-vs-map collision, optional slope support. | `examples/games/mapandobjects/`, `examples/maps/slopemario/` |

## What "contrib" means here

- **Built like core modules.** `lib/Makefile` extends its ASM-pattern rules to
  scan `lib/contrib/`, so `LIB_MODULES := ... object` keeps working in example
  Makefiles. There is no separate build target — same toolchain, same memmap.
- **No API stability guarantee.** Public API (`lib/include/snes/object.h`)
  matches the upstream PVSnesLib engine for now, but the OpenSNES core team
  treats it as example-tier code. If you depend on it, vendor it (copy into
  your project).
- **Scope distinct from `lib/source/`.** `lib/source/` is reserved for code
  that wraps SNES hardware: DMA engine, PPU register helpers, OAM management,
  SPC700 driver, etc. Anything that imposes a *gameplay* model (entities,
  states, AI helpers) belongs in `contrib/` or in an example.

## Adding a new contrib module

1. Drop the `.asm` file into `lib/contrib/`.
2. If it needs C declarations, add the header under `lib/include/snes/<name>.h`
   so existing `<snes/...>` includes keep working.
3. **Do not** include it transitively from `lib/include/snes.h`. Examples that
   need it must `#include <snes/<name>.h>` explicitly and add `<name>` to
   their Makefile's `LIB_MODULES`.
4. Document in this README: file, line count, purpose, who uses it.

## Why not move it to `examples/engine/`?

The audit recommended `examples/engine/` for full physical separation. We
chose `lib/contrib/` instead because it preserves the existing build pipeline
(same memmap, same `LIB_MODULES :=` mechanism) and keeps the `<snes/object.h>`
include path stable for the two examples already using it. The signaling
benefit ("this is not core SDK code") is the same — the path makes that
explicit. A future move to `examples/engine/` (with its own Makefile and
`#include "engine/object.h"` paths) is possible if/when the engine grows
substantial enough to warrant it.
