# Hardware verification protocol {#hardware_verification}

luna is cycle-accurate and runs every example, every chip and every
peripheral the corpus uses, and each hardware claim in these docs is checked
against a corpus of hardware documentation before it is written down. None of
that is a Super Nintendo. This page is the last step: a repeatable session on
a real console, with a fixed list of ROMs, one thing to look at per ROM, and a
grid to fill in. A release is "hardware-verified" when the grid for its tag is
complete and every row is OK or explained.

The protocol is written for a flash cart (FXPak Pro, formerly sd2snes) because
that is what the maintainers own. Any cart that runs plain `.sfc` files works
for the rows that need no enhancement chip.

## What you need

| Item | Notes |
|---|---|
| A Super Nintendo or Super Famicom | Record the model (SNS-001, SNS-101 / 1CHIP, SHVC-001) and the region. Timing rows differ between NTSC and PAL, and the 1CHIP revision has a different video output stage. |
| A flash cart | Record the firmware version. The sd2snes / FXPak family runs the DSP-series chips, and SA-1 and Super FX as FPGA cores since firmware 1.10.x, on Mk.II and Pro: the v1.10.3 notes (April 2019) fix "SA-1 and SuperFX RAM write cycles" on both ([sd2snes blog](https://sd2snes.de/blog/archives/date/2019/04)). Those cores are reimplementations: confirm a chip-row failure on a second reference before blaming the SDK. |
| A display | Record what it is and how the console is connected (composite, S-Video, RGB, an upscaler and its model). A CRT is the reference for anything about timing or interlace. A light gun generally needs a CRT; on an LCD, mark the Super Scope row as not testable rather than failed. |
| Two pads | Port 2 is used by the two-player and Super Scope rows. |
| Optional | A SNES Mouse, a Super Scope. |
| The ROMs | `make hardware-kit` collects the ROMs of this protocol into `release/hardware-kit/`, numbered in the order of the grid. Copy that folder to the SD card. Build from a clean tree at the tag you are verifying (`git checkout vX.Y.Z && make clean && make`). |

Record the tag or commit the ROMs were built from. The grid is meaningless
without it.

Before the session, `make hardware-preflight` replays the same ROMs on luna
from pseudo-random RAM (three seeds) and under PAL, and checks that every
VRAM DMA byte lands in blank: the two cheapest ways a ROM green on luna's
defaults fails on a console. A row that fails there is fixed before it is
carried to the hardware; `ROWS=1-7` restricts the run to the gate rows.

## How to run a row

1. Boot the ROM. Wait for the picture to settle (two seconds is plenty).
2. Compare with what luna shows. Each example's `README.md` carries the
   screenshot luna captured; the same picture is what the visual regression
   baseline holds. Look for the specific thing in the "Check" column, not for
   pixel identity — a CRT, an upscaler and a PNG never match pixel for pixel.
3. Do the input in the "Do" column, if any.
4. Write OK, KO or n/a in the grid, with a note for anything that is not
   plainly OK: what you saw, and a photo or a phone video if the symptom is
   visual or audible. A KO with no description cannot be acted on.

Power-cycle between rows that touch SRAM or the audio driver; leaving a ROM
running and loading the next one through the cart menu is fine for the rest.

## The rows

The list is a triage, not the corpus. Each row is there because it exercises
something luna cannot vouch for on its own: the analog output stage, power-on
memory contents, real timing of DMA and IRQ against the beam, a peripheral, a
coprocessor, or battery-backed memory. A row that fails points at a class of
behaviour, and the note under the table says which.

| # | ROM | Do | Check |
|---|---|---|---|
| 1 | `text/print_string` | nothing | Text legible, stable, no rolling. This is the boot smoke test: font upload, palette, screen on. |
| 2 | `input/controller` | press every button on pad 1, then hold two at once | Each name appears while held and disappears on release. Pad 2 is checked by row 14. |
| 3 | `scrolling/parallax_scroll` | watch the top line | The first visible line must be picture, not a strip of garbage or a line from the bottom of the map. This is the vertical-scroll-by-one fix (`KNOWN_LIMITATIONS.md`, "the PPU never outputs scanline 0"); emulators forgive it, the console does not. Layers scroll at different speeds without tearing. |
| 4 | `sprites/sprite_swarm` | watch the densest moments | Sprites flicker or drop out only where more than the per-line budget overlap; no garbage tiles, no sprite stuck at the top-left corner. |
| 5 | `hdma/hdma_wave` | watch, then A to switch to the hdma module, LEFT/RIGHT for the amplitude | A smooth horizontal sine flowing up the water image, no horizontal tearing, no line where the wave jumps — in both modes, and across the switch. |
| 6 | `windows/window_multi_hdma` | D-pad scrolls the artwork | The window shapes keep their edges while scrolling; nothing shows through the mask. |
| 7 | `color/hicolor_1792` | nothing | A vertical gradient of many more than 256 colours, with no bands of wrong colour. This is a general DMA into CGRAM on every scanline, driven by an H-IRQ: the strictest timing row in the list. |
| 8 | `backgrounds/mode5_hires` | nothing | 512-pixel-wide text is sharp and interlaced without judder. On an upscaler, try its 480i/interlace setting if the picture is unstable, and say so in the note. |
| 9 | `mode7/perspective_rotate` | D-pad moves, L/R rotate | The plane recedes to a horizon; rotation keeps the horizon straight; no glitch line at the top of the plane. |
| 10 | `transitions/fading` | any button steps through the effects | Fades are smooth to black and back; no colour jumps in the last steps. |
| 11 | `audio/snesmod_music` | A plays, B stops, START fades out | Music starts within a second, no crackle, no note hanging after stop. Power-cycle before this row: the SPC upload must work from a cold APU. |
| 12 | `audio/sfx_from_wav` | press the buttons | Each sound fires once per press, no click at the start, no click at the end. |
| 13 | `memory/save_game` | A writes slot 1, power-cycle, B reads slot 1 | The values read back are the ones written before the power cycle. Then X writes slot 2, power-cycle, Y reads it. On a flash cart this also checks that the cart writes the save file back; consult its manual if slot 1 reads blank. |
| 14 | `input/two_players` | move both pads | Each pad drives its own object; releasing pad 2 does not affect pad 1. |
| 15 | `memory/hirom_demo` | hold A | "HIROM MODE" is shown and the background turns light blue while A is held. |
| 16 | `chips/dsp1_cube` | nothing | A wireframe cube tumbles with correct perspective. Firmware-dependent: the DSP-1 is emulated by the cart, and the FXPak / sd2snes runs it only with the DSP ROM dumps (`dsp1.bin`, `dsp1b.bin`, …) in its `sd2snes` system folder ([sd2snes downloads page](https://sd2snes.de/blog/downloads)); without them the row shows nothing, which is the cart, not the SDK. |
| 17 | `chips/sa1_starfield` | nothing | 128 dots trace smooth Lissajous figures at full frame rate. Firmware-dependent. |
| 18 | `chips/superfx_3d` | nothing | A wireframe cube rotates at full frame rate with no missing edges. Firmware-dependent. |
| 19 | `games/likemario` | walk with the D-pad, jump with A | Walking, jumping and landing feel like luna: no fall through the floor, no stuck-in-wall; camera follows without judder. |
| 20 | `games/rpg` | walk, talk to a villager with A, open the chest | The full game path: map, dialogue, chest state. |
| 21 | `input/mouse` | move the mouse, click both buttons, right-click cycles sensitivity | Optional (needs a SNES Mouse, port 1). The cursor tracks the hand at each of the three sensitivities. |
| 22 | `input/superscope` | calibrate, then fire at a target | Optional (needs a Super Scope in port 2 and, in practice, a CRT). The red dot lands where the scope points. |
| 23 | `backgrounds/mode6` | press B, photograph the screen | A question, not a pass/fail: does a horizontal offset of 8 move a hi-res column? Compare with `examples/backgrounds/mode6/mode6_card.png` (luna's capture; ares and bsnes compute the same): every odd column shifted half a tile, so the diagonal steps are cut into half steps. If instead the bands climb in clean whole-tile steps, the console drops bit 3 like Mesen2. Either answer goes in the note with the photo. |
| 24 | `chips/superfx_game_skeleton` | hold the D-pad, listen | The crosshair follows the pad at full frame rate while the GSU's cube turns at half rate and the music plays: no torn or half-drawn cube (`gsuPresent` double-buffers and the NMI swaps in blank), no pause in the music while the GSU runs from its cache. Firmware-dependent, like rows 16-18. |
| 25 | `chips/sa1_save` | read NOW, power-cycle, read again | The SA-1 save path: NOW is one more after every power cycle and FOUND shows the previous value (BW-RAM, battery). A NOW that stays at 1 means the cart's SA-1 core does not keep BW-RAM across power, or the header's `$35` is not honoured. |
| 26 | `chips/superfx_save` | read NOW, power-cycle, read again | The Super FX save path: same check in the GSU's Game Pak RAM at `$E000` (header `$15`, size at `$FFBD`). Firmware-dependent, like rows 16-18 and 24. |

**What a failure in each row points at.** Rows 1 and 2 failing means nothing
else is worth running: the boot path or the joypad read is wrong. Row 3 is
the one known difference between emulators and the console that the lib
compensates for; if it fails, the compensation is being bypassed somewhere.
Rows 4 to 10 are the PPU under load: a failure there is a timing claim in
the lib (HDMA table shape, IRQ position, DMA length) that luna and the console
disagree on — query the corpus before touching code
(`.claude/rules/hardware_claims.md`). Rows 11 and 12 are the APU path,
including the cold-boot upload. Row 13 is the only test of battery-backed
persistence the project has. Rows 16 to 18 and 24 to 26 depend on the cart as much as on
the SDK; a failure needs the firmware version and, if possible, a second cart
before it is filed against the SDK. Rows 19 and 20 are the integration rows:
they fail last and tell you least, but they are what a user will run first.
Row 23 asks the console a question no reference answers and emulators answer
differently (bit 3 of a hi-res offset-per-tile offset, see the example's
README); whichever way it comes out, the photo goes to the luna and snes-rag
reports.
Row 24 is the Super FX presentation path (`gsuPresent`, the code cache) with
SNESMOD running at the same time; a torn cube is a blanking claim of the
presenter, a music pause is the GSU holding the ROM bus.

Rows 25 and 26 are the two chip save paths (BW-RAM, Game Pak RAM): each
example counts its own power-ons, so the check is a number that grows by
one per power cycle, with the previous value shown next to it.

## The grid

Copy this block into the results log below, one per session.

```
Session:       YYYY-MM-DD
Tag / commit:  vX.Y.Z (sha)
Console:       model, region
Cart:          FXPak Pro, firmware x.y.z
Display:       what, connection
Peripherals:   pads / mouse / scope

 #  ROM                         Result  Note
 1  text/print_string           OK
 2  input/controller
 3  scrolling/parallax_scroll
 4  sprites/sprite_swarm
 5  hdma/hdma_wave
 6  windows/window_multi_hdma
 7  color/hicolor_1792
 8  backgrounds/mode5_hires
 9  mode7/perspective_rotate
10  transitions/fading
11  audio/snesmod_music
12  audio/sfx_from_wav
13  memory/save_game
14  input/two_players
15  memory/hirom_demo
16  chips/dsp1_cube
17  chips/sa1_starfield
18  chips/superfx_3d
19  games/likemario
20  games/rpg
21  input/mouse                 n/a
22  input/superscope            n/a
23  backgrounds/mode6
24  chips/superfx_game_skeleton
25  chips/sa1_save
26  chips/superfx_save
```

## What to do with a KO

1. Reproduce it once more on the console; power-cycle first. Real RAM does
   not come up zeroed, and a symptom that appears only on some boots is a
   power-on-state bug — run the example in luna with `--power-on random=1`
   and a few other seeds before anything else.
2. Check whether luna shows it with `luna state --screenshot` at the same
   moment. If luna shows it too, it is an ordinary bug: file it, fix it. If
   luna does not, it is either a hardware behaviour luna does not model or a
   wrong claim in the lib: query the corpus with the symptom
   (`snes_search`, `.claude/rules/hardware_claims.md`), and only then
   decide which.
3. Open an issue with the grid header (console, cart, firmware, display),
   the row, the note and the photo or video. A luna-side discrepancy is
   reported to the luna project with the same material.

## Results log

No session recorded yet. The first one is a v1.0 must-have (`ROADMAP.md`).
