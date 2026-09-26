# The Opt-In Framework Tutorial {#tutorial_framework}

This tutorial covers the two smallest modules in OpenSNES: `gameloop` (one
public function) and `asset` (two public functions, two macros). They are the
first pieces a newcomer meets — the `main()` and the "get my PNG on screen"
step — and both are deliberately **optional**.

`PHILOSOPHY.md` lists "Mandatory framework lifecycle" under Non-goals, in
these words:

> A user is welcome to write a `main()` with their own
> `while(1) { WaitForVBlank(); ... }` loop. The framework opt-ins are
> recommendations, not requirements.

That is not a disclaimer, it is the design. Both modules are linked only if
you name them in `LIB_MODULES` (Principle 3 — modules are opt-in, never
all-or-nothing):

```makefile
LIB_MODULES := console dma background asset gameloop
```

`asset` transitively pulls `dma` and `background` (`_DEP_asset` in
`make/common.mk`). `gameloop` has no `_DEP_` line at all — it pulls nothing
beyond the runtime that every ROM already links.

The corpus votes with its feet: four examples call `gameLoopRun`
(`basics/timer`, `basics/random`, `basics/aim_target`, `input/controller`);
three use the asset bundles (`backgrounds/mode1`,
`backgrounds/mode1_bg3_priority`, `games/shmup_1942`). Everything else writes
the loop and the loads by hand, and nothing about those examples is
second-class. `games/shmup_1942` uses `bgLoad` *and* hand-writes its loop —
the two opt-ins are independent of each other.

The third member of this family is the scene stack (`sceneRun` / `scenePush` /
`scenePop`), which has its own page — see @ref tutorial_game_states for that
one.

## gameLoopRun — the boilerplate it replaces

`GameLoopConfig` is a pair of function pointers. It is not its own struct: it
is a `typedef` of `Scene` from `<snes/scene.h>`, so the same value can be
handed to `gameLoopRun` today and to `sceneRun` tomorrow without touching the
declaration.

```c
typedef struct {
    void (*init)(void);    // called once, may be NULL
    void (*update)(void);  // called every frame, MUST NOT be NULL
} Scene;
typedef Scene GameLoopConfig;
```

The whole implementation (`lib/source/gameloop.c`) is five lines:

```c
void gameLoopRun(const GameLoopConfig *cfg) {
    if (cfg->init)
        cfg->init();
    while (1) {
        WaitForVBlank();
        cfg->update();
    }
}
```

So the call site becomes:

```c
int main(void) {
    GameLoopConfig cfg = {
        .init   = on_init,
        .update = on_update,
    };
    gameLoopRun(&cfg);
    return 0;   /* never reached — gameLoopRun does not return */
}
```

`cfg` may live on `main()`'s stack, as all four examples do, precisely
because `main` never returns while the loop runs. The `return 0` exists only
to keep `int main(void)` well-formed; cproc/QBE has no `noreturn` path, so the
compiler cannot know the call never comes back.

There is no NULL check on `cfg->update`. That is the same deliberate omission
documented on `Scene::update` — a `JSL` into the register page crashes
immediately and is more diagnosable than a silently skipped frame, and the
per-frame test is not free at 1.79 MHz.

### The exact per-frame order

This is worth stating precisely, because the framework's share of it is
smaller than it looks:

| # | What runs | Who owns it |
|---|---|---|
| 1 | `init()`, once, before any `WaitForVBlank` | your code |
| 2 | `WaitForVBlank()` — sets `vblank_flag = 1`, then `WAI` | `templates/crt0.asm` |
| 3 | NMI fires: `frame_count++`, dynamic-sprite flush hook, OAM DMA, tilemap DMA, BG scroll registers, joypad/mouse/scope read | the NMI handler in `crt0.asm` |
| 4 | NMI clears `vblank_flag`, `WaitForVBlank` returns | `crt0.asm` |
| 5 | `update()` | your code |

`gameLoopRun` itself contributes steps 2 and 5 and nothing else. Everything in
step 3 — including the input read that populates `padHeld` / `padPressed` —
happens in the NMI handler and happens identically for a hand-written loop.
The framework does not read input, does not draw, and does not flush anything.

One consequence deserves emphasis: **`update()` does not run inside a usable
VBlank window.** `WaitForVBlank` returns only after the NMI handler has run to
completion and cleared the handshake flag (`crt0.asm`, the `stz.w vblank_flag`
at the very end of the ISR), and the handler has already spent the VBlank on
its own DMAs. By the time `update()` starts, VBlank is largely or entirely
gone. Write to the shadow buffers the lib gives you (the text tilemap buffer,
`oamMemory`, the scroll shadows) and let the *next* NMI upload them — which is
what the examples do. Do not treat `update()` as a place where direct VRAM
writes are safe.

