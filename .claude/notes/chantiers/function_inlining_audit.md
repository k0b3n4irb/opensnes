# Function inlining — audit + design

**Date**: 2026-05-12 (Phase 0 audit + Phase 1 design)
**Status**: design ready, Phase 2 implementation pending
**Trigger**: user-stated priority #2 ("A7 puis function inlining") for
"enterrer définitivement PVSnesLib en performance".

## Phase 0 — Audit findings

### Current inline support across the compiler stack

| Layer | State | Verdict |
|---|---|---|
| **cproc** (parser) | Parses `inline`/`__inline` → `TINLINE` (token.c:35), `FUNCINLINE` (decl.c:136), `d->u.func.inlinedefn` (decl.c:1093) | ✅ keyword accepted |
| **cproc → IR** (`qbe.c`) | NO propagation of `inlinedefn` to QBE IR. cproc emits all functions as plain `function $foo()` regardless of the flag | ❌ silent no-op |
| **qbe IR** | No `inline` attribute. No `inline.c` pass. `Lnk` struct has `export/thread/common/align/sec/secf` but no inline marker | ❌ no support |
| **qbe → asm** (`w65816/emit.c`) | `Ocall` always lowered to JSL + cleanup | ❌ no opportunity |

**Bottom line**: implementing inlining is a full end-to-end addition,
not an extension. The `inline` keyword is currently accepted silently.

### Motivated by BENCHMARK regression

`docs/BENCHMARK.md` (2026-05-10 baseline) shows OpenSNES at **-29.1%
total cycles vs PVSnesLib+816-opt**. Only one major regression:
`call_chain (helper(helper(x)))` at **+31.9%** (62 cycles vs PVSnesLib's
47). Root cause documented in BENCHMARK.md: TCO landing required a
frame for the general chained-call case, sacrificing this specific
shape.

Inlining the inner `helper` collapses the chain to a single JSL+RTL
pair, bringing the shape under 40 cycles — would recover the only
remaining structural regression.

### Candidate functions (lib + bench)

Lib C functions shortest enough to be likely inline candidates after
the heuristic ≤ 8 IR instructions, single-block:

| Function | Lib file | Lines (`.c.asm`) | Likely inline-eligible |
|---|---|---|---|
| `superfx*` helpers | `superfx.c.asm` | 34 | ✅ several single-return wrappers |
| `text4bppEnable` etc. | `text4bpp.c.asm` | 40 | ✅ MMIO setter wrappers |
| `sa1Status` etc. | `sa1.c.asm` | 50 | ✅ getters |
| `gameloop` core | `gameloop.c.asm` | 84 | ⚠️ probably too big |

Full audit of each candidate's IR size deferred to Phase 1 implementation.

### Expected gains

- **call_chain bench**: 62 → ~38 cycles (-39%, recovers regression)
- **sa1/superfx status checks in hot paths**: per-call ~35-50 cycles
  saved on the JSL/RTL/prologue/epilogue overhead
- **User code** with `static inline` helpers: depends on usage

**Cost (ROM)**: body size × call site count. With the conservative
heuristic (≤ 8 IR instr ≈ 16-32 asm bytes), 10 call sites of a small
helper add ~200-300 bytes. Must stay under the bank $00 budget
(currently 28 bytes free on `examples/maps/mapscroll` — the tightest).

## Phase 1 — Design

### Approach C (Hybrid): cproc flag propagation + qbe inline pass

Selected over Approach A (AST-only) and B (full IR inlining):
- A is too restrictive (only single-expression functions)
- B is too risky given the A6+A7 chantier's lessons (cascade discovery)

C is the conservative middle ground: cproc flags the function
as inlinable, qbe does conservative IR-level inlining only for
trivial cases.

### Heuristic (conservative — user-chosen)

A function is eligible for inlining iff ALL of:

1. **`inline` keyword present** in source (cproc sets `FUNCINLINE`)
2. **Single block** (no control flow — `if/while/for` make a function ineligible)
3. **No calls** in body (avoid recursion + cascading inline)
4. **No `alloc*`** in body (avoid stack frame complications)
5. **Body ≤ 8 IR instructions** (after standard QBE passes —
   threshold tunable per-build via `CC_INLINE_MAX_INSTR=N`)

### Implementation sites

**Site 1 — `compiler/cproc/qbe.c` (cproc → IR emission)**

Where functions are emitted, add `inline` attribute before `function`
keyword when `d->u.func.inlinedefn` is set:

```diff
- fprintf(stdout, "function %s$%s(",
+ fprintf(stdout, "function %s%s$%s(",
+         d->u.func.inlinedefn ? "inline " : "",
          rc, name);
```

**Site 2 — `compiler/qbe/all.h` (Lnk struct extension)**

```diff
 struct Lnk {
     char export;
     char thread;
+    char inline_hint;  /* function eligible for inlining */
     char common;
     ...
 };
```

**Site 3 — `compiler/qbe/parse.c` (parse `inline` keyword)**

Add `Tinline` token, recognize as linkage modifier before `function`.
Set `curf->lnk.inline_hint = 1`.

