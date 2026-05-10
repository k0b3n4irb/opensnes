---
name: cc65816 codegen — JSL inside an if branch (FIXED, kept as historical note)
description: Old workaround macro pattern for a cproc/QBE codegen bug. Bug fixed silently by another QBE chantier; the function form is now safe. Kept as a historical note in case the symptom returns.
type: project
originSessionId: b92ad4fc-ea9c-4479-b229-46e53ee464ae
---

## Status: FIXED (chantier C.7, 2026-05-06)

The bug no longer reproduces. Validated by converting the
`mode_large_size` / `mode_small_size` helpers from `#define` macros
back to plain functions in their own translation unit
(`lib/source/sprite_dynamic_helpers.c`) so cproc cannot inline them
across TU. With real `jsl` calls in `oamDynamicDraw` and
`oamMetaDrawDyn`, all 265 visual checks pass and `slopemario`
(the historical repro) still renders Mario correctly.

The exact QBE patch that addressed this is unknown, but it landed
between 2026-04-28 (bug observed) and 2026-05-06 (no repro). Likely
candidates from the chantier history:

- C.1 / C.2.1 / C.2.2 — tail call optimisations and frame
  teardown work that touched call lowering
- Phase 5b — `leaf_opt` gate removal that altered when frames are
  emitted, which in turn changed the slot layout around calls
- C.5 — DL/DW init padding fix (touched emit.c, though the
  emit path for `Ocall` is separate)

If a similar symptom resurfaces, the diagnosis route is:

1. Reproduce: helper in its own TU (no `static`, no `inline`),
   called from inside an `if (cond) { sz = helper(state); }`
   block, followed by a chain of `cmp #N` dispatches. Confirm
   the JSL is actually emitted (`grep jsl <name>` in the .asm).
2. Inspect the post-JSL store/load pair: in 2026-04-28 the
   problem was `sta 6,s` followed by `lda 6,s; and.w #$00FF;
   cmp.w #16` reading a value that didn't match what the helper
   returned. Re-check that store/load sequence first.
3. If the issue is back, the fastest mitigation is the original
   workaround: implement the helper as a `#define` macro so it
   inlines and no JSL appears inside the conditional. The
   pre-fix shape of `MODE_LARGE_SIZE(mode)` in
   `sprite_dynamic_internal.h` is the canonical example.

---

## Original write-up (kept for context)

When writing C for the OpenSNES library, **avoid this pattern**:

```c
sz = some_array[id];
if (sz == 0) {
    sz = some_helper_function(state);   /* helper returns u8 */
}
if (sz == 8) ...
else if (sz == 16) ...
```

The cc65816 / QBE codegen for the conditional + JSL produced an
asm sequence where the post-JSL `sta 6,s` was written but the
following `lda 6,s; and.w #$00FF; cmp.w #16` read back a value
that did not match what the function returned. The dispatch
silently fell through and the work in the `if` arms never ran.

Discovered in lib/source/sprite_dynamic_dispatch.c (`oamDynamicDraw`)
on 2026-04-28 while building the size-aware dispatch. The fallback
path lifted the per-sprite size mode from `oam_dynamic_size_mode`
through a `mode_large_size()` helper, and `slopemario.sfc` (which
exercises the fallback because Mario is sprite id 0 with the
default mode value) ended up rendering nothing — Mario invisible.

The bypass tests narrowed it down: replacing `mode_large_size(...)`
with a hardcoded `sz = 16` in the `if (sz == 0)` arm restored
correct dispatch. The mitigation was to define the helper as a
`#define` macro so the call inlined and no JSL appeared inside
the conditional.
