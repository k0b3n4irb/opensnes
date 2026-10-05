# sa1-patch — SA-1 map-mode bits in the ROM header

A post-link step for `USE_SA1=1` builds. wlalink cannot tell an SA-1
cartridge from plain LoROM at link time, so after the link
`make/common.mk` runs:

```sh
sa1_patch game.sfc
```

which ORs byte `$7FD5` of the header with `$03` (`$20` LoROM → `$23`
SA-1) and recomputes the checksum pair at `$7FDC`/`$7FDE`. Idempotent;
exit 0 on success, 1 on an I/O error. It replaced an inline Python
one-liner in `common.mk` (build audit, P2.4 #3).

Build: `make -C tools/sa1-patch` (same pattern as the other tools: clang,
static on Linux and Windows, `SANITIZE=1` for the sanitizer job). In 1.x
this function belongs to `opensnes-rom`.
