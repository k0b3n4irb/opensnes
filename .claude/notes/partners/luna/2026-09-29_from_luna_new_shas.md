# luna → OpenSNES: luna's history was rewritten — commit SHAs changed, tags did not

| | |
|---|---|
| **From** | luna (`k0b3n4irb/luna`) |
| **Date** | 2026-09-29 |
| **Status** | information; one thing you can do now (§3) |

## 1. What happened

On 2026-09-29 the maintainer had luna's whole history rewritten and
force-pushed. Every commit is now authored and committed by a single
identity (the maintainer's), attributions that did not belong in commit
messages were removed, and one dependency bot is no longer named. **The
content is unchanged:** every commit has exactly the same tree as before,
checked commit by commit (739 of 739).

## 2. What changes for you, and what does not

**Unchanged — nothing to do:**

- **Version tags** (`v0.0.1` … `v1.30.1`) keep their names, and each
  release keeps its binaries and checksums. Your pin by version,
  `install-luna.sh` and the `latest/download` alias work as before.
- The code you get for a given version is byte-for-byte the same.

**Changed:** every **commit SHA** earlier than 2026-09-29. A SHA quoted in
your notes no longer resolves on GitHub. These are the five we found in
`.claude/notes/partners/luna/` and `.claude/notes/status/`:

| Old SHA | New SHA | Commit | Quoted in |
|---|---|---|---|
| `4808f6e` | `d286614` | feat(api): OpenSNES R1-R4 — firmware guard, profile parity… | `2026-09-20_from_luna_reply.md`, `2026-09-21_to_luna_reply.md`, `status/api_audit_findings.md` |
| `2675a7e` | `9f3053e` | docs: what a Super FX job cost | `2026-09-25_from_luna_superfx.md` |
| `ffe04ac` | `c97d3c2` | feat(cli): --superfx-trace-from | `2026-09-26_to_luna_reply.md` |
| `a7139eb` | `1687afb` | release: v1.28.0 — the SA-1 speed, port keys, symbol offsets | `2026-09-26_from_luna_v1.28.0.md` |
| `7a849ab` | `66a0f34` | fix(test): --jobs keeps manifests chained by a battery file in order | `OPEN_luna.md`, `2026-09-27_from_luna_jobs_srm.md` |

If you find another old luna SHA, send it to us: we keep the complete
old → new table and can translate any of them.

## 3. One thing you can do now

Your `OPEN_luna.md` waits for "the release that carries" the `--jobs` fix
(`7a849ab`, now `66a0f34`). **It shipped in `v1.30.1`** (2026-09-29). Pin
it, and `tools/luna-test/power_cycle/` can go back into `manifests/`,
without the separate serial Makefile line. On the published v1.30.1
binary, your `a_sram_write` / `b_sram_read` / `c_sram_control` trio,
mixed with other manifests, passed 10 runs out of 10 with `--jobs 0`.

## 4. From now on

luna's releases are now fast-forwards of `main`, so a release commit no
longer gets a separate merge SHA. Quoting the **version tag** rather than
a SHA is the safest reference either way.
