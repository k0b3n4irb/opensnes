# Chantier — our own assets, and a rebranding

**Superseded in part (owner, 2026-10-18, later the same day):** start from
zero, not from the logo; cyberpunk, anthropomorphic animal characters, a
nuanced palette (no garish colours); the logo, the site and every example
in one identity; the examples reviewed and challenged before any
generation. The needs inventory and the review of the 85 examples are in
[`assets_rebrand_needs.md`](assets_rebrand_needs.md) (85 → 58 examples
proposed, 49 shared assets, ~600-800 generations); how to drive PixelLab
is in [`../tech/pixellab.md`](../tech/pixellab.md). The identity proposals
below (Kit the cartridge, the console's inside) are withdrawn; the
hardware rules, the acceptance gates and the trial protocol stand.

Status: PLANNING (2026-10-18). Owner decision of that day: 1.0 waits; first
a complete rebranding, and every example's assets redone with assets made
for OpenSNES. The owner has no graphics or sound expertise: the chantier
leans on generation tools, and on our pipeline as the quality gate.

This note holds the charter (v0, proposals marked **to decide**), the tool
facts checked on 2026-10-18, and the brief of the pilot, `likemario`.

## Why

- 56 files of 19 examples are byte-identical to PVSnesLib's, Nintendo
  characters among them (1.0 audit, 11-1 / 13-1); every release ships them,
  sources and built ROMs, under MIT, which does not cover them.
- The rest of our art is functional, not a showcase: no shared style, no
  shared palette, no identity.

## The tools (checked 2026-10-18)

| Tool | Driven by | What matters here | Terms |
|---|---|---|---|
| **PixelLab** | REST API v2 (`api.pixellab.ai/v2/docs`, all `POST /v2/…`) and an official MCP server (`https://api.pixellab.ai/mcp`, Bearer token; Claude Code is a listed client) | characters in 4/8 directions (≤168 px), text and skeleton animation (≤256 px), tilesets (`create-tileset`, side-scroller platforms included), map objects, inpaint, `image-to-pixelart`, style reference images; **forced palettes** on pixflux, bitforge, inpaint, rotate, skeleton animation (no colour-count parameter documented); ~$0.008 an image at 64x64, Pro tools ~$0.08 | ToS §1.3, §3.3: we own the outputs, commercial use allowed, free to distribute; outputs may **not** be used to train other models without permission; programmatic use only through the official API (§2.2) |
| **Retro Diffusion** (Astropulse) | MCP server (Bearer token `rdpk-…`), pay per generation (~$0.01, prepaid credits, free cost estimate) | grid-aligned pixels, palette-constrained output, transparent backgrounds, Wang/blob tilesets, tile variations, scene objects | its ToS not read yet: **to check** before use |
| Gemini / GPT image models | prompts, APIs | strong at concepts and keeping a character consistent across edits; not grid- or palette-native | per vendor |
| sfxr / jsfxr / ChipTone | parameters, deterministic | retro sound effects; the output is ours | free |
| Music | — | no AI tool found that writes tracker modules (`.it`): the AI music tools render audio. SNESMOD needs modules | commission a chiptune musician, or freely-licensed modules checked one by one |

Legal frame (to keep in `ATTRIBUTION.md`, not legal advice): in the US raw
AI output with no human authorship is not copyrightable (Copyright Office,
Part 2 report, January 2025; *Thaler v. Perlmutter*, D.C. Cir. 2025); it
can be distributed, but not really licensed. Human edits and a commissioned
kit are protectable. Whatever the tool, nothing may resemble an existing
property (no plumber, mushroom, warp pipe, question block, Goomba-like).

## Charter v0

### Identity — to decide

- **A world and a mascot of our own**, used across the examples and the
  site, so that the examples read as one SDK. Three directions to choose
  from: (a) a small robot, the cartridge's spirit; (b) a creature of an
  invented island world; (c) no mascot, a strong shared style only.
- **Names**: the examples whose name points at another property are renamed
  with their assets (`likemario` first; 156 references in the tree).

### Identity proposal (2026-10-18, after the owner chose "an identity of our own")

The examples are use cases, not ports: they need a world in which every
use case (a platformer, a shooter, a menu, a save screen, Mode 7) has a
place, and a cast that comes back from one to the next.

**Owner's choice (2026-10-18): a cyberpunk / steampunk mood, Blade
Runner's kind** — a rain-soaked neon megacity at night where brass steam
machinery runs under the neon. The mood only: nothing of the film itself
(no replicants, no flying police cars, no pyramid tower, no names). The
logo's colours already are that palette (violet, phosphor green, magenta
and amber neon on ink). Kit stays the mascot if the owner agrees: a small
cartridge-headed courier droid with brass joints, carrying data through
the city. The world: neon street levels and rooftops (the platformer),
the steam-pipe undercity (scrolling, parallax), the rain and the fog
(HDMA, colour math), the elevated tracks (Mode 7), the city's screens and
terminals (text, menus, saves). Its enemies: security drones, glitch
bugs, steam automatons. Backgrounds, sprites, tiles, UI and fonts all
come from it.

