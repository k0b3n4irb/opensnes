# #127 proposal 3 — default non-bank-$00 placement for QBE const data

Status: **measured, not shipped.** The one-line default-flip is safe for
LoROM and SA-1 and **corrupts HiROM**. Reverted. This note is so the next
attempt starts from the HiROM problem instead of re-running the safe half.

## The experiment

`compiler/qbe/emit.c` emits C const data as `.SECTION ".rodata.%d"
SUPERFREE` at two sites (≈line 155 and ≈line 311). SUPERFREE picks the
first bank that fits, which is bank $00 — the code bank. The flip:

```diff
-  fprintf(f, ".SECTION \".rodata.%d\" SUPERFREE\n", …);
+  fprintf(f, ".SECTION \".rodata.%d\" SEMISUPERFREE BANKS ASSET_BANKS\n", …);
```

`ASSET_BANKS` is `.DEFINE`d as `"7-1"` in the three C-targeted memory
maps (`memmap.inc`, `memmap_hirom.inc`, `memmap_sa1.inc`), and
`wrap_asm` includes the memmap before the `.c.asm` body, so the symbol
is in scope. This is the same mechanism `ASSET_SECTION` already uses
(commit `6943eccb`), extended from opt-in payload to compiler default.

**Class A — the intermediates must be rebuilt.** `make examples` alone
does not re-run cc65816 when the `.c` is unchanged, so a compiler change
needs `make clean && make`. First run of this experiment measured a
stale corpus and read as "nothing moved"; the flip had not reached any
`.c.asm`. Don't repeat that.

## Result

Clean rebuild, 257 QBE `.rodata.N` sections flipped across the corpus.

- **bank-blind lint: 74/74 clean.** No C-dereferenced const data was
  wrongly moved — after the #121 const audit, C reads of const data are
  far reads, and anything read near (string literals, non-const derefs)
  either stayed put or the lint would have hard-failed. It did not.
- **LoROM / SA-1: safe.** tetris and likemario freed bank $00 from 12 to
  **71 bytes**. Their remaining bank-$00 payload (`.rodata1`, `.sm_spc`)
  is *hand-written* asm — tetris's own `data.asm`, snesmod's driver —
  which a compiler flip cannot and should not touch. That is the honest
  ceiling of this approach: it moves compiler-emitted const data only.
- **HiROM: broken.** Both HiROM examples corrupt —
  `memory_hirom_demo` (font/tilemap garbled, text barely legible) and
  `audio_snesmod_music_hirom`. Coverage and probes still pass, so it is
  a data-placement corruption, not a crash. Reverting the flip and doing
  a full clean rebuild restores both, isolating the flip as the sole
  cause.

## Why HiROM breaks — corrected, and bounded to what was verified

First guess was "far access to high HiROM banks is broken." **Wrong, and
disproven by a working example:** `audio/snesmod_music_hirom` places
`SOUNDBANK0` at `01:8000` and `SOUNDBANK1` at `02:8000` and plays
correctly. HiROM *can* hold and read data outside bank $00.

Second guess was "any non-$00 bank." Also wrong — bank 1 corrupts too
(`.rodata.1` at `01:0000`, tested with `ASSET_BANKS "1"`), so it is not a
high-bank/mirror-only effect.

What actually distinguishes the working case from the broken one:

| | SOUNDBANK (works) | flipped `.rodata` (breaks) |
|---|---|---|
| section type | `FORCE` | `SEMISUPERFREE` |
| offset in bank | `$8000` | `$0000` |
| accessed by | snesmod's hand-written far reads | `dmaCopyVram` generic far pointer |

HiROM maps ROM banks $00-$3F to only the **upper half** ($8000-$FFFF) of
each ROM bank; the full 64 KB window is at $C0-$FF. Data placed at
**offset $0000** in a low linker bank falls in the half that the
$00-$3F view does not map. `SOUNDBANK` sits at $8000 and is fine; the
SEMISUPERFREE `.rodata` landed at $0000 and is not. Whether the true
culprit is the offset, the `:sym` bank-byte resolution for HiROM, or
`dmaCopyVram`'s bank handling was **not** isolated further — the three
move together in this test and separating them is the next step.

The structural reason it can't be papered over in QBE: **QBE emits one
`.c.asm` linked against whichever memmap the example chooses.** It cannot
emit HiROM-aware directives; the layout knowledge lives in `ASSET_BANKS`,
a per-memmap `.DEFINE`. But a memmap `.DEFINE` cannot express "offset
$8000+" — that is a `.SECTION` placement concern — so the fix is likely
deeper than the bank list.

## Next step (for whoever picks this up)

1. Find which bank range far-reads correctly in HiROM. Candidates:
   `ASSET_BANKS` for HiROM may need different values, or the far-read
   bank-byte resolution for high HiROM banks is itself wrong (a compiler
   or linker question, not a memmap one). Test `memory_hirom_demo` in
   isolation against a few `ASSET_BANKS` values before touching QBE.
2. **`ASSET_SECTION` has the same latent HiROM exposure** and I shipped
   it (`aeabdad3`): it uses the identical `SEMISUPERFREE BANKS 7-1`
   directive, so on HiROM its payload would land at offset $0000 in a
   low bank — the broken case above. No shipped example exercises it
   (the RPG and every ASSET_SECTION user is LoROM), so it is latent, not
   live. A `@warning`-equivalent note is now in `templates/assets.inc`.
   The proper fix (force $8000+ offset on HiROM, or resolve the far
   access) is part of this chantier, not a separate one.
3. Only then flip the QBE default, with HiROM green.

The LoROM/SA-1 half is proven and costs nothing to re-confirm; do not
re-derive it. The whole of the remaining work is HiROM.

## What shipped from this session

Nothing in the compiler. The reusable BANKS-`.DEFINE` mechanism
(`6943eccb`) and `ASSET_SECTION` picking its own bank (`aeabdad3`) are in
`develop`; this default-flip is not. #127 stays open on HiROM.
