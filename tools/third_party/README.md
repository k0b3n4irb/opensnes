# tools/third_party — vendored code the asset tools compile

| File | Origin | Licence | Used by |
|---|---|---|---|
| `lodepng.{c,h}` | [lodepng](https://github.com/lvandeve/lodepng) 20260119 (PNG decode/encode) | zlib | gfx4snes, img2snes, `tools/fuzz` |
| `cmdparser.{c,h}` | [cmdparser](https://github.com/XUJINKAI/cmdparser), inherited from PVSnesLib's gfx4snes | Apache 2.0 (`LICENSE-cmdparser`, shipped in the zip) | gfx4snes, img2snes |
| `stb_image.h` | [stb](https://github.com/nothings/stb) 2.30 | public domain or MIT | font2snes, `tools/fuzz` |
| `cute_tiled.h` | [cute_headers](https://github.com/RandyGaul/cute_headers) 1.06 | zlib or public domain | tmx2snes, `tools/fuzz` |

`ATTRIBUTION.md` at the repository root is the authoritative list. A tool
that needs one of these adds `-I../third_party` to `INCLUDES` and the `.c`
to `SRCS` in its Makefile (`tools/tool.mk`). Until 2026-09-15 two tools
carried byte-identical copies of lodepng and cmdparser (a fix applied to
one silently missed the other); until 2026-10-05 the two headers sat in
two tools' source trees. The cppcheck pass in `make lint-cppcheck`
suppresses this directory (upstream code with its own noise).

## Local patches

- `lodepng.c` `zlib_decompress` (2026-09-25): the pre-inflate reservation
  is capped at 1032 x the compressed size (deflate's maximum expansion).
  Upstream reserves the size the IHDR declares, so a 114-byte PNG claiming
  1073741872 x 16 pixels asked for 17 GB before inflating anything (found
  by the nightly fuzzer). A genuine image still gets its buffer in one
  allocation; a lying header fails with lodepng error 91. Re-apply on any
  lodepng update — regression input `tools/fuzz/crashes/lodepng/`.