> The Doxygen on `examples/basics/timer`'s `on_update` currently claims the
> opposite ("we are inside the VBlank window — VRAM/CGRAM/OAM writes are safe
> at this point"). That comment is wrong; the example itself is fine because
> `textPrint` only touches a RAM buffer. Trust this table, not that comment.

### What gameLoopRun will not do for you

It does not call `consoleInit()`, does not `setMode()`, does not touch
palettes, does not `setScreenOn()`, and does not install or configure the NMI
handler. All of that stays in your `init` callback, which is why any existing
hand-written `main()` converts by moving two blocks of code into two static
functions and changing nothing else. The canonical `init` body is the same one
you would have written inline:

```c
static void on_init(void) {
    textModeInit();
    textPrintAt(9, 8, "VBLANK COUNTER");
    WaitForVBlank();      /* let the first NMI DMA the tilemap buffer */
    setScreenOn();
}
```

Cost: one indirect call plus one direct call per frame, on the order of 30
cycles — the indirect call through `cfg` is not tail-call-optimised.

## Asset bundles — the tileset as a value

A background needs three blobs (tiles, palette, tilemap), two numbers
describing them (colour depth, map size) and two VRAM addresses. Written by
hand that is six `extern` declarations, three subtractions and three separate
calls, all positional. `GfxAsset` and `BgAsset` bundle the first five into one
value:

```c
typedef struct {
    u8 *tiles;
    u8 *tiles_end;
    u8 *palette;
    u8 *palette_end;
    u16 color_mode;    /* BG_4COLORS, BG_4COLORS0, BG_16COLORS, BG_256COLORS */
} GfxAsset;

typedef struct {
    GfxAsset gfx;
    u8 *tilemap;
    u8 *tilemap_end;
    u8  map_size;      /* SC_32x32, SC_64x32, SC_32x64, SC_64x64 */
} BgAsset;
```

The struct stores begin/end **pairs** rather than sizes on purpose: the
difference of two `extern` symbols is not a compile-time constant in C, so a
size field could not be initialised in `static const` storage. The subtraction
is done at load time instead.

Because the bundle is a value, it can be passed to a function, or stored in an
array indexed at runtime as a level table, or swapped at a scene boundary. The
addresses stay at the call site — the same tileset can be loaded to a
different VRAM address on a different screen.

### What one bgLoad call actually moves

`lib/source/asset.c` composes three existing calls, in this order:

```c
void bgLoad(u8 bg, const BgAsset *asset, u8 palette_slot,
            u16 tiles_vram, u16 map_vram) {
    bgSetMapPtr(bg, map_vram, asset->map_size);
    gfxLoad(bg, &asset->gfx, palette_slot, tiles_vram);      /* tiles + palette */
    dmaCopyVram(asset->tilemap, map_vram,
                (u16)(asset->tilemap_end - asset->tilemap));
}
```

and `gfxLoad` is the `bgInitTileSet` call you would otherwise write out:

```c
void gfxLoad(u8 bg, const GfxAsset *asset, u8 palette_slot, u16 tiles_vram) {
    bgInitTileSet(bg, asset->tiles, asset->palette, palette_slot,
                  (u16)(asset->tiles_end   - asset->tiles),
                  (u16)(asset->palette_end - asset->palette),
                  asset->color_mode, tiles_vram);
}
```

So one `bgLoad` fans out into **three transfers and two register writes**:
tiles to VRAM at `tiles_vram`, palette to CGRAM at a slot-derived colour
index, tilemap to VRAM at `map_vram`, plus `BGnSC` (map address + size) and
`BGnNBA` (tile address).

That fan-out is pinned by a functional test rather than by prose.
`tools/luna-test/manifests/dma_mode1_bgasset.toml` runs
`examples/backgrounds/mode1` — whose entire body is
`bgLoad(0, &bg, 0, 0x4000, 0x0000)` — and asserts all three destinations
independently: bytes in CGRAM at colour 0, bytes in VRAM under the tilemap at
word `$0000`, and bytes in VRAM under the tiles at word `$4000` (byte `$8000`;
VRAM byte address is word address × 2). The manifest also asserts
`unsafe_writes = 0`, i.e. every one of those transfers happened under forced
blank. A field-order slip in `BgAsset` — swapped tiles/map pointers, a stale
`_end` label — fails a named block instead of quietly changing the picture.

### The palette slot argument

`palette_slot` is an index, not a CGRAM address. `bgInitTileSet` converts it
according to `color_mode`, and the conversion is not uniform:

| `color_mode` | CGRAM colour index |
|---|---|
| `BG_16COLORS` | `slot * 16` |
| `BG_4COLORS` | `slot * 4` |
| `BG_4COLORS0` (Mode 0) | `bg * 32 + slot * 4` — the **BG number participates** |
| `BG_256COLORS` | always `0` — `palette_slot` is ignored |

`backgrounds/mode1_bg3_priority` is the worked example: three
`BG_16COLORS` bundles at slots 2, 4 and 0, which land at CGRAM colours 32-47,
64-79 and 0-15 respectively.

```c
DECLARE_BG_ASSET(bg1, BG_16COLORS, SC_32x32);
DECLARE_BG_ASSET(bg2, BG_16COLORS, SC_32x32);
DECLARE_BG_ASSET(bg3, BG_16COLORS, SC_32x32);

bgLoad(0, &bg1, 2, 0x2000, 0x0000);
bgLoad(1, &bg2, 4, 0x3000, 0x0400);
bgLoad(2, &bg3, 0, 0x4000, 0x0800);
```

### These calls belong in setup, under forced blank

`bgLoad` and `gfxLoad` DMA into VRAM and CGRAM with no VBlank gating of their
own. They are setup calls, not per-frame calls. Two idioms in the corpus, both
correct:

- **Explicit** — `backgrounds/mode1` and `games/shmup_1942` open with
  `setScreenOff()`, load, then `setScreenOn()`.
- **Implicit** — `backgrounds/mode1_bg3_priority` opens with `consoleInit()`
  and a `WaitForVBlank()`, relying on the fact that `crt0.asm` leaves INIDISP
  in forced blank from reset until the first `setScreenOn()`.

Prefer the explicit form in new code; it survives being moved into a scene's
`init`, where the screen is already on.

## Producing the symbols the macros expect

`DECLARE_BG_ASSET(name, color_mode, map_size)` expands to six `extern`
declarations plus a `static const BgAsset name` initialised from them. It
requires the symbol naming convention `<name>_tiles`, `<name>_pal`,
`<name>_map`, each with an `_end` sibling. `DECLARE_GFX_ASSET(name,
color_mode)` is the same without the map pair. You choose the prefix once, in
your `data.asm`.

The blobs themselves come out of gfx4snes. `make/common.mk` has a generic
`.png` → `.pic` + `.pal` rule, and an example that also wants a tilemap
overrides it — `backgrounds/mode1` asks for `-m` (emit the map) and `-o 16`
(16 colours):

```makefile
res/opensnes.pic res/opensnes.pal res/opensnes.map: res/opensnes.png
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -m -i $<
```

The three outputs are then given labels in `data.asm`, which is the entire
contract between the converter and the macro:

```asm
ASSET_SECTION "rodata1"          ; templates/assets.inc: any bank but $00

bg_tiles: .incbin "res/opensnes.pic"
bg_tiles_end:

bg_map:   .incbin "res/opensnes.map"
bg_map_end:

bg_pal:   .incbin "res/opensnes.pal"
bg_pal_end:

.ends
```

and on the C side:

```c
DECLARE_BG_ASSET(bg, BG_16COLORS, SC_32x32);
```

The macro is sugar; the struct is the contract. Nothing stops you from filling
a `BgAsset` yourself when the symbols do not follow the convention, or when
the pointers are computed rather than linked.

> `ASSET_SECTION` (`templates/assets.inc`, included in every assembled file)
> is the way to declare asset data: `SEMISUPERFREE BANKS 7-1`, so bank $00
> is never a candidate. A bare `superfree` section lets the linker pick the
> first bank that fits — bank $00 — which is how 14 examples ended up within
> 28 bytes of a full code bank before the corpus moved over on 2026-09-23.

## Side by side with the hand-written equivalent

The honest measure of a convenience is how much it removes. Here is
`backgrounds/mode0`, which loads four layers manually, against the same shape
written with bundles.

Manual — `examples/backgrounds/mode0/main.c`, one layer's worth:

```c
extern u8 t0[], t0_end[];      /* tiles   */
extern u8 p0[];                /* palette */
extern u8 bgm0[], bgm0_end[];  /* tilemap */

bgInitTileSet(0, t0, p0, 0, t0_end - t0, 8, BG_4COLORS0, VRAM_BG0_TILES);
dmaCopyVram(bgm0, VRAM_BG0_MAP, bgm0_end - bgm0);
bgSetMapPtr(0, VRAM_BG0_MAP, SC_32x32);
```

Bundled:

```c
DECLARE_BG_ASSET(bg0, BG_4COLORS0, SC_32x32);

bgLoad(0, &bg0, 0, VRAM_BG0_TILES, VRAM_BG0_MAP);
```

Three externs and three calls become one declaration and one call. The
saving compounds: `mode0` repeats that block four times, so the layer-loading
body goes from twelve statements to four. What actually reaches the hardware
is identical — same `bgInitTileSet`, same `dmaCopyVram`, same `bgSetMapPtr`,
in a slightly different order.

The same comparison for the loop, `examples/basics/timer` against its
hand-written form:

```c
/* hand-written */                   /* framework */
int main(void) {                     int main(void) {
    textModeInit();                      GameLoopConfig cfg = {
    textPrintAt(9, 8, "...");                .init   = on_init,
    WaitForVBlank();                         .update = on_update,
    setScreenOn();                       };
    while (1) {                          gameLoopRun(&cfg);
        WaitForVBlank();                 return 0;
        tick();                      }
    }
    return 0;
}
```

That is the whole delta — two lines of structure. The framework's value here
is not line count, it is that the shape of an SNES program (init once, then
one body per VBlank) is named in the API instead of being folklore a beginner
has to infer from an example.

## When the framework gets in the way

Reach for the hand-written version when:

- **The synchronisation rhythm is not one update per VBlank.** Half-rate
  logic, a fixed-step accumulator, audio polled twice per frame, a busy loop
  that deliberately skips `WaitForVBlank` during a long load — all of those
  want an explicit loop. `gameLoopRun` has exactly one cadence and no hook to
  change it.
- **The per-frame body swaps between screens.** Dispatching a `switch` from
  inside `update` works, but the scene stack exists for this — see
  @ref tutorial_game_states. Because `GameLoopConfig` *is* `Scene`, promoting
  a game is a one-line change at the call site: `gameLoopRun(&cfg)` becomes
  `sceneRun(&cfg)`.
- **You need something between `WaitForVBlank` and the update**, such as a
  profiling marker or a scanline-timed write. There is no pre-update hook.
- **The asset does not fit the bundle.** Sprite assets have their own CGRAM
  offset (128) and OAM conventions and are out of scope for `asset` — use
  `oamInitGfxSet`. Tiled map data (tilesetdef, tilesetatt, large levels) goes
  through `mapLoad` instead, see @ref tutorial_map.

## Sharp edges worth knowing

- **Sizes are truncated to `u16`.** Both loaders cast the pointer difference
  to `u16`, so a single bundle component of 64 KB or more wraps silently. In
  practice no tileset gets close, but a concatenated multi-screen tilemap
  could.
- **`bg` is not range-checked.** `bgSetMapPtr` and `bgSetGfxPtr` switch on
  `bg` and do nothing at all for a value above 3. A typo'd background number
  produces a silent no-op, not an error.
- **`bgLoad` writes `BGnSC` before the tilemap DMA.** The register is set from
  `asset->map_size` first; the map bytes follow. Under forced blank this is
  invisible, but do not call `bgLoad` on a live layer mid-frame and expect a
  clean swap.
- **The asset struct holds raw pointers with no ownership.** The blobs live in
  ROM via `.incbin`; there is no copy and no lifetime tracking. Nothing here
  allocates — consistent with the Non-goal on hidden allocations.
- **`gameLoopRun` never returns, and the compiler does not know it.** Expect a
  `return 0` after every call site; that is the documented idiom, not a smell.

## See also

- @ref tutorial_game_states — the scene stack, the third opt-in in this
  family, and the right answer once the game has more than one screen.
- @ref tutorial_graphics — what gfx4snes actually emits into the `.pic`,
  `.pal` and `.map` files the bundles point at.
- @ref tutorial_dma — why setup transfers go under forced blank and what the
  per-VBlank budget is.
- `examples/backgrounds/mode1` — the minimum `bgLoad` program, and the ROM the
  `dma_mode1_bgasset` manifest measures.
- `examples/basics/timer` — the minimum `gameLoopRun` program.
- @ref gameloop.h "Game loop API reference" and @ref asset.h "Asset bundle API reference"
