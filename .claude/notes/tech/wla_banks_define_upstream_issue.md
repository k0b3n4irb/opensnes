# `BANKS` and `.DEFINE` — the issue we were about to file, and why we must not

Status: **RETRACTED, do not send.** The capability already exists. Kept
because the way I got it wrong is the reusable part.

## What I claimed

That `.SECTION … SEMISUPERFREE BANKS` cannot take a `.DEFINE`, unlike
`BANK`, `ORG` and `PRIORITY`, and that this blocked issue #127's
default-placement work.

## What is actually true

It takes one — it has to be a **string** definition:

```asm
.DEFINE MY_BANKS "3-1"                          ; works
.SECTION "x" SEMISUPERFREE BANKS MY_BANKS

.DEFINE MY_BANKS 3-1                            ; "malformed BANKS list"
```

Measured on our pinned assembler:

| define form | result |
|---|---|
| `"3-1"` (string) | section placed at `03:8000` |
| `3-1` (bare)     | `_UNROLL_BANKS: … malformed BANKS list (MY_BANKS)` |

A bare `3-1` is an expression, so it never reaches the parser as text.
The quoted form is expanded by the tokenizer before the `strcpy` in
`phase_1.c` ever runs.

## How the wrong conclusion survived scrutiny

1. First test used `.DEFINE ASSET_BANKS 7-1` — unquoted. It failed.
2. I read `phase_1.c` and found `strcpy(g_sec_tmp->banks, g_tmp)`, no
   expression evaluation. **That looked like confirmation.**
3. I built upstream master, reproduced there, wrote a patch, and proved
   the patch "fixed" it — but in the same edit I also quoted the define.
   So the run that passed differed in *two* ways and I attributed the
   change to the patch.

Reading the source made me more confident, not more correct: it
explained the symptom I had, so I stopped looking. The control I skipped
was the cheap one — quoted define, unpatched binary — and it is the one
that decides.

**Rule for next time: when a source reading confirms a hypothesis, that
is exactly when to run the negative control.** A patch that changes two
things at once has proved nothing.

## Consequence for #127

The blocker is gone and needs no upstream change. The bank list now
lives in the memory maps (`.DEFINE ASSET_BANKS "7-1"`) and
`templates/assets.inc` refers to it symbolically, so layout knowledge is
in one place. The same mechanism is available to QBE for `.rodata.N`,
which is what proposal 3 needs.

## Separately: upstream master regresses our corpus

Syncing `compiler/wla-dx` to `4f8bbdce` (93 commits ahead of our pin)
builds, but two examples fail to link:

```
FIX_LABEL_ADDRESSES: Internal error: cannot map label "track_class_end" in section ".rodata3".
MEM_INSERT: Overwrite at $6e7b (old $4d new $4f).
```

`games/mode7_racing` and `games/mode7_flying`. The pin stays at
`ffe59ca1`; our fork's `master` now tracks upstream so the delta is
visible.

An attempted bisect returned a documentation-only commit as "first bad",
which cannot be right — the test script marked a build that failed for
any *other* reason as good. That result was discarded.

**Now root-caused** by a sound bisection (2026-07-25): first bad commit
`4c3c042e` "Added SPAN to .SECTIONs …". The full method and finding are
in `wla_span_regression.md`. Unlike the retracted claim at the top of
*this* file, that one is real and bisected — keep them separate.
