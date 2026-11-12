# Journal of wram.json

Every rewrite of `baselines/wram.json` leaves a line here (`wram_regress.py --update --why "..."`): the date, the
commit it was made on, how many examples drifted, the reason with its evidence, and the hash of the file written.
A drift that was a real defect is marked **CATCH**. The point is to count: an oracle that is rewritten often and
catches nothing measures the change, not the correctness (independent review of 2026-11-09). `devtools/check_wram_journal.py`
(in `make lint`) refuses a `wram.json` whose hash is not the last line's, and `--stats` counts the month.

| Date | Commit | Drifted | Kind | Why (and the evidence) | wram.json |
|---|---|---|---|---|---|
| 2026-11-08 | 76adcdb0 | 65 of 65 | benign | 32-bit mul/div safe in an NMI callback: the startup code gains 16 bytes of WRAM, every address after it moves; every frame hash and manifest unchanged (backfilled) | - |
| 2026-11-08 | 7802f85b | 65 of 65 | benign | WaitForVBlank no longer loses a frame: the NMI handler is about 11 cycles longer, the phase of every frame moves; diff_corpus 65/65 MATCH at tolerance 0 (backfilled) | - |
| 2026-11-09 | 09373f61 | 1 of 65 | benign | the audio driver holds a key-off 132 cycles instead of 582: sample_swap's acknowledge loop counter (tcc__lf+0, 6 turns then 4) is the only hashed byte that moves (backfilled) | - |
| 2026-11-09 | journal | 0 of 65 | benign | journal opened: the baseline as it stands, no rewrite | `04709597e08d` |
| 2026-11-10 | 011d7645 | 12 of 65 | benign | luna pin v1.40.0 -> v1.43.0 (S-DSP stepped per SPC cycle, joypad auto-read 4224 clocks): 12 of 65 streams move with ROMs unchanged and frames identical (luna_runner --compare 172/172, audio 9/9 recaptured within 0.67 % RMS); on mode2, aim_target and snesmod_music only the direct page and the stack page differ, on ppu_reset also four C-variable pages (PPU latches read at another instant) | `978fcf3e0db3` |
| 2026-11-11 | 217585ab | 1 of 65 | benign | text_pixel_text: memset (perf(lib), unrolled stores with the DBR on the destination) clears its 2.5 KB FAR layer faster than through [dp],y, so the boot reaches each vblank earlier and leaves other scratch in the direct page; testing/diff_corpus.py against the ROM built with the previous string.asm: MATCH at frame 200, tolerance 0; the 64 other streams unchanged | `974690259cf9` |
| 2026-11-12 | ea12ac8a | 3 of 65 | benign | map engine (6f2464ac): the column buffer holds the column top to bottom and 3 new words in bank $7E move map_scroll and platform_objects; animated_sprite calls oamSetAttr (d4ba6f29), which uses no tcc__r9 scratch. 3 of 65; frames identical: luna diff map_scroll 196/196 frames of a walk, platform_objects 225/225 of its playthrough, animated_sprite 134/134 of a four-direction walk; diff_corpus 65/65 MATCH | `b09f17e73048` |
| 2026-11-12 | 92591bf1 | 0 of 65 | benign | no stream moves: two re-captures made in separate worktrees (the memset of 217585ab, 1 stream; the map engine and oamSetAttr of ea12ac8a, 3 streams) are merged on develop; wram_regress passes 65 of 65 on the merged file before this line | `d8017390c138` |
