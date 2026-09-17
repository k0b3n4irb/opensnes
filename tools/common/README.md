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
lodepng (upstream code, left untouched).