The first proposal, kept for the record: **the console's inside, built from the logo.** The logo
(`assets/logo_256x256.png`) is a violet cartridge whose label is a dark
screen showing `OS:` in phosphor green. The mascot is that cartridge come
alive — **Kit**, a small cartridge with a screen for a face (two green
pixel eyes), short legs and arms. The world is the inside of the console:
circuit-board plains with gold traces, chip and capacitor towers, cable
rivers, a CRT-phosphor sky. Its enemies are glitches (magenta pixel bugs,
corrupted tiles). Why this one:

- it is the brand: the mascot is the logo, the site and the examples say
  the same thing;
- its shapes are rectangles, pins and traces, which suit 8x8 tiles and
  small palettes;
- it resembles no existing property;
- every use case has a room: the platformer on the board, the shooter in
  the data bus, Mode 7 over the motherboard, the save screen in the
  battery, the menus on Kit's screen.

The two others, kept for comparison: an island world with an animal
mascot (warm, generic, close to many existing games); no mascot, a shared
style only (safest, least memorable).

**Palette seed — the logo's own colours** (measured; SNES 5-bit values):
`#12121e` (2,2,3) ink · `#1a2a3a` (3,5,7) and `#2b4555` (5,8,10) deep teal ·
`#2a2a4a` (5,5,9), `#3a346e` (7,6,13), `#5c50a4` (11,10,20), `#8a7aee`
(17,15,29) the violet ramp · `#09bb48` (1,23,9), `#0bdb52` (1,27,10)
phosphor green · `#e71861` (28,3,12) magenta · `#f6d714` (30,26,2) amber ·
`#6a6a8a` (13,13,17), `#a9b399` (21,22,19), `#f5f5f5` (30,30,30) greys.
Fourteen colours: the master palette grows from them (a gold ramp for the
traces, a warmer ramp for skin-like accents, a sky ramp).

**How the universe is prompted.** One generator does not invent a
universe from a word; the identity is written once and every prompt
carries it:

1. A *world bible* (this section): setting, shapes, palette, cast, tone.
2. Two or three anchor images — Kit, one ground tile set, one enemy —
   generated and retouched until they are right, with the palette forced.
