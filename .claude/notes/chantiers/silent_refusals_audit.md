# Silent refusals: audit and counters (2026-11-01)

Status: lot done (counters), two items left open.

## Audit

| Call | Refusal | Was visible? |
|---|---|---|
| audio: every `AUDIO_ERR_*` (27 return sites in `audio.c`) | code returned | only if the caller reads it |
| `vramQueuePush`, `vramQueuePushSprite` | 0 returned, nothing noted | only if the caller reads it |
| `dmaCopyVram` and the other lib DMA with the screen on | the PPU drops the write | never |
| sample slot over `AUDIO_MAX_SAMPLES` (64) | `AUDIO_ERR_INVALID_ID` at run time | same as the first row |

## Done

- `audio_refuse()` in `lib/source/audio.c`: all 27 returns go through it;
  `audio_error_count` (u16, wraps) and `audio_error_last` (u8), declared in
  `snes/audio.h`.
- `vram_queue_refused` (u16) in `templates/vramqueue_ram.asm`, incremented
  on both refusal paths of `vramqueue.asm`.
- Tests: `testing/fixtures/libtests_aram` (`e_err_count` 3, `e_err_last` 1),
  `testing/fixtures/libtests` (`r_vqref` 2 then 3). Read from WRAM by luna.

## Left open

1. `dmaCopyVram` with the screen on: INIDISP ($2100) is write-only, so a
   check needs a shadow and charges every call on a path tuned to 6 cycles
   (see `dma.asm`). Better caught by the emulator (a VRAM write outside
   blanking as an event): a capability request to luna, not yet sent.
2. The 64-sample limit stated at compile time: only for a constant slot,
   which needs a macro in front of the function. Not started.
