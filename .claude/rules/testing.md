# Testing Workflow (Auto-loaded)

IMPORTANT: Every change must pass this workflow before commit.

## Test Command

The test harness runs on **luna** (cycle-accurate native emulator, pinned
binary — no Node/WASM/Mesen2). One-shot via `make tests`, or step by step:

```bash
scripts/install-luna.sh                              # fetch pinned luna (testing/luna.version)
python3 testing/luna_runner.py --coverage    # corpus liveness (NMI/VBlank + CPU state; luna's last_nmi_frame catches an NMI that dies after boot)
python3 testing/luna_runner.py --compare     # visual regression (luna fbhash vs baselines; self-animating examples opt into multiple capture points via manifest.toml `frames = [a, b]`)
make test-manifests                                  # functional probes: `luna test` on testing/manifests/*.toml (scripted input → WRAM asserts)
# ...and phase_sweep.py: the SNESMOD stop/pause/fade manifests replayed at sixteen press phases (a press/SPC700 race shows on some frames only)
# luna_runner.py, rom_coverage.py and wram_regress.py run their luna calls in parallel
# (LUNA_JOBS, default the CPU count; LUNA_JOBS=1 = serial, same output)
python3 testing/difftest.py                 # the compiler against C's integer rules: random expressions run on luna in four shapes, model checked by clang (--seeds A-B to hunt; a Class A change runs a few thousand)
python3 testing/difftest_stmt.py            # the same for small programs: loops, arrays, a struct, pointers, against an interpreter built on that model; a failure is reduced and names the wrong variable
python3 testing/wram_regress.py             # per-frame WRAM oracle over the corpus
python3 testing/luna_runner.py --coverage --power-on random=1   # same liveness pass from pseudo-random RAM (fixed seed): catches reads of never-initialised memory
python3 testing/diff_corpus.py --ref <examples tree built before the change>   # Class A A/B at equal PPU frame (luna diff)
python3 testing/rom_coverage.py              # measured lib API coverage (luna profile --pc-set); never-executed ratchet in baselines/never_executed.txt
python3 testing/audio_regress.py            # APU output hashed for eleven examples (ten audio ones, three pressed with their manifest scripts, and the Super FX skeleton) (luna --audio-out); baselines/audio.json
python3 testing/nmi_budget.py               # VBlank time budget: the NMI handler's worst frame vs a 12 000 mclk ceiling (luna profile --budget) on a representative subset
python3 testing/vram_dma_blank.py           # every VRAM DMA byte of every example lands in blank or force blank (luna --dma-trace); gsuPresent frames whole, double-buffered, swapped in blank
make hardware-preflight                              # before a console session: the 26 protocol ROMs (docs/HARDWARE_VERIFICATION.md) alive from random RAM (3 seeds) and under PAL, VRAM DMA in blank (hardware_preflight.py; ROWS=1-7 for the gate rows; not in make tests)
make test-pal                                        # PAL pass: corpus liveness under --force-region pal + libtest getRegion()/isPAL() + the games playing their manifests under `region = "pal"`, and tetris built ROM_REGION=pal (weekly pal.yml, not in make tests)
make luna-bench                                      # luna's own corpus anomaly scan (nightly luna-bench.yml); only a `bug` verdict fails, `suspect` = static screen
make coverage-host                                   # llvm-cov line coverage of QBE + cproc-qbe over the fixtures and the lib build (report, not a gate)
make docs-strict                                     # Doxygen with warnings as errors (the doc-render job); plain `make docs` stays non-fatal so a doc warning cannot block a release build
```

The host side has its own gate: `make test-sanitizers` rebuilds cproc-qbe,
QBE, wla-dx and the asset tools with ASan + UBSan and runs the fixtures,
the lib build, the tool goldens and the whole corpus under
`halt_on_error=1` (CI job `sanitizers` in `lint.yml`). Run it after any
change to `compiler/` or `tools/*/src`; it leaves sanitized binaries in
`bin/`, so `make clean && make` afterwards.
`make test-toolchain-suites` runs the three submodules' own upstream
suites on the fork binaries against known-fail ratchets
(`devtools/toolchain-suites/`); it is the check for every PIN bump and
runs inside the sanitizer job. A regression or an XPASS fails it.
`make fuzz` runs the libFuzzer harnesses of `tools/fuzz/` (lodepng, the
IT loader, cute_tiled, the aseprite2snes JSON parser, stb_image) for `FUZZ_SECONDS` each; `fuzz.yml` (on any push touching `tools/`, and weekly) gives them ten
minutes, `make fuzz-replay` (in the sanitizer job) replays the committed
crash inputs. `make lint` includes `lint-cppcheck` (tools' sources and lib C; skips
where cppcheck is absent, CI installs it). `make test-link-modules` links every lib module alone with only the
dependencies `make/common.mk` declares (`_DEP_<module>`), and in two
all-together groups; a module that needs a symbol from a module it does
not declare fails here instead of in a user's project. Runs in
`make tests`. A new module or a new cross-module reference must come
with its `_DEP_` line.

