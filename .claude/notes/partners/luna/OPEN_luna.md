# OpenSNES → luna: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item. Every item is re-checked on the pinned
luna the day the report goes out (`.claude/rules/partners.md`); the pin is
v1.33.1 (2026-10-06).

Re-checked on v1.33.1 on 2026-10-06: the bare static name still answers
« not a loaded symbol and not BANK:OFFSET » (`player_x` for `player_x.main`
on the `opensnes init --template game` ROM); `--power-on` still lists WRAM,
VRAM, CGRAM, OAM and APU RAM only (`luna state --help`); the v1.33.1 release
has four zips and no `.sha256` (we pin the sums in `testing/luna.sha256`).
The MS0 / 21 MHz and DSP-1 2 MB items were not re-run (no change named in the
1.33.0 and 1.33.1 notes).

What v1.33.0 made us find: its mosaic fix (the grid one line lower) moved
none of our baselines — no oracle of ours hashed a frame with mosaic on.
`testing/manifests/transition_mosaic_picture.toml` now does (v1.33.1
`9980548a31063b25`, v1.32.0 `b40b834549507f25`).

| Date | Item | Seen on | What we would run |
|---|---|---|---|
| - | (empty: the audio comparison request went out on 2026-10-03 in `2026-10-03_to_luna_messages-region_reply.md`, §2) | - | - |
| - | (done 2026-10-03 at the v1.32.0 pin: the three runs of the diff-audio reply replayed on the pinned binary, rule written in `testing.md`) | | |
| - | (closed by v1.33.0: `--dsp-trace` timestamps; re-run 2026-10-06 on v1.33.1, `examples/audio/echo` to frame 300: 92 rows, none at 0, first row `95632,$6C,FLG,$20`) | | |
| - | (closed by v1.33.0: `rom.checksum_computed`; re-run 2026-10-06 on v1.33.1, `print_string.sfc` with byte $0100 set to $5A: `checksum` 41211, `checksum_computed` 41285 — the figure `opensnes-rom inspect` gives, $A145. `luna_runner.py` compares the two fields on every ROM since the pin) | | |
| 2026-10-05 | `--power-on random` fills « WRAM, VRAM, CGRAM, OAM and APU RAM » (`luna state --help`, v1.32.0) and not the cartridge's RAM: the Super FX Game Pak RAM (framebuffers, save area), the SA-1 BW-RAM and I-RAM start clean, so a read of an uninitialised framebuffer or I-RAM byte cannot show on luna (chips audit G, 2026-10-03). Needed: the same fill on cartridge RAM (or a `--power-on-cart` switch), with the same seed | v1.32.0 | `luna state examples/chips/superfx_3d/superfx_3d.sfc --until-frame 60 --power-on random=1 --peek 70:0000:20` — bytes read `00` |
| 2026-10-05 | No diagnostic when CFGR bit 5 (MS0, fast multiply) and CLSR bit 0 (21 MHz) are both set: « MS0 must be zero in 21MHz mode » (fullsnes `1adef8e33ff3c4e9`; ares `06c6d2324e3c6d01`: products « may sometimes be invalid » in that mode). Our launchers mask the bit, but a program writing CFGR itself runs green on luna and may multiply wrong on a console. Needed: a counter or a note in `state.gsu` (like `bus_violations`) when a MUL/FMULT executes with both bits set | v1.32.0 | a ROM writing `$A0` to `$3037` then `$01` to `$3039` and running `fmult`; `luna state … --out - \| jq .gsu` shows `cfgr: 160, clsr: true` and nothing else |
| 2026-10-05 | A DSP-1 LoROM of 2 MB (`ROM_BANKS=64`, `USE_DSP1=1`, before our build refused it) loads and runs on luna while the only 2 MB DSP-1 LoROM board (SHVC-2B3B-01) maps the DSP registers elsewhere (chips audit S5). Low priority: `make` refuses the combination since 2026-10-04 (`ROM_BANKS_MAX` 32 for DSP-1); an emulator warning on header-vs-board inconsistencies would still be a service | v1.32.0 | build with `ROM_BANKS=64 USE_DSP1=1` on a tree before `a8113dd8`, `luna state` boots it without a word |
| 2026-10-05 | The five releases were re-published on 2026-10-04 (21:09-21:17 UTC) with another layout: `luna_<v>_<os>_<arch>.zip` (`darwin`, `arm64`) with a top directory of that name, for every OS, in place of `luna-<v>-<macos\|linux>-<aarch64\|x86_64>.tar.gz`; and **no `.sha256` sidecar any more** (the release body's download table does not mention sums). The binaries are byte-identical to the ones we had (`luna` `22ee12a5…`, `luna-gui` `1a151f30…` on linux arm64), so nothing moved for the tests, but `scripts/install-luna.sh` fetched a name that no longer exists and `make tests` went red on a clean tree until we rewrote it: the four archive sums of the pinned version are now kept in `tools/luna-test/luna.sha256` and re-read at every pin bump. Asked: publish a `SHA256SUMS` (or `<zip>.sha256`) asset with each release, and say in the release notes when the asset names change — a pinned consumer only sees the 404. | `gh release view v1.32.0 --repo k0b3n4irb/luna --json assets` (4 zips, no sums); `curl -sI https://github.com/k0b3n4irb/luna/releases/download/v1.32.0/luna-v1.32.0-linux-aarch64.tar.gz` → 404 | `scripts/install-luna.sh` on a tree without `tools/luna-test/bin/` |
| 2026-10-05 | A bare symbol name does not resolve to its unique suffixed static. Since the duplicate-static fix in cproc, a file-scope `static` reads `name.<source>` in the `.sym` (`player_x.main`); `luna test` with `assert = ["player_x = 7800"]` answers « unknown symbol `player_x` (and not BANK:OFFSET=HEX) » while `player_x` has exactly one match, `player_x.main`. We renamed the twenty manifests, the two lib fixtures and the project template (`"player_x.main"`), but a game developer writing their first manifest will type the C name. Asked: when the exact name is absent and exactly one label is `name.<something>`, resolve it (and keep refusing when two sources define it, naming both). Negative control: `k.main` and `k.other` in `testing/fixtures/compiler/static_dup` — `k` must stay ambiguous. | luna v1.32.0, `luna test` and `--assert` (`make test-project` output in the suite log) | `luna test test/manifest.toml` with `assert = ["player_x = 7800"]` on the `opensnes init --template game` ROM |
