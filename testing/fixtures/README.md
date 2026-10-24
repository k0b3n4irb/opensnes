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
| `c_features/` | switch forms, function pointers, struct members, the C features a game uses |
| `debug_channel/` | the debug channel to luna (nocash / WDM) |
| `d_quals/` | `volatile`, `const` and the qualifiers' codegen |
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
