# Clock skew on this machine defeats mtime-based guards

Incident 2026-07-18, during the apuReset chantier. `make` prints
`Warning: File '...' has modification time 109054343 s in the future`
(~3.5 years) for files under `compiler/` submodule build dirs. On a
machine with such future-mtime files:

- **Incremental `make` is unreliable**: an example ROM with a future
  mtime is "newer" than a freshly rebuilt lib object, so it never
  relinks. Concretely: after the apu.asm change, the local tree kept
  pre-change `pitch_mod`/`play_noise`/`speech_synth` ROMs, the local
  suite passed against stale baselines, and CI (clean build) failed
  the WRAM regression on all three. Local rebuild-from-clean then
  reproduced CI's hashes exactly.
- **The mtime guards can be silently defeated too**: corpus-fresh
  (#105) and per-example stale-source (#120) both compare mtimes, so
  a poisoned tree can look "fresh".

Rule of thumb reinforced: any time `make` prints clock-skew warnings,
treat every incremental result as suspect — `make clean && make`
before capturing baselines or trusting a green suite. (The dependency
chain in make/common.mk itself is correct: `linkfile: $(LINK_OBJS)`
includes the lib objects — verified during this incident.)

## Root cause (found 2026-09-15, gaps review D9)

`find . -newermt 2027-01-01` listed **1802 files**, every one under the
`compiler/{cproc,qbe,wla-dx}` submodule trees, dated 2029–2030. Nothing
in the repo writes such dates today; the source is the retired CI cache
trick that stamped submodule build trees with `touch -d 2030` so a
restored cache would never rebuild — the stamps came home through the
working tree and survived every `make clean` (which removes objects,
not sources). The machine clock was never wrong.

Fixed locally by resetting the dates (`find . -path ./.git -prune -o
-newermt 2027-01-01 -print0 | xargs -0 touch`) — the count is 0 and
`make` prints no skew warning. The guards are also less naive since
2026-09-13: `devtools/check_corpus_fresh.py` compares the lib objects
and the ROMs against the *toolchain build-tree* timestamps
(`compiler/qbe/qbe`, `compiler/cproc/cproc-qbe`,
`compiler/wla-dx/binaries/*`), not against `bin/`, which every
top-level `make` re-copies. A poisoned tree still shows up as a
"file has modification time … in the future" line from make: treat it
as a stop sign and run the `touch` above before anything incremental.

**Status: CLOSED** — cause identified, local tree clean, guards
hardened. Reopen only if a fresh clone reproduces the warning.
