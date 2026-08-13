# Codegen bug: a ternary yielding an address constant drops the bank byte

**Status:** OPEN (unfixed), found 2026-08-12 while building
`examples/sprites/aseprite_pipeline`. Class A (compiler / QBE w65816 backend).
Silent failure — wrong runtime data, no diagnostic.

## Symptom

```c
sink(x ? "AAAA" : "BBBB");   /* sink() receives a CORRUPT far pointer */
```

A conditional expression whose two operands are **address constants** (string
literals here, but any symbol address) materialises only the **low 16 bits** of
the resulting far pointer. The **bank byte is never written**, so the value
handed onward points into a garbage bank/offset. In the example this made
`textPrintAt(2, 3, cond ? "Clip: WALK…" : "Clip: WAVE…")` read an empty string
and print nothing — the row-3 label was silently blank while every non-ternary
`textPrintAt("literal")` on the same screen rendered fine.

Post-A6 the ABI passes pointers as **4-byte slots** (`pea.w :sym` + `pea.w sym`).
The direct-argument path emits both halves; the phi/select path does not.

## Minimal repro

`ternary_addr_const_bank_drop_repro.c` (next to this note):

```c
extern void sink(const char *s);
void f_arg(int x)   { sink(x ? "AAAA" : "BBBB"); }                 /* broken */
void f_local(int x) { const char *p = x ? "AAAA" : "BBBB"; sink(p); } /* also broken */
extern void isink(int v);
void f_int(int x)   { isink(x ? 11 : 22); }                       /* control: fine */
```

Compile: `bin/cc65816 ternary_addr_const_bank_drop_repro.c -o out.asm`

## The generated code (the bug)

`f_arg` — each branch stores **only the low word** into the phi slot, and the
call then pushes the (never-initialised) high word `6,s` **twice**, never the
low word `4,s`:

```asm
@cond_false.4:
	lda.w #string.3
	sta 4,s          ; low word only  (bank half at 6,s never written)
	jmp @cond_join.5
@cond_true.3:
	lda.w #string.2
	sta 4,s          ; low word only
@cond_join.5:
	lda 6,s          ; <-- uninitialised bank half
	pha
	lda 6,s          ; <-- pushes the bank half again; low word 4,s dropped
	pha
	jsl sink
```

`f_local` shows the same missing-bank write (`10,s` never set), so the defect is
in the **phi/select lowering of an address constant**, independent of whether
the result is passed directly or via a local. `f_int` (ternary of integer
constants) is correct — the bug is specific to address constants, where the
value is 4 bytes but only 2 are emitted.

## Root layer

QBE w65816 backend. The phi copy / select lowering treats a symbol
address-constant operand as a 16-bit value and emits a single `lda.w #sym ;
sta lo` instead of also emitting the bank half (`lda.w #:sym ; sta hi`, or the
`pea.w :sym / pea.w sym` pair the direct call path uses). Compare with the
correct direct-argument lowering in `compiler/qbe` and the far-pointer ABI in
`compiler/ABI.md` (post-A6 4-byte pointer slots).

## Impact

Any `cond ? PTR_A : PTR_B` (string tables, lookup selection, "pick one of two
buffers") passed onward or stored can carry a wrong bank. Common idiom; the
failure is silent. This is why the example was written with `if/else` around two
`textPrintAt` calls rather than a ternary — a deliberate avoidance with this
note as the paper trail, **not** a fix.

## Suggested next steps

1. Reproduce with the file above; bisect the QBE lowering (`regression_method`).
2. Fix the phi/select of address constants to emit both halves (mirror the
   direct-arg `pea.w :sym / pea.w sym`).
3. Add a `devtools/compiler-tests/` case: `cond ? "a" : "b"` → both bank pushes
   present; and a runtime luna probe asserting the selected string is read.
4. Candidate for a `.claude/STRUCTURAL_DEFECTS.md` entry (silent-failure,
   compiler layer) if not fixed promptly.

## Cross-references

- `examples/sprites/aseprite_pipeline/main.c` — `drawLabel()` uses if/else + a
  comment pointing here.
- `compiler/ABI.md` — post-A6 4-byte far-pointer slots.
- `.claude/rules/debugging.md` / `regression_method.md` — fix the root cause.
