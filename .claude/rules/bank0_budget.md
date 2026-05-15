# Bank $00 ROM Budget (Auto-loaded)

CRITICAL: Bank $00 ROM overflow is a silent-failure 🔴 documented in
`KNOWN_LIMITATIONS.md`. The compiler emits 16-bit addresses that always
read bank $00, so any `static const` array (string literals, LUTs, asset
data) that spills past the 32 KB bank-$00 boundary is read as **garbage**
at runtime. No error, no warning at runtime — just wrong data.

The build system enforces a two-step ratchet on this. Read this file before
adding const data, refactoring an example, or tuning the threshold.

## The two-step ratchet

| Stage | Check | Trigger | Outcome |
|-------|-------|---------|---------|
| 🔴 Hard fail | `static const` actually spilled to bank $01+ | spill happened at link | build fails (exit 1) |
| 🔴 Hard fail | bank $00 free space < `BANK0_FAIL_THRESHOLD` (default **16 bytes**) | imminent — one more const literal will overflow | build fails (exit 1) |
| 🟡 Soft warn | bank $00 free space < `--warn-threshold` (default 2048 bytes) | comfort margin gone | warning printed, build continues |

Implementation: `devtools/symmap/symmap.py` (`print_bank0_overflow_check`)
+ `make/common.mk` (the post-link check after every `.sfc` build).

## Default threshold (8 bytes on wip/a6-a7-atomic-v3; 16 on develop)

The threshold is **always set just below the current example minimum**
so the next const literal that lands somewhere in that margin fails
fast rather than producing a silently broken ROM. The current
minimum drifts with chantier work; the threshold tracks it.

| State                          | Min free | Threshold |
|--------------------------------|----------|-----------|
| v0.16.0 (`mapscroll.sfc`)      | 28 bytes | 16        |
| v0.18.0 (post-inline retrofit) | 28 bytes | 16        |
| wip/a6-a7-atomic-v3 (post-A6)  | 12 bytes | 8         |

The 12-byte minimum on the A6+A7 chantier branch is structural:
post-A6 pointer args push `pea.w :sym` *plus* `pea.w sym` (4 bytes
of ROM) at every call site instead of one `pea.w sym` (2 bytes
pre-A6). The 3 affected examples (likemario, tetris, mapandobjects)
have many lib-call sites in their main TUs. Re-tightening to 16
requires either: (a) lib code-size optimisations that recover the
4 bytes back per call; or (b) routing the canonical force-emit
anchors out of bank $00 — first attempted 2026-05-14 via
`.SECTION X BANK 1 FREE` in qbe `emitdat`, abandoned because audio
examples have bank 1 packed solid with SPC sample data; FREE BANK 1
hard-fails to fit. A robust scheme needs multi-bank fallback or a
SUPERFREE name-grouping trick — left for a dedicated chantier.

This is a **ratchet**: never RAISE the threshold (= weaken the gate)
unless the current build's actual minimum dropped below it. The drop
from 16 → 8 on wip is justified by the documented post-A6 minimum;
the goal is to claw it back to 16 once one of the recovery paths
above lands.

## When to bump `BANK0_FAIL_THRESHOLD` tighter

Bumping the default tighter (say 64 → 128 → 256) is a **deliberate audit
step**, not an incremental commit. Procedure:

1. Run `make` and list every example with `(N bytes free)` warnings under
   the new threshold.
2. For each, refactor: combine related const arrays into a single array
   with offset macros (the canonical pattern, see
   `KNOWN_LIMITATIONS.md:68-72`); move large data to RAM (drop the
   `const`); or use assembly with explicit bank addressing.
3. Validate the refactor with `make clean && make` + the full
   `--quick` test suite — heavy const moves can shift VRAM/WRAM
   addresses.
4. Bump `BANK0_FAIL_THRESHOLD` in `make/common.mk` *and* this file in
   the same commit.

## When to bump it looser

Almost never. If a chantier needs more headroom on a specific example,
override per-example via the example's Makefile:

```makefile
BANK0_FAIL_THRESHOLD := 0   # disabled for this build only
```

…or pass `make BANK0_FAIL_THRESHOLD=0`. This is for short-term debugging
only — the chantier should refactor const data before merging.

## Per-example breakdown (run today)

```sh
for sym in $(find examples -name '*.sym'); do
    free=$(python3 devtools/symmap/symmap.py --check-bank0-overflow "$sym" \
           2>&1 | grep -oE '\(([0-9]+) bytes free' | head -1 | grep -oE '[0-9]+')
    [ -n "$free" ] && echo "$free $sym"
done | sort -n
```

Read the bottom of the list. Anything below 100 bytes is a candidate for
the next refactor wave.

## Why this rule exists

A 1-day external review on 2026-05-07 flagged that 12 examples were
within 71 bytes of bank $00 ROM overflow with the existing 2 KB warn
threshold. The hard-fail path existed for actual spills but **not for
imminent ones**. Ratchet added in the same review session.

## When this rule does NOT apply

- Examples that don't link C code (assembly-only ROMs are not subject
  to the const-spill class).
- SuperFX `.sfx.bin` builds (GSU is its own ROM space, separate from
  the 65816 bank $00).
- Debug-mode builds explicitly using `SKIP_BANK0_CHECK=1` (which already
  bypasses the entire check). If you set it, document why in the commit.

## Cross-references

- `KNOWN_LIMITATIONS.md` (bank $00 ROM overflow entry, severity 🟢)
- `make/common.mk` (the wiring)
- `devtools/symmap/symmap.py` (the check)
- `.github/workflows/opensnes_build.yml` (the CI gate)