**Site 4 — `compiler/qbe/inline.c` (NEW pass)**

```c
void
inline_pass(Fn *fn, Fn **all_fns, int nfns)
{
    /* For each Ocall in fn:
     *   1. Find target Fn in all_fns by name
     *   2. Check eligibility (single-block, ≤ N instr, no calls/allocs)
     *   3. If eligible: clone body, rename temps, splice into caller
     */
}
```

**Site 5 — `compiler/qbe/main.c` (pipeline integration)**

Inline pass requires module-level view (all functions in scope).
Options:
- **Two-phase**: parse all functions first, then run inline + lowering
  pass per-function. Requires `main.c` restructuring.
- **JIT mode**: when a function is parsed, store it in a global table.
  When a caller is lowered, look up eligible callees and inline.

The two-phase approach is cleaner. Estimated extra ~50 lines in main.c.

**Site 6 — `compiler/qbe/w65816/emit.c` (no change needed)**

Inlining happens before emit, so `Ocall` to inlined functions is
removed before lowering. Native `Ocall` to non-inlined functions
continues unchanged.

### Test fixtures

**Fixture 1 — bench regression target**:
`tools/opensnes-emu/test/fixtures/benchmark/bench_functions.c` already
has `call_chain` — re-run after inlining lands. Goal: cycle count <
PVSnesLib+opt's 47.

**Fixture 2 — minimal inline test** (NEW):
`compiler/tests/inline_trivial.c` — single inline function called
once, verify body inlined.

**Fixture 3 — heuristic boundary** (NEW):
`compiler/tests/inline_boundary.c` — function at exactly 8 instr (inline),
9 instr (not inline). Verify gate.

**Fixture 4 — non-eligibility cases** (NEW):
- Function with `if` → not inlined
- Recursive function → not inlined
- Function with `alloc*` → not inlined

### Acceptance criteria

- `make lint-docs` zero drift
- `node test/run-all-tests.mjs --quick` → 269/269 green
- `call_chain` in BENCHMARK.md drops below 47 cycles (PVSnesLib+opt level)
- Cycle bench TOTAL improves (target: -32% or better vs PVSnesLib+opt)
- Per-build override via `CC_INLINE_MAX_INSTR=0` disables inlining
  entirely (escape hatch)
- ROM size stays under BANK0_FAIL_THRESHOLD margin on all examples

### Estimated effort

- **Phase 2 implementation**: 1-2 days focused work
- **Phase 3 validation**: 0.5 day (bench + Mesen2 spot check)
- **Total**: ~2-3 days, breakable across sessions

### Risks + abort conditions

| Risk | Mitigation |
|---|---|
| Temp renumbering bugs (collision with caller's temps) | Property test: every IR pass test with random inline targets |
| ROM bloat exceeds bank $00 budget | `CC_INLINE_MAX_INSTR=0` escape hatch + measure per-example |
| Multi-block inlining accidentally accepted | Strict single-block gate in eligibility check |
| Module-level architecture change breaks existing passes | Two-phase restructuring done as separate prep commit |

Abort if:
- Phase 2 reveals > 3 unexpected qbe architecture changes needed
- ROM bloat exceeds budget on > 2 examples after heuristic tuning
- Cycle bench shows regression on more than 2 non-call_chain functions

## Phase 2 — Implementation outline (for next session)

Order of operations:

1. **0.a** — Test fixtures committed first (TDD): `compiler/tests/inline_*.c`
   with expected ASM patterns. They start failing.
2. **0.b** — Two-phase main.c restructuring (no behavior change, just
   refactor). Commit, verify 269/269.
3. **1.a** — Add `Lnk.inline_hint` + parse `inline` keyword in qbe/parse.c.
   Commit, verify 269/269 (no behavior change yet, just data flow).
4. **1.b** — cproc/qbe.c emits `inline` prefix. Verify by reading
   intermediate `.qbe.ir` for inline-marked C functions.
5. **2.a** — `inline.c` skeleton with eligibility check (no inlining yet,
   just identification + tracing under `CC_TRACE_INLINE=1`).
6. **2.b** — Actual inlining: clone body, renumber temps, splice into
   caller. Test fixture 2 passes.
7. **2.c** — Boundary tests pass (fixtures 3 + 4).
8. **3** — Validate `call_chain` in BENCHMARK.md drops. Re-baseline.
9. **4** — Commit to qbe submodule + parent PINS.md bump.

## Cross-references

- `docs/BENCHMARK.md` — current cycle baseline (incl. call_chain
  +31.9% regression)
- `compiler/qbe/doc/il.txt` — QBE IL syntax (for adding `inline`
  attribute)
- `compiler/qbe/all.h` — `Lnk` struct definition
- `compiler/qbe/parse.c:929` — `parsefn` (where to plug `inline` parse)
- `compiler/cproc/qbe.c` — function emission point (where to add
  `inline` prefix)
- `.claude/STRUCTURAL_DEFECTS.md` — function inlining is not catalogued
  (it's a discretionary perf chantier, not a structural defect)
