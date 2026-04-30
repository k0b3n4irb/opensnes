# OpenSNES — Design Philosophy

This document captures the **principles** that guide design decisions in
OpenSNES. When a feature, API, or refactor is on the table, the question
is "does it respect these five principles?" If yes, ship it. If no, keep
talking.

It is intentionally short. Long manifestos die unread.

---

## Mission

OpenSNES is a **2D game engine for the Super Nintendo, written for game
developers who know C but don't want to learn 65816 assembly, PPU
register layouts, or NMI handler timing to ship a game.**

The hardware is small (3.58 MHz, 32 KB banks, 128 KB WRAM). The engine
respects that — every abstraction is opt-in and pays for itself in code
size.

## Positioning vs PVSnesLib

OpenSNES is forked from [PVSnesLib](https://github.com/alekmaul/pvsneslib)
and shares its lineage. The two projects sit at **different layers** of
the same stack and are complementary rather than adversarial:

| Concern | PVSnesLib | OpenSNES |
|---------|-----------|----------|
| Audience | SNES hardware enthusiasts who want a thin C layer over the metal | C developers who want to ship a SNES game without learning the metal first |
| Compiler | tcc816 (right-to-left push) | cproc + QBE (left-to-right push, modern C11) |
| NMI / VBlank | User-managed | Auto-orchestrated (auto-flush hook, scroll sync, OAM DMA) |
| Hardware quirks | Exposed (Y-position +1 scanline, etc.) | Hidden behind APIs, documented in `docs/hardware/` |
| Font / asset model | Bring-your-own (caller provides tile + palette pointers) | Battery-included (default font shipped, asset pipeline opt-in) |
| Test infrastructure | Lightweight | 261-check CI suite (visual regression on snes9x + Mesen2, lag detection, runtime, static, compiler patterns) |
| Scope | Library | Library + framework opt-ins (sprite engine, scene system, …) |

A user moving from one to the other should feel a **change in altitude**,
not a change in vocabulary. We deliberately keep names compatible
(`oamSet`, `bgSetMapPtr`, `WaitForVBlank`) to ease migration in either
direction.

---

## The five principles

### 1. Sane defaults, escape hatches

The 90% case is a one-liner; the 10% case is still possible.

> A high-level API does the right thing without ceremony. The low-level
> primitive remains accessible for users who need it.

**Example — sprite drawing:**
```c
oamDynamicDraw(id);              // high — engine picks size, NMI auto-flushes
oamMemory[id * 4 + 1] = y - 1;   // low — direct OAM byte poke, full control
```

Both are public API. We do not remove the low-level primitive in the
name of "API surface reduction". We document and recommend the
high-level path; the low-level path is there when the high-level isn't
the right shape (a sprite-heavy benchmark, a quirky port, an experiment).

### 2. Hide quirks, document the escape

The user does not have to discover hardware traps the hard way. The
person who *wants* to understand can find the explanation.

**Example — sprite Y +1 scanline quirk:**

The SNES PPU draws sprite OAM_Y at scanlines N+1..N+8. `oamSet` and
`oamSetY` subtract 1 internally so the user's `y` argument is the
rendered top scanline. Direct `oamMemory[]` writers must subtract 1
manually. **Both behaviours are documented in
`docs/hardware/OAM.md`.**

**Implication:** every piece of "magic" comes with a doc entry that
explains *what* is being hidden, *why*, and *what changes if you go
around it*. Magic without docs is a debugging trap waiting to fire.

### 3. Modules are opt-in, never all-or-nothing

The user pays only for the features the project consumes.

**Example — `LIB_MODULES`:**
```makefile
LIB_MODULES := console sprite sprite_dynamic dma input  # only these link
```

A project that doesn't draw text never links `text` or its bundled
font. A project that uses static sprites never links `sprite_dynamic`.
A project that doesn't use the dynamic sprite engine pays only ~25
cycles per VBlank for the (no-op) NMI hook indirection.

**Implication:** when adding a feature, the question is *"can this be
its own module?"* before *"where does this live in `text` or `sprite`?"*.
A module is a clean cut between linkage and complexity; a function added
to an existing module forces every consumer of that module to ship the
new code whether they want it or not.

### 4. Type-safe at the boundary

Compile-time errors beat runtime mysteries. Magic strings and magic
positional arguments rot.

**Example — engine init:**
```c
// Old (positional, opaque):
oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE16_L32);

// New (struct, named, type-safe):
static const OamDynamicConfig cfg = {
    .vramLarge      = 0x0000,
    .vramSmall      = 0x1000,
    .slotLargeInit  = 0,
    .slotSmallInit  = 0,
    .sizeMode       = OBJ_SIZE16_L32,
};
oamDynamicInit(&cfg);
```

**Implication:** any new public function with more than three arguments
gets a struct. Sentinel values (`OBJ_HIDE_Y = 240`, `OAM_Y_OFFSCREEN = 224`)
get named macros. Magic numbers in tutorials are replaced by the
matching constant.

### 5. Predictable performance

The user can reason about timing without guessing. The engine never
hides "lazy" allocations or background work that fires at unexpected
moments.

**Examples:**

- The NMI handler's call to `oamDynamicNmiFlush` costs 25 cycles when
  the engine is idle. **Documented in `templates/crt0.asm`.**
- `oamVramQueueUpdate` flushes up to 7 sprite tiles per call.
  **Documented in `lib/source/sprite_dynamic.asm`.**
- VBlank DMA budget is ~4 KB. Tilemap pages are split across multiple
  VBlanks when bigger. **Documented in `KNOWN_LIMITATIONS.md`.**

There is no garbage collector, no lazy initialisation pattern, no
implicit job queue. If something runs every frame, it is visible in
the source and quantified in the docs.

---

## Acceptance criteria for new features

When a new feature lands, walk through these five questions:

1. **Defaults & escape** — does the high-level path do the right thing
   in the common case, and does the low-level primitive still exist
   for the edge case?
2. **Quirks** — what hardware behaviour is being hidden, and where is
   it documented?
3. **Modularity** — is this a new module, an extension of an existing
   one, or core lib? If it's core, why?
4. **Types** — are the parameters named via struct/enum, or just
   positional `u16`s?
5. **Performance** — what does this cost in cycles, RAM, ROM bytes?
   Where is that written down?

A feature that fails one of these is not blocked from merging — but
the gap is identified, named, and tracked. We do not hide trade-offs
to make a PR look cleaner.

## Non-goals (the line we won't cross)

- **A monolithic engine class.** Components and scene managers stay
  modular and opt-in (Principle 3). No `OpenSnesEngine *engine = ...`
  global on which everything hangs.
- **Hidden allocations or hidden background work.** No GC, no lazy
  init, no implicit threads. The SNES has 128 KB of WRAM total.
- **A `printf` clone in core lib.** `textPrintU16` and `textPrintHex`
  cover the cases we actually use; full format-string parsing costs
  1-2 KB ROM for marginal value (Principle 5).
- **Mandatory framework lifecycle.** A user is welcome to write a
  `main()` with their own `while(1) { WaitForVBlank(); ... }` loop.
  The framework opt-ins (planned: `gameloop`, `scene_2d`) are
  recommendations, not requirements.
- **Wrapping every PVSnesLib name in OpenSNES branding.** Where the
  name is good and the semantics match, we keep the name (Principle 1
  for portability).

---

## Living document

These principles are not law — they are a snapshot of what we
currently believe makes OpenSNES distinctive. They will evolve as the
project does. Updates land via PR with the rationale in the commit
message; significant shifts get an entry in `CHANGELOG.md` under a
"Philosophy" subsection.

Cross-references:
- `README.md` for the elevator pitch and getting-started link
- `ROADMAP.md` for the planned framework opt-ins
- `KNOWN_LIMITATIONS.md` for the silent-failure catalogue
- `docs/hardware/` for the quirk documentation referenced in Principle 2
- `compiler/ABI.md` for the calling convention referenced in Principle 4
