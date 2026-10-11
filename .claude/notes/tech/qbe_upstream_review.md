# QBE upstream, read against our fork (2026-10-11)

Our backend is a fork of QBE (Quentin Carbonneaux, `git://c9x.me/qbe.git`).
The fork's root is a squash, so git knows no merge-base; `compiler/PINS.md`
locates it at upstream `120f316` (2025-05-30). This note reviews what
upstream did since. Twin of `cproc_upstream_review.md`.

## What was read

41 upstream commits after `120f316`, up to `e786f06` (the tip fetched on
2026-10-11). Each was tried alone on our tree: 20 apply, 21 conflict. That
count says little: most are for targets we do not build for.

Our pipeline (`main.c`, `func()` and `finalize_and_emit()`) runs promote,
ssa, loadopt, coalesce, the inliner, threadjnz, gvn (with `copy.c`),
simpljnz, gcm, simpl, then the w65816 isel and emitter. It skips `spill`
and `rega`. Only commits in those shared passes can matter.

| Upstream | What | For us |
|---|---|---|
| `b58e2e6` exponential `usewidthle()` | compile time | already carried (`77998b5`) |
| `e01207f` exponential `defwidthle()` | compile time, **and it changes the phi rule** | **not taken, and wrong upstream** — see below. The correct half (a phi being proven at width w0 answers yes only for a question at w0 or wider) is what we fixed ourselves on 2026-10-10 (`79ff312`), found by `difftest_stmt` |
| `ade9f31` more phis are copies (`phi(a, self)`) | optimisation, applies alone | not measured yet; a Class A lot of its own if tried |
| `3cfca4b` fewer temps in `loadopt()`, `6fa30d5` its range fix | compile time; the second fixes a function the first adds | not needed; if `3cfca4b` is ever taken, `6fa30d5` comes with it |
| `afd5d2e` drop dead predecessors in `fixphis` | "odd but probably harmless" phi arguments (its own words) | not taken; no symptom here |
| `8ff0651` `-` followed by a non-digit in the IL | parser robustness | not taken: our IL comes from cproc |
| `fbe3cea`, `c081897` no 80-character limit on names | conflicts | not taken. Here a C name of 80 characters or more stops the build with "identifier too long", naming a temporary IL file and not the C line: loud, poorly said |
| `6a2dca8`, `2ccbf47`, `1366e70` | `spill.c`, `rega.c` | we skip both passes |
| `5c1eb24`, `c633655`, `e8365dd`, `640c78d` if-conversion, `simplcfg` | x86 `cmov`, a new CFG pass | conflict; our own passes (`threadjnz`, `simpljnz`) cover that ground |
| the rest | amd64, arm64, rv64, Windows ABI, Mach-O, cosmetics | not our target |

Nothing upstream since our base fixes a defect we have.

## A miscompilation in upstream's current tip, which we do not have

`e01207f` (2026-05-05) made `defwidthle()` remember, for the length of one
query, the phis it has started proving (`p->visit = w`, cleared only when
the whole query ends). A phi whose proof **failed** keeps its mark, and the
next path of the same query that reaches it takes it for proven. With
`and`, whose rule is "either operand narrow":

```
export function w $f(w %a, w %c) {
@start
	jnz %c, @l, @r
@l
	jmp @j
@r
	jmp @j
@j
	%p =w phi @l %a, @r 1
	jnz %c, @m, @n
@m
	jmp @k
@n
	jmp @k
@k
	%s =w phi @m %p, @n 3
	%q =w and %p, %s
	%z =w extub %q
	ret %z
}
```

`%p` is not narrow (it can be `%a`); its failed proof leaves the mark;
`%s` then "is narrow" through it; `%q` is taken for 8 bits and the `extub`
is removed. Run on this machine (aarch64), with a C `main`:

| QBE | `f(0x1234, 1)` |
|---|---|
| upstream `e01207f~1` | 0x34 |
| upstream `e01207f` | 0x1234 |
| upstream tip `e786f06` | 0x1234 |
| our fork, `-t arm64` | 0x34 |

Our `defwidthle()` clears the mark when a proof ends, failed or not. It may
therefore still cost exponential time on a deep diamond of phis — never
seen; if it is, the memo must keep proven phis only.

Reporting this upstream is a public act: a draft is in
`.claude/notes/partners/qbe-upstream/`, and it waits for the owner's word.

## Redo it

```sh
git clone git://c9x.me/qbe.git /tmp/qbe-upstream
git -C /tmp/qbe-upstream log --oneline 120f316..master
```
