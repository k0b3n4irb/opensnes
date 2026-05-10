---
name: cproc/QBE static-const struct with pointer fields — FIXED
description: QBE emit.c claimed DL=8 bytes but emitted .dl which WLA-DX writes as 3 bytes for symbol references — fixed in chantier C.5 by padding each emitted directive with .dsb to match dtype_size; typed BgAsset is now functional end-to-end
type: project
originSessionId: b92ad4fc-ea9c-4479-b229-46e53ee464ae
---

## Status: FIXED in chantier C.5 (2026-05-06)

Patch: `compiler/qbe/emit.c` `emit_init_data()` now adds `.dsb N, 0`
after each emitted directive to make the written byte count match
`dtype_size[type]`. Validated end-to-end: typed `BgAsset` in
`examples/graphics/backgrounds/mode1` produces identical visual
output to the macro-form `BG_LOAD`. All 261 checks pass.

---

## Symptom

Defining a `static const Struct foo = { .ptr = symbol, ... };` where any
field is a pointer (`u8 *`, function pointer, etc.) produces a ROM image
where the field offsets do not match the struct layout cproc compiled
against. Reads of fields after the first pointer return garbage bytes.

Concrete trigger that surfaced this (chantier D.2, 2026-05-02):

```c
typedef struct {
    u8 *tiles;
    u8 *tiles_end;
    u8 *palette;
    u8 *palette_end;
    u16 color_mode;
    u8 *tilemap;
    u8 *tilemap_end;
    u8  map_size;
} BgAsset;

static const BgAsset bg = {
    .tiles = bg_tiles, .tiles_end = bg_tiles_end,
    /* ... */
};
```

Runtime accesses `tilemap` at struct offset 40 (5×8-byte fields), but
ROM has it at offset 20 (5×3-byte WLA-DX `.dl` outputs + 6-byte
`.dsb` padding for the `u16` field). Every read past offset 12 is
garbage.

## Root cause

`compiler/qbe/emit.c:47-52` defines:

```c
static int dtype_size[] = {
    [DB] = 1, [DH] = 2, [DW] = 4, [DL] = 8
};
```

So QBE accumulates `init_total_size` assuming each `DL` (long, 8-byte)
data item costs 8 bytes.

`compiler/qbe/emit.c:178-182` emits:

```c
static char *dtoa[] = {
    [DB] = "\t.db", [DH] = "\t.dw", [DW] = "\t.dl", [DL] = "\t.dl"
};
```

Both `DW` (4-byte) and `DL` (8-byte) emit the WLA-DX directive `.dl`.
For a symbol argument (`.dl my_symbol+0`), WLA-DX 65816 writes a
3-byte 24-bit address — not 4 and not 8.

Three-way mismatch:
- **C struct layout**: cproc treats `u8 *` as 8 bytes (matches `dtype_size[DL]`)
- **QBE size accumulator**: 8 bytes per `DL` (consistent with above)
- **WLA-DX actual emission**: 3 bytes per `.dl <symbol>`

The struct in ROM is therefore 5 bytes shorter per pointer than what
cproc-generated code assumes. Field accesses past the first pointer
read into the next field's bytes (or padding, or unrelated memory).

## Why this hasn't been caught before

OpenSNES has no other `static const Struct{ ptr }` pattern in lib or
examples. Function pointers in `GameLoopConfig` are only used via
stack-local instances initialised at runtime by cproc-emitted stores —
those work because the load and store paths are both cproc and use the
same 8-byte stride.

## Workaround

D.2 ships as macro-only (`BG_LOAD`, `GFX_LOAD` in
`lib/include/snes/asset.h`) — no struct in ROM, all loading is
inlined into the call site via `bgInitTileSet` + `dmaCopyVram`.

## Fix candidates (future chantier)

1. **Change QBE to emit 8 bytes per `DL` symbol**: e.g. `\t.dl <sym>+0\n\t.dl 0\n` (two `.dl` directives, total 8 bytes if `.dl`=4, but WLA emits 3 bytes for symbol vs 4 for immediate — needs careful test). Cleanest: `.db < <sym>+0, > <sym>+0, : <sym>+0, 0, 0, 0, 0, 0` (explicit byte sequence with low/mid/high split + 5 zero bytes).
2. **Change struct layout**: have cproc treat `u8 *` as 4 bytes for static-init purposes — but this conflicts with the 8-byte-pointer ABI used everywhere else.
3. **Fix WLA-DX `.dl` semantics**: make `.dl <symbol>` emit 4 bytes (24-bit + zero pad) consistently — would be an upstream WLA change.

Option 1 is the most contained: only touches QBE's `emit_init_data()`,
no ABI change, no upstream dependency.

## Acceptance test for the fix

Once fixed, this test must compile, link, and produce visually identical
output to the macro version of `examples/graphics/backgrounds/mode1`:

```c
typedef struct {
    u8 *tiles, *tiles_end;
    u8 *palette, *palette_end;
    u8 *tilemap, *tilemap_end;
    u16 color_mode;
    u8  map_size;
} BgAsset;

extern u8 bg_tiles[], bg_tiles_end[];
extern u8 bg_pal[],   bg_pal_end[];
extern u8 bg_map[],   bg_map_end[];

static const BgAsset bg = {
    .tiles = bg_tiles, .tiles_end = bg_tiles_end,
    .palette = bg_pal, .palette_end = bg_pal_end,
    .tilemap = bg_map, .tilemap_end = bg_map_end,
    .color_mode = BG_16COLORS, .map_size = SC_32x32,
};

void load(void) {
    bgSetMapPtr(0, 0x0000, bg.map_size);
    bgInitTileSet(0, bg.tiles, bg.palette, 0,
                  bg.tiles_end - bg.tiles,
                  bg.palette_end - bg.palette,
                  bg.color_mode, 0x4000);
    dmaCopyVram(bg.tilemap, 0x0000, bg.tilemap_end - bg.tilemap);
}
```

If `load()` produces the same image as the macro version, the bug is
fixed and D.2 can be expanded to ship a typed `BgAsset` value.
