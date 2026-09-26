# luna → OpenSNES: what to expect when you move the pin to 1.26.0

Companion to the 2026-09-20 reply. We reviewed `scripts/install-luna.sh` and
the `tools/luna-test/` harness against the changes queued for the tag, so you
know in advance what moves and what does not.

## The install path needs no change

`install-luna.sh` was checked against the **real** v1.25.0 artifacts, not
against the workflow that is supposed to produce them:

- asset `luna-v1.25.0-linux-aarch64.tar.gz` + `.sha256` sidecar — matches the
  script's `luna-${VERSION}-linux-${ARCH}.tar.gz` exactly;
- the sidecar names the bare filename, so `sha256sum -c` works from `$TMP`;
- the tarball unpacks to `luna-v1.25.0-linux-aarch64/luna`, which is the path
  the script installs from.

The names are version-templated, so bumping `luna.version` to the new tag is
the only edit. The `x86_64` / `aarch64` mapping and the `LUNA_BIN` dev
override are unaffected.

## Your JSON parsing is safe

`cpu.sp_min` (new) and `profile`'s `stack` block (new) are additive. Your
harness goes through `json.loads` into plain dicts and reads the keys it
wants, so unknown keys are ignored. Your baselines store derived values —
`fbhash`, an audio SHA-256, WRAM page hashes — not luna's raw JSON, so
nothing there sees the new fields either.

`BUDGET_RE.search()` in `nmi_budget.py` is also safe against the new
`stack:` line that `profile` prints: it is a search, not a line index. The
`--report` fallback that looks for a row ending in `NmiHandler` is safe too —
the stack line always ends in `)`.

## One hazard, and it is in your stated plan

`nmi_budget.py` derives its verdict from the exit code alone:

```python
verdict = "OVER" if proc.returncode == 1 else "ok"
```

`--stack-floor` also exits **1**. Nothing breaks today, because you do not
pass it. But your reply says you will "add a measured gate next to it" — and
if both gates run in the *same* `profile` invocation, a stack failure will be
reported as `NmiHandler OVER`, which would send you looking at the wrong
thing.

Two ways out, both fine:

- run the two gates as separate `profile` invocations (simplest, and the
  stack gate does not need `--budget`'s symbol); or
- keep one invocation and read the lines rather than the code — they are
  unambiguous:
  ```
  budget: NmiHandler max 6512 mclk (frame 402) > 6000 — OVER
  stack: deepest S $1C23 at $00CB5B (AudioDriverBoot, frame 4) < $1C60 — UNDER
  ```

We kept exit 1 for both deliberately: "a gate failed" is one condition, and
splitting the code would have made `--stack-floor` behave unlike `--budget`
for no gain. If you would rather have a distinct code, say so — it is a
one-line change and yours is the only harness affected.

## Baselines that will move, and why

You already plan to re-run the whole suite on the bump. This is so you can
tell an expected shift from a regression.

**`baselines.json` (`fbhash`) — expect movement on timing-sensitive ROMs.**
The tag carries a 65C816 fix: NMI/IRQ are now sampled one cycle before an
instruction's last bus access, where ares and Mesen2 sample them, instead of
at the instruction boundary. Delivery moves by at most one instruction, but
that is enough to land an animation one step away. Our own Star Fox golden
moved for exactly this reason and was re-recorded after checking the frame
was still correct; all 16 hardware-reference PPU tests stayed pixel-exact,
and 90 of 91 goldens were byte-identical.

**`audio.json` (SHA-256) — same class as the shift you already met.** You
described a boot path 125 µs longer producing an identical signal four
samples later. The interrupt change is that kind of change. If a hash moves,
align the two WAVs before concluding anything — which is precisely the tool
you scoped in your reply, and which does not exist yet.

**`wram.json` — should hold.** WRAM at vblank is one logic step per NMI, so
it is insensitive to this class of timing change unless a fixture's per-frame
work is itself cycle-budgeted.

## `$4016` / `$4017`: read the bits, not the byte

You asked us to tie `$4017` bits 2-4 high. We closed the wider gap in the
same commit, because fixing only your half would have swapped one deviation
for another: luna used to return a bare `0` or `1` with **every other bit
zero**, and neither reference does that. Now:

| | bits 0-1 | bits 2-4 | bits 5-7 |
|---|---|---|---|
| `$4016` | the port's data lines | open bus | open bus |
| `$4017` | the port's data lines | **always 1** | open bus |

We checked your tree: `REG_JOYA` / `REG_JOYB` are declared in
`registers.h` but read nowhere in `lib/` or `examples/` today, so no existing
OpenSNES code is affected.

**But your R4 plan is to start reading them** — the 17th clock in `crt0`'s
NMI handler. Both are `vu8`, i.e. whole-byte reads, so:

- `if (REG_JOYB)` will be **always true**, on every console and every
  emulator, because bits 2-4 are tied high. That is not a luna artefact.
- mask before testing: `REG_JOYA & 1` for port 1's data line, `REG_JOYB & 1`
  for port 2's, or `& 3` if you want d1 as well (the multitap's second line).

This is the single most likely way the change could cost you an afternoon, so
it is worth putting in the header comment next to `padIsConnected()` along
with the note you already planned about auto-read.
