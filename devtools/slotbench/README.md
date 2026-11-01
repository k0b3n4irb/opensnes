# slotbench — sprites at fixed VRAM slots, against the dynamic engine

A contributor bench, not an example: it decides a design question and gates
nothing. `make bench-slots` builds one ROM per variant under `build/slotbench/`
and runs it on luna.

The case: N sprites 32x32, 130 synthetic frames of 512 bytes (every word names
its frame and strip), a tick every two frames, a row and a column of map (64
bytes each) sent each frame. Two scenarios: `mix` (changes per tick: mean 4.08,
median 3, p90 10, max 18) and `burst` (all change at once every 32 ticks).

| Variant | What runs |
|---|---|
| `dyn16`, `dyn18` | `oamDynamic32Draw`, drained by the NMI hook |
| `fix16`, `fix18` | a C loop (`slotsUpdate`), `vramQueuePushSprite`, `oamSet`; 5 frames per VBlank |
| `fix18a` | the same decision in assembly (`slots.asm`): what a module's loop would cost |
| `fix18u` | as `fix18`, push budget of 9 frames, `vramQueueFlushUntil(258)` |

Columns, master cycles per frame over frames 300-1323 (`luna profile`):
`scenario` is the bench's own work, left out of every comparison; `decide` the
frame-changed test and the queueing; `place` the OAM writes; `dyn_draw` both of
them in the dynamic engine; `dyn_nmi` its NMI work, sends included; `flush` the
queue's sends. Then `bench_dra…` is the longest run of frames with something
unsent, `bad_strip` the VRAM strips that did not end on the frame asked, and
`vram_lost` / `oam_lost` / `lost_frames` the bytes written outside the blank, from
luna's memory trace: the PPU drops those. Findings and their reading:
`.claude/notes/tech/fixed_slot_sprites.md`.