The audio oracle is a hash of the WAV, so it flips on a shift of a few CPU
cycles in the code that talks to the SPC700 (the phase, not the sound).
**A commit that re-captures `baselines/audio.json` quotes the output of
`luna diff --audio --align-onset <before>.sfc <after>.sfc --until-frame 300`** (luna
v1.32.0: RMS per 500 ms window, first non-silent sample, MATCH / DIFF at
2 %) for the ROM built before the change against the one after; the hash
stays the guard, the comparison says by how much it moved. A half-volume
module gives 75 % and DIFF; two silent captures give MATCH with
`a=none b=none` on the onset line — read that line for an example meant
to play.
`--align-onset` (luna v1.35.0, pinned 2026-10-10) lines the two captures up
on their first sample above the silence level before cutting the windows,
and prints the shift: a sound that starts a few samples earlier because
the code got faster is then compared with itself (music_large on
2026-10-10: 0.25 % without, 0.09 % with, "onset shift +6 samples"), and a
shift of thousands of samples is a fact to explain, not noise.

The WRAM oracle hashes every WRAM page at each vblank **except the pages of
the plain C band that lie wholly above the ROM's last C variable** — the
stack's region (since 2026-09-26; before, it included the stack and moved on
any change in a library function's size: 20 re-captures in 54 commits,
without ever catching a bug alone). It still moves on codegen changes that
touch the direct page or move globals, and that is the point: it makes you
justify the change rather than notice it three commits later. After an
intentional change, rebuild clean and `wram_regress.py --update` — and say
in the commit why the drift is benign. Precedent: `aa595933` after the
indexed-long fusion, `912fb24a` after the #132 compiler fix. How deep the
stack goes is gated separately (the stack floor of `rom_coverage.py`).

Coverage covers every example (`luna_runner.py --list`). Static analysis
(`symmap.py`), the build (`make`), and compiler C→ASM checks remain separate
make/devtools steps. luna runs SA-1 / Super FX / DSP-1 natively — no chip-ROM
side channel. Migration off snes9x-WASM: `.claude/notes/chantiers/luna_migration.md`.

## Change Classification

| Class | What changed | Required validation |
|-------|-------------|-------------------|
| **A** | Compiler (cproc/qbe/wla-dx) or runtime (crt0, runtime.asm) | `make clean && make` + full `make tests` (luna) on ALL affected examples, **plus** the A/B proof: keep the ROMs built before the change (`rsync -a --include '*/' --include '*.sfc' --exclude '*' examples/ /tmp/examples_before/`) and run `python3 testing/diff_corpus.py --ref /tmp/examples_before [--tolerance N]` — every example must MATCH at its manifest frames (a boot-length offset is reported, a DIFF is a rendering change to explain before any re-baseline), **and** a hunt of the differential test beyond its gate: `python3 testing/difftest.py --seeds 1-2000` and `python3 testing/difftest_stmt.py --seeds 1-2000` (about a minute each; since 2026-10-08) |
| **B** | Library module (lib/source/) | `make lib` + `make tests` covering examples using that module |
| **C** | Single example or new example | Build that example + `make tests` (`luna_runner.py --only <ex>`) |
| **D** | Docs, Makefile, tools only | `make tests` only |

## 2-Pillar Validation

luna unifies what used to be two emulators (snes9x automated + Mesen2 manual),
so validation is now 2 pillars:

1. **luna** (automated) — `make tests` (coverage + visual regression + probes).
   luna is cycle-accurate and runs the chips, so it is both the automated suite
   and the visual reference (no separate manual Mesen2 pass). For deep
   interactive debugging, luna's GUI / MCP (`luna mcp`) is available.
2. **Full rebuild** — `make clean && make` must succeed with zero warnings.

