# Negative fixtures — C the toolchain must REFUSE

Each `<name>.c` here must fail to compile, and cc65816's stderr must contain
the text in `<name>.expect`. They pin the three C features the w65816
toolchain does not implement — variadic functions, struct parameters /
returns / assignment by value, inline assembly — as **clean refusals**: the
historical failure mode was silent (a dropped struct assignment shipped
garbage in 2026-07), and a refusal that names the feature is the contract
`KNOWN_LIMITATIONS.md` documents. If one of these starts compiling, the
feature landed: move the fixture to `cases/` with real `.checks`.

Run with the rest: `python3 devtools/compiler-tests/run.py` (gaps review C1/C2,
2026-09-13).

`float_arith` and `long_long` (2026-10-08) pin two refusals that were silent
miscompilations until then: floating-point and 64-bit values computed at run
time. They are not features waiting to land; `cases/wide_constants_fold` pins
the other side, the constants of those types that must keep compiling.

`vla` and `vla_const_length` (2026-10-17, audit 04-1) are the same kind: a
variable-length array compiled without a word into a pointer to a 2-byte
frame slot, and the callee wrote over the return address. A `const`
variable as the length is the form a game is likely to write.

`anim_clip_256` is of another kind: a refusal the **library** makes (a
`_Static_assert` in `DECLARE_ANIM_CLIP`, `snes/anim.h`), pinned here because
this is where a compile that must fail is checked. It stays for good.

`far_address_plain_pointer` (2026-10-25, STRUCTURAL_DEFECTS A10) is a
refusal of the frontend, like the `__far` assignment: an address above
`$FFFF` cast to a plain pointer lost its bank on an indexed access.
`far_address_expr` is the same cast written as a base macro plus an
offset, which reaches the cast unfolded. `cases/far_address_cast` pins the casts that must keep compiling.
