# Chantier #132 — a `const` array of structs loses its bank byte

Status: **CLOSED** — shipped 2026-07-22. Three defects, not one; the
first two masked each other, which is why neither was visible alone.
Kept as a worked example of a bug whose "obvious" fix is wrong.

## The reported symptom

A `static const` array of structs indexed at runtime is read from bank
$00, whatever bank it is actually in. The build's bank-blind check
refuses it, so no ROM ships broken — but it forces users into parallel
scalar arrays. `tmx2snes -e` currently emits that workaround shape,
which is a tool accommodating a compiler bug and should be undone once
this lands.

## First diagnosis — CORRECT BUT INCOMPLETE (kept for the reasoning)

`compiler/cproc/qbe.c`, the pointer-arithmetic operand widening:

```c
if (e->type->kind == TYPEPOINTER) {
    if (e->u.binary.l->type->kind != TYPEPOINTER)
        l = funcinst(f, IEXTUW, ptrclass, l, NULL);
    ...
}
```

`IEXTUW` means *zero-extend a 16-bit word to 32 bits*. cproc types a
struct member's address computation as **`long` (size 4)**, not as a
pointer — confirmed by instrumenting the site:

```
PTRARITH: lkind=12 lsize=4 …   (tab + i      — left IS a pointer)
PTRARITH: lkind=7  lsize=4 …   (… + offsetof — left is TYPELONG!)
```

so the guard fires on a value that is already 4 bytes wide and
**truncates it to its low word, discarding the bank**:

```
%.7 =l add $tab, %.6     ; full 24-bit address, bank in the high half
%.8 =l extuw %.7         ; <-- bank destroyed here
%.9 =l extuw 0
%.10 =l add %.8, %.9
```

## The first fix attempt — SUPERSEDED

Widen only operands **narrower** than a pointer. With it, the IR becomes
`add $tab, %off` / `add %.6, 0` and every access form is correct:

| form | before | after |
|---|---|---|
| `tab[i].a` | `lda [tcc__r9]` with `lda.w #0` bank | `lda.l tab,x` |
| `tab[i].b` (offset) | idem | `lda.l tab+1,x` |
| `ents[i].y` (u16) | idem | `lda.l ents+2,x` |
| `ents[i].s` (ptr) | idem | `adc.w #:ents` — bank carried |
| `p[i].b` (param) | `lda [tcc__r9]` | unchanged (correct: runtime bank) |

A size-based guard (`type->size < 4`) is **wrong** — an array's `size`
is its byte length, so `char buf[6]` would skip widening while
`char buf[2]` would not. The guard must key on the operand already
being of pointer class, not on its size.

## Why that attempt could not ship — and the WRONG theory it produced

With the fix, `make tests` gives 14 visual regressions. They are not
timing shifts. Minimal reproducer:

```c
typedef struct { u16 addr; u16 tile; u8 pal; u8 prio; } Cfg;
Cfg cfg;
u16 build(void) {
    return cfg.tile | ((u16)cfg.pal << 10) | ((u16)cfg.prio << 13);
}
```

IR is correct (`add $cfg, 2`, `add $cfg, 4`, `add $cfg, 5`), but the
emitted code contains **one** load:

```asm
lda.w cfg+2      ; cfg.tile — right
sta 2,s
lda.w #0         ; cfg.pal  — the load is GONE
and.w #$00FF
```

The `and.w #$00FF` is `Oloadub`'s own tail, so the load instruction is
reached; what fails is its address. Both the `Oadd Kl` producing the
address temp and the `Oloadub` consuming it degrade to `lda.w #0`,
which is what `emitload` emits for a temp with **no slot and no
register**.

> **This is where the reasoning went wrong.** From "`lda.w #0` means an
> unallocated temp" I concluded the slot allocator was at fault, and
> went looking in `mark_addr_only_kl` and `is_nop_instruction`. The
> allocator was innocent. Instrumenting the emitter showed `Oloadub`
> never reaching `emitins` at all — the instruction had been rewritten
> upstream, by a QBE pass, into a shift of an earlier load. Dumping the
> IR after each pass (`qbe -d A`) found it in one command. **Read the
> IR between passes before theorising about the backend.**

Real-world impact of that miscompilation: `build_tile_entry` in
`lib/source/text.c` reads `text_config.palette` and `.priority` from
`text_config+2` (the font_tile field) instead of `+4` / `+5` — which is
why the RPG's HUD digit vanished.

The defect is **pre-existing**. It is unreachable today only because
cproc's spurious `extuw` turns these addresses into runtime values,
hiding the folded-constant-address path.

## What it actually was: three defects in a chain

The slot theory above was wrong — the address temp is fine. The real
chain, found by dumping the IR after each QBE pass (`qbe -d A`):

**1. `convert()` used QBE's native class model.** `class = dst->size ==
8 ? 'l' : 'w'`, but this target maps 4-byte integers to `l`. Converting
to `unsigned long` therefore produced a `w`-class value, so pointer
arithmetic's scaling multiply came out as `%x =l mul %i, 1` with a `w`
operand — malformed IR.

**2. The pointer-arithmetic widening repaired that by accident**, and
broke the bank in the process. It extended any operand whose C type kind
was not `TYPEPOINTER`: that fixed the multiply (its result got extended
in the following add) while truncating member addresses that were
already 4 bytes wide. **The two defects hid each other.** Removing the
over-extension alone breaks `p[i]` through a parameter; fixing the
conversion alone leaves the bank loss. Both had to move together.

**3. QBE's load forwarding assumed a 4-byte `w`.** With (1) and (2)
fixed, addresses reach the folded `$sym + const` form for the first
time, and `load.c` started serving byte loads out of an earlier word
load by shifting 16 and 24 bits — from a load that only ever fetched 2
bytes. That is what made `text_config.palette` read out of the
`font_tile` field and blanked the RPG's HUD digit. Target now carries a
`wordsz`.

## How it was proved safe

The final codegen shifts cycle counts, so 12 examples sample a different
frame and their fbhash moves. Instead of eyeballing screenshots, each was
checked with `luna wram-trace`: WRAM at vblank N is the same game frame
regardless of how fast the code runs, so identical per-frame hashes mean
identical game logic. **All 12 matched at every one of 400 vblanks**,
SA-1 and Super FX included. Only then were the baselines updated.

Two compiler pattern tests had to be re-pinned: they fixed one spelling
of a spill (`sta N,s`) and of a mode restore, both of which the improved
codegen writes differently while preserving the property. Each re-pin
was checked non-vacuous by simulating the original bug.

## Bonus

The A6 far-pointer matrix went from 7/8 green with 4 XPASS to **8/8 with
5 XPASS** — bank-2 byte, word and parameter derefs all close. Those were
recorded as an "A6 far gap", not as consequences of this bug.

## Debt paid in the same series

`tmx2snes -e` emitted parallel scalar tables **because of this bug**, and
its README called it a design choice. It was not one. The tool now emits
`<TYPE>_FIELDS` + `<TYPE>_TABLE` — an array of structs, which is what a
game wants to write — and `examples/games/rpg` consumes it that way.

That is the point worth remembering from this chantier: the workaround
had already been written into a **public tool output format**, which is
a contract, not a local hack. Encoding a compiler bug there is far worse
than working around it in one example, because the example is deleted in
a commit and the format is not.
