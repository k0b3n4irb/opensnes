# Bank $00 ROM Budget (Auto-loaded)

CRITICAL: Bank $00 ROM overflow is a silent-failure 🔴 documented in
`KNOWN_LIMITATIONS.md`. The compiler emits 16-bit addresses that always
read bank $00 **for a C dereference**, so a `static const` array (string
literals, LUTs, C-indexed data) that is read by C code (`arr[i]`, `*p`) and
spills past the 32 KB bank-$00 boundary is read as **garbage** at runtime —
no error, no warning, just wrong data. (Data merely *passed* to a lib
DMA/asset function — tiles/maps/palettes/fonts via `dmaCopyVram` &
friends — carries its bank in the 4-byte pointer and works in any bank; see
`KNOWN_LIMITATIONS.md`. The ratchet still guards the C-deref + string-literal
class, which remains the silent failure.)

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

Spill detection (since 2026-07-07) is **section-based**: the checker parses
the .sym `[sections]` block and hard-fails any `.rodata.N` section (QBE's
per-datum C const emission) placed in bank $01+, naming the contained
symbols. This covers named top-level statics, which the older
`string.N`/`name.N` symbol heuristics missed — likemario's anim clips
spilled right through the ratchet and shipped a dead animation before
this. Sections holding only `__opensnes_force_emit_*` anchors are exempt
(linker-only data, never C-deref'd).

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

## Keeping assets out of bank $00 in the first place (since 2026-07-22)

Most bank-$00 pressure is not code — it is payload that never needed to
be there. `.SECTION … SUPERFREE` lets the linker choose, and it chooses
the first bank that fits, which is bank $00. `examples/games/rpg` sat at
**12 free bytes** with 12 KB of map data parked in the code bank.

Assets do not need bank $00:

- anything handed to a lib DMA function (tiles, tilemaps, palettes,
  fonts) travels as a **far pointer** — `dmaCopyVram` and friends read
  the bank byte;
- anything C reads through a **`const`** pointer is a far read (#121).

Only const data that C dereferences through a *non*-const pointer must
stay. So declare payload with the macro from `templates/assets.inc`,
included automatically in every assembled file:

```asm
ASSET_SECTION "townmap"
town_map:    .incbin "res/town_map.bin"
.ENDS
```

You do not pick a bank. The macro uses `SEMISUPERFREE BANKS 7-1`, so the
linker tries the highest bank first and walks down; bank $00 is simply
not a candidate. The RPG went from 12 to **9393** free bytes in bank $00
this way, with a pixel-identical ROM.

The list is literal because it must be: a `.DEFINE` is not expanded
inside a `BANKS` clause (wlalink: "malformed BANKS list") and a bank
outside the memory map is a hard link error ("out of range [0, 8]").
Banks 1-7 exist in every ROM memory map the SDK ships.

Every link now also prints how much declared payload ended up in bank
$00 (`report_bank0_asset_payload` in `symmap.py`). It matches section
names exactly — `.rodata*` from QBE and `asset.*` from the macro —
because an earlier heuristic version flagged `.text.bgSetMapPtr` for
containing "map", and a report you cannot trust is worse than none.
Sections that opt out of the macro are simply not counted.

### What still blocks making it the default

Issue #127's remaining piece is non-bank-$00 placement for C const data
(`.rodata.N`, emitted by QBE) without an opt-in. Measured 2026-07-22, it
needs one of:

- **a wla-dx change** — `SUPERFREE` searches banks ascending, so bank
  $00 wins by construction. A search-order flag (or reversing it for
  data sections) would flip the default with no per-project setup. This
  is the clean answer and the only one that needs no knowledge of the
  ROM layout at compile time. `compiler/wla-dx` currently carries **0
  local patches**, so this is a deliberate fork decision, not a drive-by;
- **or a generated per-layout macro** — QBE cannot emit `BANKS <list>`
  because it does not know the bank count, and the list can be neither
  a `.DEFINE` nor over-range. `make/common.mk` already generates
  `project_config.inc`; it could generate the section-declaration macro
  with a literal list matching the memory map in use. Workable, but it
  moves ROM-layout knowledge into the build system.

The 2026-05-14 attempt (`.SECTION X BANK 1 FREE` in qbe `emitdat`) failed
for a different reason — a single hardcoded bank, which the audio
examples fill with sample data. `SEMISUPERFREE`'s fallback list is what
that attempt was missing.

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

## The RAM twin: C RAM band budget (since 2026-07-11)

The same silent-failure family exists for RAM: all C-accessible RAM must
sit in `$00:0000-$1FFF` (the 8 KB WRAM mirror) — anything higher is
silently wrong-banked by the compiler's `sta.l $0000,x` addressing
(structural defect B2). `make/common.mk` runs
`symmap.py --check-ram-budget` after every link:

| Stage | Trigger | Outcome |
|-------|---------|---------|
| 🔴 Hard fail | a bank-$00 RAM section crosses or sits past `$2000` | build fails (exit 1) |
| 🔴 Hard fail | free space < `RAM_FAIL_THRESHOLD` (default **512 bytes**) | build fails (exit 1) |
| 🟡 Soft warn | free space < `RAM_WARN_THRESHOLD` (default **1024**) | warning + largest-sections list, build continues |

The free-byte count prints at every link — this is the *instrument* that
turns "will the game hit the 8 KB RAM ceiling?" into a tracked number
(the decision gate for scheduling the B2 chantier). Corpus baseline at
introduction: breakout 668 bytes free, tetris 928 — warnings on those
two are deliberate, they really are within 1 KB of the ceiling.
`SKIP_RAM_CHECK=1` bypasses; the same ratchet discipline applies (never
raise the fail threshold above the corpus minimum without a refactor).

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