3. Every later generation takes those anchors as style references
   (PixelLab's reference-image endpoints) and the forced palette: the
   style comes from the anchors, the prompt says only what is new.
4. Characters are made once and derived (rotation, animation from the
   same character), never prompted again from text.

Prompt shapes (English, as the tools expect), each with the forced palette:
- Kit: "small courier droid with a game cartridge for a head, violet
  plastic shell, dark screen face with two green pixel eyes, brass joints,
  short legs, a satchel, side view, dark outline, neon rim light".
- Ground: "side-scroller tileset, wet neon city rooftop, dark metal and
  brass pipes, rivets, puddles reflecting magenta and green neon,
  floating catwalk platforms".
- Background: "night megacity skyline, rain, fog, distant neon signs
  without text, steam vents, layered for parallax".
- Enemy: "hovering security drone, brass and dark metal, one magenta
  eye, side view".

### A trial before a subscription

PixelLab has a free trial with no card. Five tests, judged before paying:

| # | Test | Passes when |
|---|---|---|
| 1 | Kit, side view, 16x16 and 32x32 | one character at both sizes, readable at 16x16 |
| 2 | Kit walking, 4 frames, from test 1 | the same Kit in every frame, no flicker of shape |
| 3 | the ground tileset, 16x16 metatiles | the tiles join without seams; a level can be drawn with them |
| 4 | a glitch enemy, with test 1 as style reference | it belongs to Kit's world at a glance |
| 5 | test 1 through `opensnes-palette quantize` and the build | 15 colours + transparent, and it looks right in the ROM on luna |

Four out of five, with retouches that take minutes rather than hours: the
subscription is worth it. Fewer: Retro Diffusion gets the same five tests,
or a pixel artist gets the brief and the anchors are commissioned (the
rest is then derived by the tool from them).

The logo itself is better as vector art than as pixel art at large sizes:
a redrawn Kit logo (SVG, then the pixel sizes `assets/` ships) once Kit
exists.

### The hardware rules every asset follows (not negotiable)

- Tiles of 8x8 pixels; backgrounds built from 16x16 metatiles; sprites
  16x16 or 32x32 (the sizes `oamInit` pairs).
- 4 bpp: at most 15 colours plus transparent (index 0) per palette; 8 BG
  palettes and 8 sprite palettes on screen. Colours are 15-bit: each
  channel a multiple of 8 (`--round`).
- One pixel scale: no mixed resolutions, no anti-aliased edges, no
  gradients beyond a 2-step dither.

### Style — proposals, to decide

- **One master palette** of about 48 colours in ramps (a ramp: 3 to 5
  steps of one hue), from which every 16-colour palette is drawn.
  `opensnes-palette plan` keeps each scene within its 8 slots.
- Light from the top left; a dark selective outline on sprites (not pure
  black); backgrounds less saturated and darker than sprites, so the
  player reads at a glance.
- Animation: walk 4 frames, idle 2, jump 1, fall 1, at 8 to 10 frames a
  second.

### Sound — proposals, to decide

- Modules: up to 8 channels, short samples at 32 kHz or below, within the
  driver's room (`opensnes-music inspect`: 58,368 bytes without echo,
  58,624 - 2,048 x edl with it).
- Effects: sfxr-family, mono, under a second, through `opensnes-sample`.

### Acceptance of an asset

1. It converts with its settings file and fits its budget
   (`opensnes-sprite inspect`, `opensnes-tileset`, `opensnes-palette plan`,
   `opensnes-music inspect`).
2. It is judged in the ROM, on luna screenshots at the example's manifest
   frames, side by side with the asset it replaces.
3. Its provenance is written down: tool, prompt, date, the human edits
   (an `ATTRIBUTION.md` row per asset family).
4. Licence of the new assets: **to decide** — CC0 is the honest choice for
   AI output that is not copyrightable; CC-BY 4.0 for a commissioned kit.
   PixelLab's no-training clause binds us, not the people who reuse a CC0
   asset — to confirm with PixelLab before choosing CC0.

## The pilot: `likemario`

What it uses today (measured 2026-10-18):

| Asset | Today | Notes |
|---|---|---|
| Player | `res/mario_sprite.png`, 16x112: seven 16x16 frames, one OBJ palette; `[sheet] size = 16, colors = 16` | the code uses frames 1 (jump), 2-3 (walk), 6 (stand) |
| Tileset | `res/tiles.png`, 64x48: 48 tiles of 8x8, one BG palette | its drawings are the other property's (warp pipe, question block, hills, palm trees, cloud) |
| Level | `res/BG1.m16` (hand-made map, 47 tile indices used), `res/map_1_1.b16` (one property per tile: solid or empty) | both index the tileset by number |
| Music | `res/overworld.it` (45 KB) | to replace |
| Effect | `res/jump_sfx.it` (14 KB, the jump through `snesmodPlayEffect`) | to replace |
| Unused | `res/mariojump.brr` | delete |

