# tools/common — sources shared by the asset tools

| File | Origin | Licence | Used by |
|---|---|---|---|
| `lodepng.{c,h}` | [lodepng](https://github.com/lvandeve/lodepng) (vendored, PNG decode/encode) | zlib | gfx4snes, img2snes, `tools/fuzz` |
| `cmdparser.{c,h}` | command-line parser inherited from PVSnesLib's gfx4snes | Apache 2.0 | gfx4snes, img2snes |

Until 2026-09-15 each tool carried its own byte-identical copy of these
four files; a fix applied to one copy silently missed the other (gaps
review H8). A tool that needs them adds `../common` to its include path
and compiles `$(COMMON)/<name>.c` into its own `build/` — see the
`common_%.o` rule in `tools/gfx4snes/Makefile`. The cppcheck pass in
`make lint-cppcheck` scans this directory with the tools and suppresses
lodepng (upstream code).

## Local patches

- `lodepng.c` `zlib_decompress` (2026-09-25): the pre-inflate reservation
  is capped at 1032 x the compressed size (deflate's maximum expansion).
  Upstream reserves the size the IHDR declares, so a 114-byte PNG claiming
  1073741872 x 16 pixels asked for 17 GB before inflating anything (found
  by the nightly fuzzer). A genuine image still gets its buffer in one
  allocation; a lying header fails with lodepng error 91. Re-apply on any
  lodepng update — regression input `tools/fuzz/crashes/lodepng/`.
