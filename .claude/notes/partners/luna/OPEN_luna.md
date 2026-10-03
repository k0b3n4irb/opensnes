# OpenSNES → luna: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item. Every item is re-checked on the pinned
luna the day the report goes out (`.claude/rules/partners.md`); the pin is
v1.31.0 (2026-10-03).

| Date | Item | Seen on | What we would run |
|---|---|---|---|
| - | (empty: the audio comparison request went out on 2026-10-03 in `2026-10-03_to_luna_messages-region_reply.md`, §2) | - | - |
| 2026-10-03 | Not a gap, a to-do at the next pin: `luna diff --audio` is on luna `develop` (`a47a55e`), validated on that build (`2026-10-03_to_luna_diff-audio_reply.md`). When the release carrying it is pinned: replay the three runs of the reply on the pinned binary, then add to `.claude/rules/testing.md` that a commit re-capturing `baselines/audio.json` quotes its output | luna develop `a47a55e` | - |
