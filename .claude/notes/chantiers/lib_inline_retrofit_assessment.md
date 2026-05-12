# Lib inline retrofit — feasibility assessment (rejected)

**Date**: 2026-05-12
**Status**: REJECTED — current SDK state makes the retrofit cost-prohibitive
**Context**: Follow-up to the function-inlining chantier shipped 2026-05-12
(commit `d825295`). The shipped feature enables `inline` keyword for user
code; this assessment evaluated retrofitting it to lib helpers.

## Findings

### Inline candidates in lib (post-trace audit)

After `CC_TRACE_INLINE=1 make -C lib`, the linear-flow / leaf / ≤ 8 IR-instr
heuristic identifies **7 candidates**:

| Function | ins | Source |
|---|---|---|
| `getBrightness` | 1 | console.c |
| `setScreenOff` | 3 | console.c |
| `mosaicInit` | 4 | mosaic.c |
| `hdmaWaveSetSpeed` | 5 | hdma.c |
| `scopeCalibrate` | 6 | input.c |
| `colorMathDisable` | 7 | colormath.c |
| `colorMathInit` | 8 | colormath.c |

### Example usage frequency

`grep -rl '\bFN\b' examples/*/main.c examples/*/*/main.c …`:

| Function | Examples using |
|---|---|
| `setScreenOff` | **22** |
| `scopeCalibrate` | 1 |
| `colorMathInit` | 1 |
| others | 0 |

Only `setScreenOff` has enough call sites to justify retrofit work.

### Two attempted patterns — both blocked

#### Pattern A: `inline` keyword in .c definitions (intra-TU only)

Theory: mark `colorMathInit` / `colorMathDisable` etc. with `inline` in
their .c files. Other functions in the same .c file (e.g., colormath.c
wrappers `colorMathTransparency50`) could inline them via the new pass.

**Result**: zero benefit. The wrappers in colormath.c call
`colorMathEnable` / `SetOp` / `SetHalf` / `SetSource` /
`SetFixedColor` — none of which pass the eligibility heuristic
(too large, control flow). They do NOT call `colorMathInit` or
`colorMathDisable`, so marking those helpers inline yields no
intra-TU win.

Other lib files have similar structure: tiny API-level helpers
without internal callers.

#### Pattern B: `static inline` in header (cross-TU)

Theory: move `setScreenOff` body to `console.h` as `static inline`.
Each TU including the header gets the inline body; the new pass
inlines it at each call site (22 examples).

**Blocker**: when the inline pass declines (or for any call site that
can't inline, e.g., function-pointer use), cproc emits the static body
in the .c file's compiled .asm. WLA-DX has no concept of TU-scoped
symbols — all emitted labels become global at link time:

```
lib/build/lorom/background.o: build/lorom/background.asm:9:
  _TRY_PUT_LABEL: Label "setScreenOff" was defined more than once.
```

Every lib .c file that includes `console.h` (transitively, via `snes.h`)
emits its own `setScreenOff` standalone. Linker rejects the duplicates.

Removing the .c file's own definition didn't help (other lib TUs still
duplicate it). The pattern is structurally incompatible with the
current WLA-DX + cproc + multi-TU lib build.

### Three potential resolutions — all out of scope

1. **cproc fix**: don't emit `static inline` standalone bodies when the
   pass inlined every call site of that symbol within the TU. Requires
   a new pass or post-emit pruning. Estimated: 1–2 days.

2. **Symbol mangling**: cproc emits `static inline` functions with a
   per-TU prefix (e.g., `__static_console_setScreenOff`). Loses the
   plain symbol name; debugging cost. Estimated: 1 day + symbol-table
   audit.

3. **C99 `inline` + one `extern inline` per symbol**: header has
   `inline void setScreenOff(void) { … }` (external linkage, no static).
   One .c file has `extern inline void setScreenOff(void);` to force
   the single external definition. cproc's `inlinedefn` semantics may
   or may not produce the expected behaviour — needs experimental
   validation. Estimated: 0.5 day for one symbol, multiply for the
   handful that benefit.

## Recommendation: ship as-is

The 2026-05-12 function-inlining chantier already delivers the win
for the patterns that matter:

- **User-written code** with `static inline` helpers in a single TU
  inlines correctly (eligibility heuristic + splice work end-to-end).
- The bench fixture's `call_chain` regression (+31.9 % → -59.6 % vs
  PVSnesLib+opt) is recovered via this exact pattern.
- Total cycle reduction vs PVSnesLib+opt: -29.1 % → -31.4 %.

The lib retrofit would have delivered cycle savings on `setScreenOff`
call sites in 22 examples (~28 cycles × ~1–2 calls per example =
~600–1200 cycles total), at the cost of one of the three resolutions
above. The cost/benefit is unfavourable until a separate compiler
chantier addresses the cproc/WLA-DX static-inline interaction.

## Cross-references

- `compiler/qbe/inline.c` — the inline pass (linear-flow heuristic)
- `compiler/cproc/decl.c:1093` — inlinedefn semantics
- `compiler/cproc/qbe.c:1405` — `inline` prefix emission
- `docs/BENCHMARK.md` — measured cycle gains
- Develop commit `d825295` — function inlining chantier ship

## When to revisit

- If a future chantier addresses cproc symbol mangling or selective
  standalone emission for static inline (option 1 or 2 above)
- If a single high-value lib symbol (e.g. `setScreenOff` becoming a
  hot path) justifies the C99 `inline` + `extern inline` pattern
  cost (option 3) for that symbol only
- If we add a header-only sub-library (a separate `snes/inline.h` of
  hand-curated tiny helpers, no .c counterpart) — but that's
  architecturally different from "retrofit" and deserves its own
  chantier proposal
