# Aseprite Pipeline

![Screenshot](aseprite_pipeline.png)

An animated 32×48 metasprite — Kit, the fox-kit courier — whose **every byte of
data is generated from an Aseprite export** — no hand-written frame tables, no
hand-typed animation clips. This is the working reference for the two-tool sprite pipeline:

```
hero.aseprite ─┬─ res/hero.png  + hero.png.toml  → opensnes-sprite sheet → hero.pic/.pal + hero_meta.inc
               │                                    (tiles, palette, metasprite pointer table)
               └─ res/hero.json + hero.json.toml → opensnes-sprite anim  → hero_anim.h
                                                    (one AnimClip per Aseprite tag)
```

`sheet` owns the pixels and the metasprite geometry; `anim` owns the
timeline (tags, per-frame durations, direction). They meet at the frame value: a
clip's frame *i* selects `hero_metasprites[i]`, resolved inline by
`animTickMeta()` and drawn with `oamDrawMetasprite()`.

The sheet holds eight frames and two tags — **walk** (frames 0-3, forward loop)
and **wave** (frames 4-7, ping-pong) — with per-frame millisecond durations.
`opensnes-sprite anim` converted those to ticks and folded the ping-pong into the
frame order (4, 5, 6, 7, 6, 5). Press **A** to toggle between the two generated
clips.

## Assets

`res/hero.png` and `res/hero.json` are what Aseprite writes with
`--sheet hero.png --data hero.json --list-tags` (the JSON-array form). Here they
are made by `devtools/art_assets/build.py` from `characters/kit_tall_sheet.png`:
Kit drawn by PixelLab at 32×48, his walk and his wave animated by its
`animate-with-text-v3` (CC0, `assets/neo-lumen/README.md`). A real `.aseprite`
project exported with those options gives the same two files.

## SNES Concepts

- Metasprite composition from multiple OAM entries (`oamDrawMetasprite`)
- The `anim.h` player driving a metasprite via `animTickMeta()`
- Machine-generated metasprite table (`opensnes-sprite sheet --metasprite`) + animation clips (`opensnes-sprite anim`)
- OBJSEL size mode and OBJ VRAM base for 16×16 hardware sprites
- Sprite palette at CGRAM 128 (`OBJ_CGRAM_BASE`)

## How to Build

```sh
cd examples/sprites/aseprite_pipeline && make
```

The build runs the full pipeline from the two settings files beside the
assets (`res/hero.png.toml`, `res/hero.json.toml`): `opensnes-sprite sheet`
and `opensnes-sprite anim`, then the generated `assets_gen.asm` links the
tiles, then `main.c` compiles (it `#include`s both generated headers). No
`data.asm`, no conversion rule in the Makefile (`docs/tools/CONVENTIONS.md`).

## Modules Used

`console`, `sprite`, `dma`, `text`, `text4bpp`, `background`, `input`, `anim`

## See Also

- `tools/opensnes-sprite` — sheets, metasprites and Aseprite clips (`docs/tools/opensnes-sprite.md`)
- `examples/sprites/sprite_sizes` — the OBJ size pairs a metasprite is built from
- `examples/sprites/animated_sprite` — single-sprite frame animation
