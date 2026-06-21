# ASM ABI Lint (Auto-loaded)

CRITICAL: Hand-written ASM in `lib/source/*.asm` is verified at build
time against the C signatures in `lib/include/snes/*.h`. The lint
caught the hdma_gradient retrofit miss from chantier A6.12 *after*
shipping (a user-visible regression). The check now runs in `make
lint` and the `doc-drift` CI job so the same class of bug fails fast
instead of reaching users.

Read this file before adding a new ASM file, retrofitting an existing
one for an ABI change, or pushing a function with a non-standard
calling convention.

## What the lint checks

For every function in `lib/source/*.asm` whose name matches a public
C signature in `lib/include/snes/*.h`:

1. Look up the C signature (`load_signatures`).
2. Compute expected stack offsets per the cc65816 left-to-right ABI:
   - `u8`, `s8`, `u16`, `s16`, `bool` → 2-byte slot
   - `u32`, `s32`, pointer → 4-byte slot (post-A6 chantier)
3. Detect prologue pushes (`php`, `phb`, `phx`, `phy`) and add their
   sizes to the arg base offset (`prologue_shift`).
4. Walk the ASM body. For every `lda N,s ; comment` whose comment
   names a parameter, verify the offset matches the expected slot.

Implementation: `devtools/check_asm_abi.py` → `make lint-asm-abi`.

## The `; lint-asm-abi: skip-file` marker

A small fraction of ASM files use a non-cc65816 calling convention
that the lint cannot verify by construction. The file-level marker
opts the whole file out:

```asm
; lint-asm-abi: skip-file <reason>
```

Place it in a comment within the first 30 lines of the file. The
lint scans line-by-line for the regex `; lint-asm-abi: skip-file\b`,
case-insensitive, and skips every function in the file when found.

Current users (one):

| File          | Reason                          | Doc                                                       |
|---------------|---------------------------------|-----------------------------------------------------------|
| `audio.asm`   | PVSnesLib R-to-L + 1-byte packed| `.claude/notes/tech/audio_legacy_pvsneslib_abi.md`        |

### When to use the marker

- The file uses **PVSnesLib's right-to-left push + 1-byte-packed u8
  arg convention** (legacy modules ported pre-A6, where every u8 is
  packed adjacent to the next u8 on the stack instead of padded to
  a 2-byte slot).
- The file uses a custom calling convention with hand-rolled stack
  manipulation that the lint's prologue model can't capture.

### When NOT to use the marker

- The file just saves an extra register with `phb`/`phx`/`phy` in
  the prologue — the lint accounts for that automatically via
  `prologue_shift`. Don't opt out; let the lint verify the offsets.
- A single function in an otherwise-clean file is broken. Fix the
  function instead of papering over the whole file. There is no
  per-function opt-out by design.
- You don't understand why the lint flags a mismatch. The diagnostic
  names the function, the param, and the slot the offset actually
  hits — read it, then either fix the offset or correct the comment.

### Required when adding a marker

1. **Tech note**: file under `.claude/notes/tech/<topic>.md` with the
   resolution paths (deprecate / shim / rewrite / replace) and a
   recommendation. Cross-link from the header's `@warning` block.
2. **Header `@warning`**: the public-facing C header MUST carry a
   `@warning` block listing which functions work today vs which are
   broken. Users should not discover the issue at runtime.
3. **Marker line**: include a brief reason inline:
   ```asm
   ; lint-asm-abi: skip-file pvsneslib-legacy-cc
   ```

## How the lint detects the prologue

The lint walks each function from its label until the first
stack-relative read, accumulating:

| Instruction | Shift bytes | Notes                                         |
|-------------|-------------|-----------------------------------------------|
| `php`       | base=5      | Sets the base (vs. 4 without PHP)             |
| `phb`       | +1          | Data bank register, 1 byte                    |
| `phd`       | +2          | Direct page register, 2 bytes                 |
| `phx`       | +2          | **Assumes 16-bit X/Y** — the SDK convention   |
| `phy`       | +2          | **Assumes 16-bit X/Y** — the SDK convention   |

Once a `lda N,s` / `sta N,s` / `cmp N,s` / etc. is reached, the
prologue scan stops for that function. Push instructions later in
the body are NOT accounted for — they're not part of the arg-base
offset.

### 8-bit X/Y caveat

If a function pushes `phx`/`phy` while X/Y are in 8-bit mode (after
`sep #$10`), the actual push is 1 byte, not 2. The lint assumes
16-bit (matches the SDK's `rep #$10` default). If the lint flags
mismatches by exactly N bytes after a `phx` or `phy`, check the mode
at the push site. The fix is either (a) move the push to 16-bit
mode (preferred — it's free), or (b) add the skip-file marker if
the convention is intentionally non-standard.

## Anchored claims this lint enforces

| Claim                                                          | Where        |
|----------------------------------------------------------------|--------------|
| Args pushed left-to-right                                      | `compiler/ABI.md`, `CLAUDE.md` critical constraints |
| Post-A6: pointers and `u32` are 4-byte slots                   | `compiler/ABI.md`, A6+A7 chantier merge commit      |
| `php` shifts arg base from 4,s to 5,s                          | `compiler/ABI.md`, runtime.asm                      |
| The hdma_gradient regression class is closed                   | This file + the lint script itself                  |

## Inspecting lint coverage

```sh
python3 -c "
import sys; sys.path.insert(0, 'devtools')
from check_asm_abi import parse_asm, load_signatures, file_is_skipped
from pathlib import Path
sigs = load_signatures(Path('lib/include/snes'))
for asm in sorted(Path('lib/source').glob('*.asm')):
    skipped = file_is_skipped(asm)
    fns = parse_asm(asm)
    has_sig = sum(1 for f in fns if f.name in sigs and not f.pvsneslib_cc and not skipped)
    print(f'{asm.name}: {has_sig} checked, skipped={skipped}')
"
```

Today's totals: 68 functions actively verified, 22 in `audio.asm`
skipped via marker, 128 internal helpers with no public signature.

## When this rule does NOT apply

- `lib/source/audio.asm` itself — the marker is intentional and
  documented (see the tech note).
- `combined.asm` and `*.c.asm` — those are compiler output, not
  hand-written. Verifying them is the compiler test suite's job
  (the C→ASM compiler-pattern checks; these lived in the removed
  `opensnes-emu` submodule and are pending re-homing into the repo — see
  `.claude/notes/chantiers/luna_migration.md`).
- SuperFX `.sfx` sources — GSU has its own ISA and its own ABI.
- `runtime.asm`'s `tcc_*` helpers — internal compiler runtime, called
  by codegen via a different convention.

## Cross-references

- `devtools/check_asm_abi.py` — the implementation.
- `compiler/ABI.md` — the canonical cc65816 calling-convention spec.
- `.claude/notes/tech/audio_legacy_pvsneslib_abi.md` — the legacy
  module the marker exists for.
- `lib/include/snes/audio.h` — the `@warning` block paired with the
  marker.
- Commit `595dc27` (develop) — the shift-aware lint extension.
