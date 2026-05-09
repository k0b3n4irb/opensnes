# tools/ — Public Tools (Distributed)

These are the **end-user-facing tools** of the OpenSNES SDK, built by
`make tools` and shipped in the per-OS release archives. Every example
Makefile in `examples/` calls into them through `make/common.mk`.

For maintainer-only Python scripts (linters, symbol-map analysis,
benchmark, BRR↔IT conversion, snesdbg), see `devtools/`.

## Available tools

| Path | Role | Used by |
|------|------|---------|
| [`gfx4snes/`](gfx4snes/) | PNG/BMP → SNES tiles, palettes, tilemaps (2bpp / 4bpp / 8bpp; LZ77; tile dedup) | every example with graphics, via `GFXSRC` in `make/common.mk` |
| [`font2snes/`](font2snes/) | PNG → 2bpp / 4bpp font tiles (compiled C version) | text-rendering pipeline |
| [`smconv/`](smconv/) | Impulse Tracker (`.it`) → SNESMOD soundbank for SPC700 | every audio example, via `USE_SNESMOD` |
| [`img2snes/`](img2snes/) | RGB / RGBA PNG → indexed PNG (quantize, BGR555 round, scale) | upstream of `gfx4snes` for assets authored in RGB. No example currently calls it from a Makefile — it is a manual artist-pipeline step run before committing PNGs. |
| [`tmx2snes/`](tmx2snes/) | Tiled (`.tmx`) → SNES tilemap | `examples/maps/tiled` |
| [`sa1-patch/`](sa1-patch/) | Patch SA-1 ROM header byte | post-link step in `make/common.mk` for SA-1 examples |
| [`opensnes-emu/`](opensnes-emu/) | snes9x WASM emulator + 7-phase test harness (separate submodule) | `node test/run-all-tests.mjs` — the project's main validation gate |
| [`debug-fixtures/`](debug-fixtures/) | Test-fixture ROMs (a known-good `clean/` set + an intentionally-broken `broken/` set) | `opensnes-emu` static-analysis phase verifies its detectors flag the broken set and clear the clean set |

## Naming notes

### `font2snes` is in two places

- `tools/font2snes/` — **the production C tool**, distributed in releases.
  This is what `make/common.mk` invokes.
- `devtools/font2snes/` — a Python **reference implementation**, used to
  cross-check the C version's bitplane packing during development. Its
  README labels it as such.

If you are an end user, you want the C version. The Python reference
exists for SDK maintainers debugging conversion edge cases.

## Maintenance artefacts (not distributed, not built)

### `valgrind-static.supp`

A Valgrind suppression file for statically-linked binaries. The file's
header explains what it suppresses and why (false positives from glibc's
static-link tcache initialisation). It is **not wired into any Makefile
or CI workflow** — it is a hand-invoked artefact for maintainers
debugging memory issues in the static C tools (smconv, cproc-qbe). Use:

```bash
valgrind --suppressions=tools/valgrind-static.supp ./bin/smconv ...
```

It lives at `tools/` rather than `devtools/` because the binaries it
suppresses are tools binaries, but it is otherwise maintainer-only.
Removing it would lose the institutional memory of what false positives
to expect from the static-link path.

## See also

- [`devtools/README.md`](../devtools/README.md) — maintainer-only Python
  tooling (symmap, cyclecount, check_mvn, brr2it, font2snes Python ref,
  gen_hud_bar, benchmark, snesdbg, lint_asm, lint_commits,
  verify_toolchain, check_doc_drift).
- [`make/common.mk`](../make/common.mk) — how the per-example Makefiles
  reach into `tools/` for the asset pipeline.
- [`README.md`](../README.md) — top-level project overview.
