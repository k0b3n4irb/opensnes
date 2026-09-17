# Regression inputs from fuzzing

One file per bug the fuzzers found and we fixed, under `<target>/`. `make
-C tools/fuzz replay` runs each through its harness on every push (part of
the sanitizer job): a fixed crash must stay fixed. Name them after the fix
commit or the issue (`itloader/short-header-<sha>.it`). Keep the
directories even when empty — `.keep` files hold them.

## What is here

| Input | Target | Bug |
|---|---|---|
| `itloader/oom-sample-length-3.7GB.it` | smconv's Impulse Tracker loader | a sample header asked for 3.7 GB; the loader now bounds the length by what is left in the file (2026-09-14) |
| `tiled/asan-heap-overflow-trailing-space.tmj` | cute_tiled (tmx2snes) | both scanners skipped whitespace past the end of the caller's buffer, which is not NUL-terminated — heap-buffer-overflow (2026-09-15) |
| `stbimage/ubsan-memcpy-null-empty-idat.png` | stb_image (font2snes) | a zero-length read reached `memcpy` with a NULL source — undefined behaviour (2026-09-15) |
| `stbimage/oom-idat-declares-2GB.png` | stb_image (font2snes) | an 840-byte PNG declared a huge IDAT chunk; the accumulator is grown to the DECLARED length before any byte is read, so stb asked for `realloc(2 GB)`. Bounded by what a memory source can still supply (2026-09-16) |
