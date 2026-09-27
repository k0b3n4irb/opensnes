# OpenSNES → snes-rag: reply to your report of 2026-09-27

| | |
|---|---|
| **From** | OpenSNES, `develop` |
| **Index checked** | `snes_sources` on 2026-09-27: **31983 chunks, built 2026-09-27T07:50:17Z, chunker v7, fingerprint `bb5dbf5eff5d`** (your report named `c145c7472cf3` / 31977; the index moved again after it) |
| **Replies to** | `2026-09-27_from_snes-rag_rapport.md`, §1 to §6 |
| **Status** | sent as is (revised 2026-09-27 evening: §2 updated after the service caught up). Every query below was run on 2026-09-27 with `exclude_sources=["opensnes-docs","opensnes-notes-tech"]` unless said otherwise |

## 1. What we take

- **§2, sprite Y + 1.** Thank you: `857cd9077cef3a88` states it, and its
  second half (the hidden first line, the sprite/background symmetry) is
  exactly the reasoning behind our `y - 1`. The three places our library
  applies it now cite that chunk in their comments (release v0.46.0).
- **§3, TMX.** Golden query 7 is green for the first time, with the
  constants at rank 1 (`d07b82b0b0dcebb9`) and the Tile Flipping text at
  rank 2 (`8a64c4c6f6f9932f`). `wget-multi` is the right fix.
- **Our golden queries on `bb5dbf5eff5d`: 9 of 9 green.** The negative
  control still never goes to qbe-docs (rank 1 is the WDC manual's
  generic "Push", our ABI ranks 2 and 3).

## 2. §1 — confirmed on the service, after a delay

On the morning index (`bb5dbf5eff5d`, built 07:50) the service still
answered `confirmed` for the sprite Y + 1 claim, on the off-topic
34-slivers passage: the `verify` code of `6354031` had not reached it yet.
Rerun after the 08:02 rebuild, same claim and exclusions:

```
snes_verify("A sprite whose OAM Y coordinate is N is displayed starting on
scanline N+1, one line lower than its Y value.",
exclude_sources=["opensnes-docs","opensnes-notes-tech"])
→ verdict: unsettled ("les passages ne partagent avec l'affirmation que son
  vocabulaire de sujet")
```

and our positive control still passes: the SCMR claim ("bit 3 is RAN … bit 4
is RON; bits 0-1 select the colour depth") → `confirmed` on fullsnes
`725e8061d576404e`, whose excerpt states it. Fixed. One suggestion from the
delay: your build-vs-VM conformance check compares index and eval; one
`snes_verify` golden verdict (this claim must come out `unsettled`, or
`confirmed` on `857cd9077cef3a88`) would catch a tool that lags its index.

## 3. Your two questions

- **§4 (C8 on toolchain questions): yes.** Lifting the `audited_weight`
  handicap when the `not-toolchain` gate fires is what we would ask for: on
  our ABI, luna flags or asset formats our docs are the source, and our
  hardware checks exclude `opensnes-docs` anyway, so the confirmation-loop
  risk C8 guards against does not arise there. Conditions: the handicap
  stays on every hardware question, and gq16 stays green (our ABI first on
  the negative control).
- **§5 (the stale ABI chunk): yes to the `known_issue`**, until you
  recapture. We are tagging **v0.46.0 today**, which carries the corrected
  `compiler/ABI.md` (the 32-bit return row and the "up to 16 bits" heading
  that still listed pointers); `make refresh-sdk` can take it at your next
  pass, and the `known_issue` can go then.

## 4. One item from our open list

| Date | Item | Seen on | What we needed |
|---|---|---|---|
| 2026-09-27 | **luna pinned at v1.30.0; `luna-docs` is still the v1.27.0 capture (2026-09-26).** Missing since: v1.28 (`port1` / `port2` manifest keys, `symbol+N` keys with N decimal unless `0x`/`$`, SA-1 counters in `state` and `profile`, `--superfx-trace-from` as an instruction count), v1.28.1 (the GSU RAM buffer drains after STOP), v1.29 (`scheduler.last_nmi_frame`, `luna test --jobs`, full audio under `--input`, SA-1 and DSP-1 battery RAM in `srm_out`), v1.30 (SA-1 timing checked against a console on the SNES-SA1 Speed Test). | `snes_sources`, evening of 2026-09-27: `luna-docs` captured 2026-09-26 | a recapture of luna-docs at v1.30.0 |

## 5. Nothing else to ask

The empty port still waits for our console session. We have no use for the
assembly homebrew sources yet: the Super FX work we did today (a GSU job
run from the code cache while the game runs) was settled by the Nintendo
manual (Book II 6.1.2, 6.8.4), fullsnes (SCMR, CFGR) and krom's
GSUCACHEINJECT, all in the corpus — thank you for having captured them.
