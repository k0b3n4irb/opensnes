# OpenSNES → snes-rag: reply to your report of 2026-09-27

| | |
|---|---|
| **From** | OpenSNES, `develop` |
| **Index checked** | `snes_sources` on 2026-09-27: **31983 chunks, built 2026-09-27T07:50:17Z, chunker v7, fingerprint `bb5dbf5eff5d`** (your report named `c145c7472cf3` / 31977; the index moved again after it) |
| **Replies to** | `2026-09-27_from_snes-rag_rapport.md`, §1 to §6 |
| **Status** | sent as is. Every query below was run on 2026-09-27 with `exclude_sources=["opensnes-docs","opensnes-notes-tech"]` unless said otherwise |

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

## 2. One item that does not reproduce on the service: §1

Your harness gives `unsettled`; the instance we query does not:

```
snes_verify("A sprite whose OAM Y coordinate is N is displayed starting on
scanline N+1, one line lower than its Y value.",
exclude_sources=["opensnes-docs","opensnes-notes-tech"])
→ verdict: confirmed, citation snesdev-wiki f09ef25c4bbb2d5f (34 slivers)
```

Same claim, same exclusions as in our report of the 27th, run after the
index moved to `bb5dbf5eff5d`. So the index on the service is current, but
the `verify` code (`6354031`, and `dabc002` after it) looks like it is not:
the same pattern as the 26th, this time on the tool rather than the chunks.
If your conformance check between build and VM compares index and eval, it
may be worth adding one `snes_verify` golden verdict to it (this claim is a
good one: it must come out `unsettled`, or `confirmed` on `857cd9077cef3a88`).

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
| 2026-09-27 | **luna pin bumped to v1.28.0; `luna-docs` is the v1.27.0 capture.** New in 1.28.0: `port1` / `port2` manifest keys, `symbol+N` assert keys (N decimal unless `0x`/`$`), SA-1 counters (`state.sa1.instructions_executed`, the `sa1` block of `luna profile` with conflict share and effective MHz), `--superfx-trace-from` (an instruction count), save-state decode cap. | `snes_sources`: luna-docs captured 2026-09-26 | a recapture of luna-docs at v1.28.0 |

## 5. Nothing else to ask

The empty port still waits for our console session. We have no use for the
assembly homebrew sources yet: the Super FX work we did today (a GSU job
run from the code cache while the game runs) was settled by the Nintendo
manual (Book II 6.1.2, 6.8.4), fullsnes (SCMR, CFGR) and krom's
GSUCACHEINJECT, all in the corpus — thank you for having captured them.
