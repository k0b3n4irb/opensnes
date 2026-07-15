# Lib ASM serving C callers: never rely on the caller's DB for data above $1FFF

**Rule.** A `lib/source/*.asm` function that is a public C API (declared in
`lib/include/snes/*.h`) runs with **DB = $00** (the crt0 contract for C).
Any absolute / absolute,X read of a WRAM symbol placed **above $1FFF**
(e.g. a `BANK $7E SLOT 2` RAMSECTION) therefore reads **open bus** —
$00:2000+ is registers/ROM, not a WRAM mirror. Use **long addressing**
(`lda.l sym`, `adc.l sym,x`, `sta.l sym`) for every such access, or place
the data under $2000.

**Why this exists (two shipped bugs, same class):**
1. Dynamic-sprite queue/state — originally `BANK $7E SLOT 2 ($2800+)`,
   unreachable from C; fixed by moving under $2000 (see the comment in
   `templates/crt0.asm` ~l.225).
2. `mapGetMetaTile` / `mapGetMetaTilesProp` (issue #103, Cooper dogfood
   F14, fixed 2026-07-12) — read `metatilesprop` / `mapadrrowlut` /
   `maptile_L1*` ($7E:3000+) with plain absolute addressing. Worked from
   nothing (no asm callers existed); returned open-bus garbage from C.
   Nobody saw it because **no example called the getters** — the missing
   piece was an execution test from C.

**The test convention that locks it:** every map/lib API meant to be called
from C gets a **libtest vector called from C** (devtools/libtests). The
#103 pin loads real tmx2snes data (shared with examples/maps/mapscroll,
pinned in bank 2) and asserts `mapGetMetaTilesProp(1280,80) == 0xFF00`
(T_SOLID) — host-parsed expected values, proven failing on the pre-fix lib.

**Review checklist for new lib ASM touching WRAM:**
- Data above $1FFF (any `BANK $7E/$7F SLOT 2` RAMSECTION)? → all reads and
  writes in C-callable paths must be `.l` (long), like `mapLoad` already
  does (`sta.l maptile_L1b` etc.).
- Or is the data small and C-visible? → prefer `BANK 0 SLOT 1` (< $2000),
  like `.map_bank00` / the dynamic-sprite state.
- Either way: a libtest vector that calls the API **from C**.

Related: `mapLoad`'s @note in `map.h` (user-side of the same trap — F15) and
issue #104 (mechanical class lock, read-side symmetric of the spill check).
