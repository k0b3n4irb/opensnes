# API naming decisions — owner's call before the v1.0 freeze

Status: owner said go on N1 then N2-N6 (2026-09-22); D1-D5 still open.
Written 2026-09-21.
Source: `.claude/notes/reviews/2026-09-20_api_audit.md` §3.2 / §3.3.

Everything in the audit that was a *defect* or a *same-slot retype* is done
(see the status blocks in the audit). What is left is taste: a name is only
wrong relative to the names around it. Each row below can ship without a
break — new name + the old one as an `OPENSNES_DEPRECATED` alias (mechanism
in `types.h`, warning not error in user builds, precedent `30fe59ce`) — and
the aliases disappear at the first breaking release. "Uses" counts call
sites in `examples/`, `docs/`, `templates/` on 2026-09-21: the cost of the
rename inside the repo.

## Recommended: do

| # | Today | Proposal | Uses | Why |
|---|---|---|---|---|
| N1 ✅ 2026-09-22 | `TRUE` = `0xFF`; predicates return `0xFF` (`isPAL`, `isInVBlank`, `padIsConnected`), `1` (`getRegion`, `mouse*`) or a raw flag (`scopeIsConnected`) | every predicate returns 0 / 1; `TRUE` = 1 | 2 | `if (isPAL() == 1)` is false today. Two in-repo comparisons to check. Not aliasable: it is a value change, do it before the freeze or never |
| N2 | `scopeButtonsHeld` = auto-repeat; "currently down" is `scopeButtonsDown`, while `padHeld` / `mouseButtonsHeld` = currently down | `scopeButtonsHeld` = currently down, `scopeButtonsRepeat` = the auto-repeat one; `scopeButtonsDown` deprecated | 1 | same word, opposite meaning on the third device |
| N3 | `colorMathEnable(mask)` / `mosaicEnable(mask)` **replace** the layer set; `windowEnable` / `hdmaEnable` **OR** into it | `colorMathSetLayers` / `mosaicSetLayers` for the replacing pair, old names deprecated | 18 | "Enable" that disables what you enabled a line earlier |
| N4 | `sa1Init()` initialises nothing (crt0 does) and returns the status; `dsp1Present()`; `gsuInit()` returns presence *and* sets defaults | `sa1IsReady()`, `dsp1IsPresent()`, `gsuIsPresent()` (+ `gsuInit` keeps the defaults part) | 14 | docs already cite a `sa1IsReady()` that does not exist |
| N5 | `LzssDecodeVram` — the only capitalised public function | `lzssDecodeVram` | 14 | one-letter rename, alias is free |
| N6 | `rand()` / `srand()` with non-libc signatures | `rngNext()` / `rngSeed()` (or `randU16` / `randSeed`) | 25 | a libc name that is not the libc function collides the day a user links any C library code |

## Recommended: decide, either answer is defensible

| # | Question | Options |
|---|---|---|
| D1 | `hdmaEnable(mask)` while every other hdma function takes a channel index (header examples are correct since `d4271733`) | (a) keep the mask, add `HDMA_MASK(ch)`; (b) `hdmaEnable(channel)` + `hdmaEnableMask(mask)` — (b) changes the meaning of an existing call, so it needs a release where the old form warns |
| D2 | `WaitForVBlank` — the lone PascalCase function, 363 uses, and the name every PVSnesLib port arrives with | (a) keep it, documented as the one deliberate exception; (b) `waitForVBlank` + alias. Recommendation: (a) — the port-compatibility is worth more than the consistency |
| D3 | `sqrt16`, `atan2_8`, `mul16`, `ease_in_quad`, `ease_out_quad` break the `fixXxx` camelCase of their siblings (21 uses) | rename to `fixSqrt` / `fixAtan2` / … with aliases, or leave: they read like the libm names they mimic |
| D4 | unprefixed exported globals that become frozen API: `x_pos` / `y_pos` (camera, 20 uses), `cursor_x` / `cursor_y` (text), `cgwsel`, `cgadsub`, `mosaic_size`, `objptr`, `objgetid` | prefix them (`map_cam_x`, `text_cursor_x`, …) behind accessor functions, or declare them internal and stop documenting them. Globals cannot be aliased by a macro without risk (`x_pos` is a plausible user identifier — that is the problem) |
| D5 | duplicates: `getRegion` vs `isPAL`; `profileGetFrameCount` vs `getFrameCount`; `VBlankCallback` vs `VoidFn`; `BG_MODEn` vs `BGMODE_MODEn`; `mode7SetPivot(u8,u8)` vs `mode7SetCenter(s16,s16)`; `gameLoopRun` vs `sceneRun`; four identical layer-mask constant sets | pick one of each and deprecate the other; none is a bug |

## Not recommended now

- `console.h` hosting six naming schemes: splitting the header moves
  `#include` lines in every example for no behavioural gain. Revisit with
  the first breaking release.
- `t_objs` / `u8 xpos[3]`: accessors (`objGetX(idx)`) are additive and can
  land any time; renaming the struct is a break with no alias path.
- `dmaTransfer`'s signature, removal of the five `*Bank` functions: already
  scheduled for the breaking release (`30fe59ce`).

## How a decision lands

One commit per row: new name, deprecated alias, in-repo callers migrated,
a libtest vector where behaviour is involved (N1, N2, N3), `make tests` +
`diff_corpus` 85/85. N1 first — it is the only one that cannot be deferred.