## Before Commit Checklist

1. `make clean && make` — zero warnings
2. `make tests` (luna coverage + visual regression + probes)
3. **Triage impacted examples — short list, not exhaustive dump.** Apply the
   workflow in the next section (Impacted-Examples Triage). The output is a
   smart-selected list with one "what to look for" line per entry, not a wall
   of paths. It goes in the commit message or the report of the lot: it is what
   lets a reader check the change by hand if they want to.
4. **For library changes (Class B)**: grep all example Makefiles for the changed
   module name in LIB_MODULES to enumerate the candidate set, then triage.
5. **The validation is ours, by this protocol; nobody is waited for.** Since
   2026-10-10 (it writes down the mandate the owner gave on 2026-09-22 —
   "je ne valide rien" — and the chief-engineer mandate noted in
   `compiler.md`; recorded as D-004 in
   `~/workspace/snes-tutor/registre/DECISIONS.md` on our recommendation): a change that passed the
   steps above is committed. Do not assume examples work because they
   compiled: the luna visual-regression pass is the visual reference, and
   the triaged subset says what to look at. Until that date this line read
   "NEVER commit without user validation", which the practice had
   contradicted for weeks. **What does wait for a go — the owner's, typed here or
   relayed in his quoted words; nobody else's** (`snes-tutor` decides
   nothing since D-009 of the same day) — is a public or irreversible
   act: a merge to `main`, a tag, a release, closing an issue, writing in
   someone else's repository (`exchanges.md`).
6. Conventional Commits format in message

## Impacted-Examples Triage

For every change, run this three-step pipeline before committing:

1. **Identify** — enumerate every example whose compiled bytes could change,
   not just every file edited. Class A compiler changes touch every example
   in principle; Class B library changes touch the subset that links the
   changed module; Class C example changes touch only one. Use the diagnostic
   tools available (e.g. `CC_TRACE_TCO` for compiler optimisations, `grep
   LIB_MODULES` for library changes, `find -newer` after a rebuild) to get a
   real list, not a guess.

2. **Triage** — collapse the candidate set down to a short list of representative
   examples, dropping redundant coverage. Heuristics for keeping an entry:
   - covers a **distinct code path** that no other entry exercises
     (e.g. the lone `framesize=2 phantom frame` site for a TCO change);
   - exercises a **specific module combination** (audio + DMA + scrolling)
     other entries don't;
   - is a **lib site invoked from many examples** — pick *one* example using
     it, not all of them;
   - lives in **games** (longer interactive paths with input + state machine)
     when the change touches anything around frame timing or state.
   Drop entries that are pure duplicates of a kept one (same module, same
   shape, no extra coverage).

3. **Present** — write the kept list as a small table with one
   "what to look for" per entry. Each line should answer "if this regresses,
   what visible symptom would I expect, and which button or scenario surfaces
   it?" Don't ask the user to fish for bugs blind. Example shape:

   | Example | Why kept | What to look for |
   |---|---|---|
   | `examples/maps/dynamic_map` | exercises 2 framed-tail-call wrappers | press **D** (SNES A) — the map should toggle 32×32 ↔ 64×64 cleanly without flicker |
   | `examples/graphics/effects/mosaic` | only `framesize=2` phantom-frame site in the diagnostic trace | press **A** (SNES Y) — fade-in should complete and stop, no garbage tiles |

   Keep the list to 2–5 entries when possible. If the change genuinely needs
   wider coverage say so explicitly with the reasoning.

**Why triage instead of an exhaustive list:** Class A changes can touch every
example; asking the user to walk through every one is both unrealistic and
wasteful — most are redundant for any given change. The triage forces explicit
reasoning about which axes of coverage actually matter for *this* change, and
the "what to look for" makes the validation an active check (the user knows
the failure shape), not a passive "press buttons and hope".

## Debugging Ported Examples

When a ported example has visual bugs:
1. **Compare with the PVSnesLib original** — build the original PVSnesLib example
   and compare the generated ASM output (stack offsets, VRAM layout, register values)
2. **Check VRAM layout** — sprite tiles, font tiles, and tilemaps must not overlap
3. **Check OBJSEL** — Name Base and Name Select gap determine where tiles 256+ are read
4. **Never guess** — verify actual state with luna (`assets-dump` for
   VRAM/tilemaps/OAM, `state`/`peek_memory` over MCP; docs/tutorials/debugging.md)
