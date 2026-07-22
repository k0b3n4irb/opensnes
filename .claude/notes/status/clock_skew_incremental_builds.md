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

Root cause of the future mtimes not yet chased (submodule checkout or
an earlier wrong system clock). If the warnings persist after a fresh
clone, investigate the machine clock / filesystem timestamps.
