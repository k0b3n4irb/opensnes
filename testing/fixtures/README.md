# testing/fixtures — the ROMs that assert on the library, the compiler and luna

Twenty single-purpose ROM projects (a `main.c`, a `Makefile` over
`make/common.mk`, sometimes an `.asm`), gathered here on 2026-10-05 from three
roots. The root `Makefile` holds the one list (`FIXTURES_LIB`,
`FIXTURES_COMPILER`, `FIXTURES_STRESS`): `make fixtures` builds them all,
`make tests` rebuilds the asserted ones clean and runs their `test_*.py`
(`FIXTURE_TESTS`), `make test-manifests` runs the ones a luna manifest reads,
`make rom-coverage` profiles them with the corpus. A fixture's test imports
`testing/lib` (`from lib import find_luna, assert_mem`).

## Library (`libtests*`)

| Fixture | Pins | Asserted by |
|---|---|---|
| `libtests/` | console, sprite, dma, background, text, math, anim, map, audio, fixed32, collision, window, input, object, colormath, mosaic, profile, scene, tile, math_ease; 40 rows of text; the full 32 bits of `fix32Sin`; `USE_SRAM` in LoROM | `test_libtest.py` (also `--region pal` in `make test-pal`); `audio_v2.toml`, `input_pad_unplugged_*.toml` |
| `libtests_fx/` | hdma, mode7, SNESMOD, `nmiSet`, an IRQ armed before the driver boots (the first fixture has no bank-$00 RAM for the first two and boots the other audio driver) | `test_libtest_fx.py` |
| `libtests_dsp1/` | the DSP-1 commands no example calls, on luna's real firmware; SKIPs without the user-supplied `dsp1b.rom` (CI) | `test_libtest_dsp1.py` |
| `libtests_hirom/` | the HiROM map: the sram module's HiROM mapping, the bank byte of a pointer to RAM under `.BASE $C0` (a wlalink fix, 2026-09-20) | `test_libtest_hirom.py` |
| `libtests_sa1_sram/` | the sram module on SA-1: BW-RAM at `$40:0000`, writable once crt0 sets SBWE (2026-09-26) | `test_libtest_sa1_sram.py`; `d_`/`e_sa1_bwram_*.toml` power-cycle chain |
| `libtests_gsu/` | the Super FX job path: cache-resident job, RAM job, ROM job, a save while a job runs | `libtest_gsu_cached*.toml`, `f_`/`g_gsu_save_*.toml` |
| `libtests_dma_channels/` | the DMA channels the library takes: a Super FX letterbox, the game's HDMA on 2-6, the NMI's text flush and dynamic sprite uploads together; the channels' registers read back | `libtest_dma_channels.toml` |
| `libtests_region/`, `libtests_region_pal/` | the region a ROM declares (header byte $FFD9, `ROM_REGION`) and the one it runs on (`isPAL()`), read back; the PAL twin includes the same `main.c` | `libtest_region_ntsc.toml`, `libtest_region_ntsc_on_pal.toml`, `libtest_region_pal.toml` |
| `libtests_bigrom/` | a 4 MB LoROM image (`ROM_BANKS=128`): a const table in bank 127 (`$FF:8000`) read back, the header size byte `$0C` | `libtest_bigrom.toml` |
| `libtests_aram/` | the sound CPU's memory. A sample loaded in steps (`audioLoadSampleBegin` / `Step`): refusals, a voice command and another load between two steps, the bytes and the directory entry read back, a load given up. The echo ring's place: refused when 48 KB of samples reach into it, accepted when they do not, a shorter delay giving memory back; the sample still loaded whole after a 12 KB ring is replaced by a 2 KB one (the driver's wait) | `test_libtest_aram.py` |
| `libtests_snesmod/` | the SNESMOD stop / pause / fade queue and what the driver leaves of the machine | `libtest_snesmod.toml` |

## Compiler (`compiler/`)

Runtime proofs of the compiler chantiers; the pattern checks in
`devtools/compiler-tests/` prove shapes, these prove results.

| Fixture | Pins |
|---|---|
| `a6_farptr/` | the 4-byte pointer ABI: the bank byte through every lib boundary |
| `a7_32bit/` | 32-bit arithmetic: add/sub carry, mul, div, mod, shifts, signed folds |
| `b2_far_ram/` | `FAR` objects in bank $7E with bank-honouring codegen |
| `bswap/` | the byte swap of a 16-bit word as one `xba`: `(w << 8) \| (w >> 8)` in both orders, from a FAR read and on a global in place, swept over the 65 536 values of w against the swap built without the shape and a host checksum; the shapes left alone (a signed `>>`, 32 bits, the masked idiom) still right; a branch on the swapped value; the provoked fault (`QBE_BSWAP_FAULT=1`, a `sar` taken) makes the s16 form wrong, `QBE_NO_BSWAP=1` keeps the values and loses the `xba` |
| `far_bank7f/` | `FAR` objects in bank $7F placed by assembly: every access form, `memcpy`/`memset`, no overlap |
| `c_features/` | switch forms, function pointers, struct members, the C features a game uses |
| `const_div/` | a divide or remainder of two constants folded at compile time (signed and unsigned, 16 and 32 bits, negatives, limits, a numerator folded just before): no division helper called, every value against one written by hand; INT_MIN / -1, INT_MIN % -1 and a zero divisor kept as run-time calls |
| `debug_channel/` | the debug channel to luna (nocash / WDM) |
| `d_quals/` | `volatile`, `const` and the qualifiers' codegen |
| `eq_swap/` | an equality compared from the operand the accumulator already holds (`lda.w g / and.w #$00FF / cmp T`, not `sta T0 / lda T / cmp T0`): `==` and `!=` of a FAR byte and a global byte, fused with the branch and materialised, the store kept when the operand is read again, the ordered compares `<` / `>=` left in place; 256 values of g against 256 bytes, against the same results computed without the shape and a host checksum; the provoked fault (`QBE_EQ_SWAP_FAULT=1`, an unsigned `<` / `>=` swapped) makes them wrong, `QBE_NO_EQ_SWAP=1` keeps the values and brings back the store and reload |
| `farptr_dp/` | far pointers read in their direct-page slot and stepped on 16 bits: two pointers walked in turn, a walk to the last byte of an object placed at `$7E:FFF0-$FFFF`, a const table, a plain array, a walk backwards, against a host model; the refusals (a pointer compared with the end of that object, an extern object assembly puts across `$7E:FFFF`), each turned red by its provoked fault (`QBE_FARPTR_FAULT=1`, `=2`) |
| `g1_idioms/` | the G1 idioms I1, I2, I3 and I4 (`compiler/qbe/idiom.c`, behind `OPENSNES_G1_IDIOMS=1`, off by default). I1: the extension of `s >> k` taken as the 32-bit `sar` of the extension of s, in a path follower's loop, for k = 1..15, in a chain of two shifts and with the extension in a dominating block. I2: a loop index used only as `2*v` carried scaled (entry scaled at 32 bits, step and wrap mask scaled, `v = j >> 1` after the loop), read through a FAR pointer so an index of `$8000` or more reaches the next bank: masked, mask keeping bit 15, the bare 16-bit wrap up and down across `$8000`, a `sub` step, two exits, no trip, constants. I3: a loop whose trip count is a constant of at least 1 entered at its body, the test at the bottom: counter used or not, signed, a step of 7, wrapping past `$FFFF`, counted down, one trip, nested, seven loops of `main`; a loop with a break and one that never runs are left alone. I4: a counter used only for the trip count counted down to zero from that count: signed from -5, a step of 3, counted down, wrapping past `$FFFF`, `<=`, a second exit on the data, a loop that never runs (count 0). Values against a host model, the switch off then on (the trace names every rewritten function and its idiom), and each provoked fault turns its values red: `OPENSNES_G1_FAULT=1` (a logical shift, high word 0), `=2` (the entry scaled at 16 bits), `=3` (the trip count not looked at: the loop that never runs is rotated and runs once), `=4` (signed compares read as unsigned when counting: the counter from -5 counts 0) |
| `inline_label_cut/` | the label of a copied block cut where the assembler reads a size suffix: three levels of `static inline` copies whose 40-character cut ends in `.b`, `.w` and `.l` (repaired with a trailing `_`), values against a host model in the main loop and the NMI callback; `QBE_INLINE_FAULT=label` makes the compiler refuse the label, and with `QBE_LABEL_FAULT=noguard` as well the link fails |
| `jump_table/` | a dense `switch` as a jump table: every shape the recogniser takes (from 0, from 10, negative and wrapping, a byte, no default with phi moves, shared labels, fallthrough, a default in the middle, the value read after, in a loop indexed by its counter, on a loop counter) or refuses (three cases, sparse, 32 bits, a dense cluster in a sparse switch), swept and checksummed against a host model; one sweep in the main loop and in the NMI callback |
| `leaf_frame/` | a leaf's direct-page frame (`tcc__lf`): the same leaf in the main loop and in the NMI callback, no wrong result on either side |
| `leaf_split/` | leaves above the 20 words of `tcc__lf`, split between it and a stack frame: results against a host model, and the main loop against the NMI callback |
| `static_dup/` | two sources each defining `static u16 k` and `static u16 tag()`: the link succeeds and each reads its own (statics are emitted `name.<source>`) |

Each is rebuilt from clean before its test: a stale `.sfc` built with an
experimental toolchain once produced misleading XPASSes (a6_farptr,
2026-07-04). The clean and the build are separate `make` invocations on
purpose — this Makefile exports `-j`, and `clean all` in one command ran
both goals concurrently (clean deleted `crt0.o` mid-link).

## Examples that left the corpus (`retired_examples/`)

Eight examples left `examples/` before 1.0. Three in the examples review
(2026-10-15): they duplicated another example for a reader, but tests and
comparisons read them. Five in the assets rework (2026-10-19): chip boot
diagnostics and ROMs that ask the console a question, which a reader does not
need but the luna manifests and the console protocol
(`docs/HARDWARE_VERIFICATION.md`, rows 23, 25 and 26, by their path here) do. They are built by `make fixtures` and profiled by `make rom-coverage`
like the others; nothing of them is in the user documentation.

| Fixture | Read by | Its example now |
|---|---|---|
| `tiled/` | `dma_tiled.toml`, `movement_tiled.toml`; `devtools/twinbench` (PVSnesLib's maps/tiled) | `maps/map_scroll` (the same Tiled pipeline) |
| `metasprite/` | `footprint_metasprite.toml`, `input_metasprite.toml`, `oam_metasprite.toml`; twinbench (PVSnesLib's MetaSprite); the `anim.h` cost figure | `sprites/sprite_sizes`, `sprites/aseprite_pipeline` |
| `snesmod_music_large/` | `audio_snesmod_music_large.toml`, `input_snesmod_music_large.toml`; `audio_regress.py` | `audio/snesmod_music` (one call loads a soundbank of any size) |
| `mode6/` | `backgrounds_mode6.toml`, `backgrounds_mode6_card.toml`; protocol row 23 (the hi-res offset-per-tile question, `mode6_card.png`) | `backgrounds/mode2` (offset-per-tile in Modes 2 and 4) |
| `sa1_hello/` | `coproc_sa1_hello.toml` (the SA-1 boot and I-RAM handshake) | `chips/sa1_starfield` |
| `sa1_save/` | `h_sa1_save_boot1.toml`, `i_sa1_save_boot2.toml` (BW-RAM kept across a power cycle); protocol row 25 | the SA-1 tutorial, "Saving to BW-RAM" |
| `superfx_hello/` | `coproc_superfx_hello.toml`, `coproc_superfx_vectors.toml` (GSU boot, SRAM, FMULT) | `chips/superfx_3d` |
| `superfx_save/` | `j_gsu_save_boot1.toml`, `k_gsu_save_boot2.toml` (Game Pak RAM kept across a power cycle); protocol row 26 | the advanced Super FX tutorial, "Saving" |

## luna stress ROMs (`stress/`)

Hardware corners that pin luna itself: see [`stress/README.md`](stress/README.md).

## Bench (`benchrom/`)

Not a test: the cycles-per-call instrument of the C1 audit
(`lib/ARCHITECTURE.md`) and the far-deref cost of B2 (`benchrom/b2_deref/`),
run by hand with each directory's `bench.py`. Not in the fixture lists.

Build artifacts (`*.sfc`, `*.sym`, `*.o`, `*.wrap.asm`, `.opensnes_config`)
are gitignored; only sources, Makefiles, tests and manifests are tracked.