Because the tileset's drawings are recognisable, redrawing tile for tile
at the same indices would keep the other property's level design. The
pilot therefore makes **a new tileset and a new level**: the level drawn in
Tiled and converted by `opensnes-level` (the pipeline `maps/map_scroll`
uses), its collision as tile properties. The format fits: `likemario`
reads a header of width and height in pixels then the size in bytes
(`main.c:264-266`), the same header as the `BG1.m16` `opensnes-level`
writes for `maps/map_scroll` (2400/240/18000 here, 3376/240/25320 there,
checked 2026-10-18). `maps/map_scroll` is itself built on the other
property's art (`tiledMario`, `tilesMario`): it follows the pilot.

Steps:

1. The owner decides the identity (mascot or not, the world) and opens the
   tool account (an API key; PixelLab's MCP is added to Claude Code with
   its token).
2. The master palette, then the player (4 directions not needed: a
   side view, the four animations above), then the tileset (ground top and
   fill, a floating platform, two decorations, a background layer if it
   fits the palette budget).
3. Conversion through the settings files, the level in Tiled, the build.
4. luna screenshots at the manifest's frames, old and new side by side; the
   owner judges.
5. Music and the jump effect last (a commissioned module or a free one;
   the jump from sfxr).
6. The manifest `movement_likemario.toml` follows the new level geometry;
   the fbhash, WRAM and audio baselines are re-captured, each with its
   justification (`testing.md`).

### Owner's choice of declination (2026-10-18): Néo-Lumen

Of the three declinations shown on the shared page (Néo-Lumen, Rouille &
Néon, Brume), the owner picked **Néo-Lumen**: pure cyberpunk on a wet
night, no brass and no rust (this answers question 2 of
`assets_rebrand_needs.md` §6; the steampunk side of the earlier direction
is dropped).

- Hero: Kit, a fox-kit courier (hooded jacket, waterproof satchel).
- Cast: an owl who tends the noodle bar, a raccoon hacker, a pangolin
  guard, a frog mechanic.
- Foes: the security machines (one-eyed drones, patrol automatons,
  robot insects), so an enemy always reads as an enemy.
- One city in five districts: Rooftops (platformer), Low street
  (top-down RPG), Canal (reflections, water, 256 colours), Sky (shmup,
  Mode 7), Arcade (breakout, puzzle).
- Palette, 32 colours on the 15-bit grid, muted neons rather than loud
  ones: ink/night `#080810 #101828 #182838 #203048`, teal
  `#183840 #205058 #307078 #48a0a0`, violet
  `#281838 #402858 #604080 #8060a8`, steel `#384050 #586878 #8898a8`,
  Kit's fur `#702810 #b04818 #e07830 #f0b880`, magenta neon
  `#601838 #a02860 #e05090`, cyan neon `#186070 #28a0b0 #70e0e0`,
  amber `#684018 #b07028 #f0b050`, mist `#485868 #7888a0 #b0c0d0`,
  highlight `#e8e8f0`.
- Risk named on the page: the closest to cyberpunk seen elsewhere, so the
  animals and the details have to carry the identity.

### The direction validated on the console (2026-10-18)

Three trials, about 74 generations, one ROM after each (built outside the
repository, filmed with `luna record`): Kit with an idle, a walk, a run
and a jump, roofs drawn in the city's style, the generated city in
parallax, the owl on the next roof. The owner played the third one:
"c'est joli et fun… une super étape pour valider et prendre cette
direction". Néo-Lumen and the PixelLab method are taken; the next work is
the examples themselves, wave by wave (`assets_rebrand_needs.md` §5).

What the owner hit while playing, and why, so the examples do not repeat it:

- **A frame of another character for four frames on each landing**: the
  jump had been cut to 8 frames and the code still asked for the ninth,
  which was the owl's block in the same OBJ sheet. An example's frame
  table must come from the sheet (`opensnes-sprite anim` or a generated
  header), not from numbers typed in `main.c`.
