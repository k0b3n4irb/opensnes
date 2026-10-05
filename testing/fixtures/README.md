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

## luna stress ROMs (`stress/`)

Hardware corners that pin luna itself: see [`stress/README.md`](stress/README.md).

## Bench (`benchrom/`)

Not a test: the cycles-per-call instrument of the C1 audit
(`lib/ARCHITECTURE.md`) and the far-deref cost of B2 (`benchrom/b2_deref/`),
run by hand with each directory's `bench.py`. Not in the fixture lists.

Build artifacts (`*.sfc`, `*.sym`, `*.o`, `*.wrap.asm`, `.opensnes_config`)
are gitignored; only sources, Makefiles, tests and manifests are tracked.
