# fuzz — libFuzzer harnesses for the asset tools' parsers

Every parser an asset tool feeds with a user file has a harness here:
`lodepng` (gfx4snes, img2snes), `itloader` (smconv), `tiled`
(tmx2snes's cute_tiled), `json` (aseprite2snes), `stbimage` (font2snes).
Seeds are the golden fixtures of each tool.

```sh
make -C tools/fuzz            # build (clang, ASan + UBSan + fuzzer)
make fuzz                     # FUZZ_SECONDS per target (default 60); a crash fails
make fuzz-replay              # replay crashes/<target>/* — the regression corpus
```

`fuzz.yml` runs ten minutes per target on any push touching `tools/` and
weekly; `make test-sanitizers` runs the replay. A fixed crash commits its
input under `crashes/<target>/`; the evolving `corpus/` is local and
ignored. Contributor-only, never shipped.