- **A roof out of reach**: walking at 1 px a frame, a 33-frame jump
  covered 33 px for a 32 px gap and a roof 16 px higher. Jump reach is a
  number to compute from speed, impulse and gravity when the level is
  laid out, and the movement manifests should check it.
- **Lag**: not the ROM (595 loop turns, one per frame, measured with
  `luna state --peek-at WaitForVBlank`); the GUI fell to 8–16 fps while
  the machine ran a dozen `rustc` at a load of 17 on 8 cores.

### Mode 7: a ground in 256 tiles (2026-10-19)

The four Mode 7 examples (perspective, dsp1_ground, perspective_rotate,
rotate_scale) took the city from above. What was learnt:

- **A generated picture does not reduce to 256 tiles.** `aerial.png` has
  3448 distinct 8x8 tiles out of 4096: nothing in it sits on the 8-px grid,
  so even its repeated helipads differ tile by tile. Clustering to 256
  medoids (each cluster replaced by a real tile of it, weighted by use)
  kept the streets and smeared the roofs and the plaza.
- **A Wang tileset does.** `create-tileset` (standard, 16 px, 16x16 crops
  of the aerial view as lower and upper references, the whole picture as
  `color_image`) gave street→roof and, chained on the same street tile,
  street→canal: 72 tiles of 8x8 for the whole road network, in the
  picture's own colours (9). The pieces that make it a place (the plaza at
  full size, two helipads, a skylight) are cut from the aerial view and
  stamped on the 16-px grid, so each stamp costs its tiles once. Result:
  235 tiles, 16 colours, `devtools/art_assets/mode7.py`.
- **A layout rule for corner Wang sets**: two upper terrains never meet,
  so the canal has a street on each side (vertices 15 and 18 around water
  on 16-17); the generator asserts it per cell.
- **The sky shares the ground's palette.** perspective and dsp1_ground draw
  a 4bpp sky that reads CGRAM 0-15, which the 8bpp ground palette fills: the
  sky's colours are written first and the ground's appended. The city's art
  all being on the same 15-bit palette, the ground added two colours.
- **Sprite order, not priority**: rotate_scale drew the shadow (OAM 0) over
  the glider (OAM 1) while its comment said the opposite; the lower index
  wins between sprites (snesdev-wiki *Sprites*, chunk c2d3d31a61323196). It
  showed only once the glider was a real picture.

### The race on the rooftops (2026-10-20)

mode7_racing went from a green rounded rectangle and an arrow built in C
to a hover-bike race round a lagoon, with two rivals. What was learnt:

- **`create-tileset` checks two things the docs do not say**: in
  `standard` mode it refuses `target_palette` (422, "works with mode='pro'
  only"), and its `color_image` must be 64x64 (a 17x1 strip failed the job
  with "Expected image of size 64x64 ... got 68 RGBA bytes"; the job then
  reads `failed` and `GET /tilesets/{id}` answers 410). A 64x64 image of
  palette stripes works.
- **References tame the texture**: asphalt asked in words only came out
  with white lane lines in random places (seed 7200); 16x16 noise in the
  wanted colours as lower and upper references gave plain asphalt and a
  clean red and white curb (seed 7201).
- **Chaining across the two roles**: the gravel is the *upper* terrain of
  the first set and the *lower* of the second; passing the first set's
  upper terrain id as the second's `lower_base_tile_id` gave a pixel-equal
  gravel tile in both.
- **One request, many variants**: `generate-image-v2` at 32x32 returns 64
  candidates, which for the bike came in six colour schemes; Kit and the two
  rivals are three of them, 11 colours together, so one OBJ palette.
- **Turned sprites without a rotation tool**: PixelLab's rotations turn a
  character between camera directions, not a top-down sprite in the plane.
  RotSprite does that well at 32x32 (Scale2x three times, turn without
  filtering, back by the commonest colour of each 8x8 block); 0 to 90
  degrees in five frames, and the OAM flips give the sixteen directions.
- **Props on the 16-px grid** cost their tiles once: six props scattered
  over the gravel took the map from 138 to 227 tiles.

### extbg: the orb under the metro (2026-10-21)

