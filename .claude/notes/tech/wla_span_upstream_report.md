# Upstream issue for vhelin/wla-dx — FILED

**Filed 2026-07-26: https://github.com/vhelin/wla-dx/issues/729**

The text below the first `---` (down to the second `---`) is what was
posted. The title was flattened to plain text (backticks render literally
in GitHub titles):
"wlalink: FIX_LABEL_ADDRESSES internal error when a SUPERFREE section
exactly fills a ROM bank (SPAN regression)".

---

## Title

`wlalink`: `FIX_LABEL_ADDRESSES: Internal error` when a `SUPERFREE`
section exactly fills a ROM bank (regression from the SPAN change)

## Summary

A `SUPERFREE` section whose size is exactly one bank fails to link on
current `master`: its trailing label lands on the bank boundary and
`wlalink` aborts with `cannot map label`. A section one byte smaller, or
one byte larger, links fine. `git bisect` points at the commit that
added `SPAN`.

## Version

- Bad: `master` (reproduced on `4f8bbdce`, `v10.7-9`).
- First bad commit: **`4c3c042e`** — *"Added SPAN to .SECTIONs so a
  .SECTION can be placed at the border of two ROM banks. Should fix
  GitHub issue #663"* (`v10.7-7`).
- Last good commit: its parent `a369bec5` (`v10.7-6`).
- The released **v10.7 tag is not affected** — this is a master-only
  regression introduced 7 commits after the tag.

Found via bisection over the 93 commits between `a369bec5` and
`4f8bbdce`.

## Minimal reproducer

Self-contained, no external files. `main.asm`:

```
.MEMORYMAP
DEFAULTSLOT 0
SLOT 0 $8000 $8000
.ENDME
.ROMBANKMAP
BANKSTOTAL 2
BANKSIZE $8000
BANKS 2
.ENDRO

.SECTION "big" SUPERFREE
big:
.dsb $8000, $11      ; exactly one bank (32768 bytes)
big_end:
.ENDS
```

`link.txt`:

```
[objects]
main.o
```

Build:

```
wla-65816 -o main.o main.asm      # assembles fine
wlalink -s link.txt main.sfc      # aborts on master
```

## Expected

Links cleanly (as it does on `a369bec5` and on the v10.7 release),
placing `big` in one bank and `big_end` at the following bank boundary.

## Actual (on master)

```
wlalink: FIX_LABEL_ADDRESSES: Internal error: cannot map label "big_end" in section "big".
```

## The boundary is the trigger

Same memory map, only the section size changes:

| `.dsb` size | `a369bec5` (good) | `4c3c042e` (bad) |
|---|---|---|
| `$7ff0` (bank − 16, does not fill the bank) | links | links |
| **`$8000` (fills the bank exactly)** | **links** | **`cannot map label`** |
| `$8001` (spills one byte into the next bank) | links | links |

So it is not an out-of-space condition — the ROM has a whole empty bank.
It is specifically a section that ends *on* a bank boundary, i.e. a
trailing label whose address is the first byte of the next bank. That is
exactly the placement the `SPAN` change reworked, which is consistent
with it being the first bad commit.

In a real project this also surfaces as
`MEM_INSERT: Overwrite at $XXXX (old $cd new $YY)` a few commits later on
master, when the mis-mapped section overlaps another — same root cause,
different symptom.

## Environment

- Linux x86_64, clang, `cmake -G "Unix Makefiles"`.
- w65816 target (but the failing code is target-independent `wlalink`
  placement, so other CPUs with a same-size-as-bank `SUPERFREE` section
  should reproduce).

---

## Notes for us (not part of the issue)

- Tone: report, ask, offer. vhelin authored the SPAN commit himself, so
  a precise first-bad-commit + minimal repro is the most useful thing we
  can hand him; no need to speculate about the fix.
- If asked, we can also share that #663's SPAN feature is what we tripped
  — the interaction is a section that *exactly* fills rather than spans.
- Our stake (a code generator emitting WLA that produces bank-exact
  SUPERFREE sections) is worth one sentence at most, and only if he asks
  why it matters. The bug stands on its own.