The last Mode 7 example on the old art. The plane and the ball were built
in C at run time (a checkerboard, grey pillars, `tileEncode4bpp`); they
are now two converted assets (`res/plane.png`, `mode = 7`, `colors = 144`;
`res/orb.png`, 16x16 sheet) and main.c only uploads them (the `tile`
module is gone from the example). What made it simple: `opensnes-tileset`
keeps the indices of an indexed PNG, so the viaducts' pixels are written
0x81-0x8D in the picture and arrive as such; the only rule is the
palette's (BG1 reads all eight bits, so CGRAM 129-141 repeat 1-13, and the
palette stops at 143 under the orb's OBJ palette). The boot is shorter
(nothing built at run time): the manifest's bound on the orb's x at frame
100 went from 96 to 160.

### breakout becomes Neon Breaker (2026-10-21)

The game addresses its tiles by number (a brick is tiles 13-14 in one of
eight palettes, the font ASCII − 10, the sprites 15-21 of the BG sheet
read through the second name table at word $1000), so the new art keeps
that numbering and the game's code is unchanged but for the asset names:
`breakout.py` draws a 128-tile sheet whose PNG palette is the whole CGRAM
image, converted with `no-reduce` (`tiles.pal` is the old palette.dat),
BG3's 13 2bpp tiles (`bpp = 2`, `no-palette`), and the two maps and the
level colours as binaries. The seven PVSnesLib .dat files and their
preview PNGs are gone. One trap: CGRAM 16-23 (BG3) must hold the first
level's colours in the palette itself, as the game copies a level set only
from the second level on. Both manifests pass unchanged.

## Open questions for the owner

- Identity: a mascot, which world.
- Tool and budget: PixelLab, Retro Diffusion, or both compared on the
  player sprite (a few dollars).
- Music: commission a musician, or free modules.
- Licence of the new assets: CC0 or CC-BY.
- The rebranding beyond the assets: name, logo, colours of the site.

## Sources

- PixelLab: https://www.pixellab.ai/, https://www.pixellab.ai/pixellab-api,
  https://api.pixellab.ai/v2/docs, https://www.pixellab.ai/termsofservice,
  https://mcpservers.org/servers/pixellab-code/pixellab-mcp
- Retro Diffusion: https://mcpservers.org/servers/retro-diffusion/retro-diffusion-mcp
- Copyright frame: https://blog.promise.legal/ai-game-assets-copyright-steam-disclosure-2026/

### likemario becomes a game (2026-10-20)

The owner found the demos were demos and asked for the platformer with Kit as
a real game. It has a title, 29 shards, four robot insects and two drones
(stomped from above, fatal from the side), three lives, a checkpoint per roof,
a beacon to reach and a game over, and went back to `examples/games/`. What
was learnt:

- **A dynamic sprite's slot is its draw order.** The engine hands out its
  32x32 slots in the order of the `oamDynamicDraw()` calls of a frame; a
  machine skipped off screen shifts every later sprite onto someone else's
  tiles. Draw them all every frame, hidden when unseen.
- **A 32-px sprite at y = 240 is not hidden**: sprites wrap at line 256 and
  its bottom 16 rows show at the top. `oamHide()` also sets the ninth x bit;
  a raw `oamy` must be 224 (below the 224 lines).
- **Static sprites beside the engine** work in the same OAM: the engine
  hides only the entries it drew the frame before, and its last slots' tiles
  are free when it uses fewer than twelve.
- **The 2 KB text map in the VBlank** fits beside seven sprite uploads and
  a map column: the three manifests' DMA asserts stay at zero unsafe writes.
- **A greedy search finds the playthrough.** RIGHT held, a press at a time,
  keeping the earliest press after which Kit has his three lives and is
  furthest right 70 frames later: ten presses, 22 shards, two stomps. A
  140-frame horizon got stuck, because the jump over a gap leads straight
  into the next insect and only a second press can stomp it.
- **The scratchpad is not durable**: `/tmp` was emptied twice during the lot.
  Helpers that a long search needs go to `~/.cache`.
