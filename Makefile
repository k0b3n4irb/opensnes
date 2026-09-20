#==============================================================================
# OpenSNES SDK - Master Makefile
#==============================================================================
#
# Usage:
#   make              Build everything (compiler, tools, library, examples)
#   make compiler     Build compiler only (cc65816/QBE + wla-dx)
#   make tools        Build tools only (font2snes, etc.)
#   make lib          Build library only
#   make examples     Build all examples
#   make clean        Clean all build artifacts
#   make install      Install binaries to bin/
#
#==============================================================================

# Platform detection (parallel builds + release naming)
UNAME := $(shell uname -s)
UNAME_M := $(shell uname -m)
ifeq ($(OS),Windows_NT)
    MAKEFLAGS += -j$(NUMBER_OF_PROCESSORS)
    PLATFORM  := windows
    ARCH      := x86_64
else ifeq ($(UNAME),Darwin)
    MAKEFLAGS += -j$(shell sysctl -n hw.ncpu)
    PLATFORM  := darwin
    ifeq ($(UNAME_M),arm64)
        ARCH := arm64
    else
        ARCH := x86_64
    endif
else
    MAKEFLAGS += -j$(shell nproc)
    PLATFORM  := linux
    ifeq ($(UNAME_M),aarch64)
        ARCH := arm64
    else
        ARCH := $(UNAME_M)
    endif
endif

# Paths
COMPILER_PATH := compiler
TOOLS_PATH    := tools
LIB_PATH      := lib
EXAMPLES_PATH := examples
TESTS_PATH    := tests

RELEASE_DIR := release
VERSION ?= $(shell git describe --tags --abbrev=0 2>/dev/null)
ifneq ($(VERSION),)
    RELEASE_NAME := opensnes_$(VERSION)_$(PLATFORM)_$(ARCH)
else
    RELEASE_NAME := opensnes_$(PLATFORM)_$(ARCH)
endif

.DEFAULT_GOAL := all
.PHONY: all clean clean-examples install compiler tools lib examples cli tests test-compiler test-tools test-sanitizers coverage-host luna-bench test-toolchain-suites test-link-modules fuzz fuzz-replay test-manifests test-pal test-nmi-budget test-wram test-project rom-coverage bench budget asset-budget submodules verify-toolchain lint-commits lint-docs lint-asm-abi lint-vram lint-cppcheck lint docs docs-strict help release clean-release hardware-kit

#------------------------------------------------------------------------------
# Main targets
#------------------------------------------------------------------------------

all: submodules compiler tools lib examples cli
	@echo ""
	@echo "=========================================="
	@echo "OpenSNES SDK build complete!"
	@echo "=========================================="

clean:
	$(MAKE) -C $(COMPILER_PATH) clean
	$(MAKE) -C $(TOOLS_PATH) clean
	$(MAKE) -C $(LIB_PATH) clean
	$(MAKE) -C $(EXAMPLES_PATH) clean
	-rm -rf bin/

install: compiler tools lib cli
	$(MAKE) -C $(COMPILER_PATH) install
	$(MAKE) -C $(TOOLS_PATH) install

# Install the `opensnes` project CLI (init/build/run/doctor) into bin/ so it
# ships in the dev tree and, via the release target's `cp -r bin/*`, in the
# release zip. The CLI resolves the SDK root from its own bin/ location.
cli:
	@mkdir -p bin
	@cp scripts/opensnes bin/opensnes
	@chmod +x bin/opensnes
	@echo "Installed CLI: bin/opensnes  (run 'bin/opensnes doctor')"

#------------------------------------------------------------------------------
# Components
#------------------------------------------------------------------------------

submodules:
	@git submodule update --init --recursive

verify-toolchain:
	@python3 devtools/verify_toolchain.py

# Lint commit messages from origin/develop..HEAD (override RANGE=... for other ranges).
RANGE ?= origin/develop..HEAD
lint-commits:
	@python3 devtools/lint_commits.py $(RANGE)

# Doc-drift sentinel — version macros, ROADMAP status line, examples count
# across active rules. See devtools/check_doc_drift.py and
# .claude/rules/doc_consistency.md. Wired in CI under .github/workflows/lint.yml.
lint-docs:
	@python3 devtools/check_doc_drift.py

# Static analysis of the host C (gaps review H4): the asset tools' own
# sources and the lib's C. cppcheck needs no compile database, so it
# gates on a recursive-make tree as is. Vendored decoders are suppressed
# (lodepng, stb_image: upstream code with its own noise), gfx4snes's
# version macros are supplied. The w65816 backend is checked advisory
# only (upstream QBE idioms). First run found a dangling context pointer
# in cmdparser (both copies), an uninitialised read in aseprite2snes and
# the free-then-fatal paths cppcheck could not see were fatal (noreturn).
# Skips with a note when cppcheck is not installed; CI installs it.
lint-cppcheck:
	@if ! command -v cppcheck >/dev/null 2>&1; then \
		echo "lint-cppcheck: cppcheck not installed, skipped (CI runs it)"; \
	else \
		cppcheck --quiet --enable=warning,performance,portability --error-exitcode=1 --inline-suppr \
			--suppress='*:tools/common/lodepng.c' \
			--suppress='*:tools/font2snes/src/stb_image.h' \
			-DGFX4SNESVERSION='"x"' -DGFX4SNESDATE='"x"' -D__BUILD_DATE='"x"' -D__BUILD_VERSION='"x"' -DVERSION='"x"' \
			-Itools/smconv/src -Itools/common tools/*/src tools/common \
		&& cppcheck --quiet --enable=warning,performance,portability --error-exitcode=1 --inline-suppr \
			-D__OPENSNES__=1 -Ilib/include lib/source/*.c \
		&& { cppcheck --quiet --enable=warning --inline-suppr compiler/qbe/w65816/*.c || true; } \
		&& echo "lint-cppcheck: OK"; \
	fi

# ASM ↔ C signature ABI consistency. Catches the class of bug that bit us
# at chantier A6+A7 hdmaSetupBank: hand-written ASM reading a param at an
# offset that contradicts the C signature's calling-convention layout.
# See devtools/check_asm_abi.py for the matching rules.
lint-asm-abi:
	@python3 devtools/check_asm_abi.py --quiet

# VRAM base-alignment linter. BG/sprite VRAM bases are programmed through
# registers that hold only the high address bits, so a misaligned base is
# silently masked (the value you wrote is not the one the PPU uses). Catches that
# silent-failure class statically. See devtools/check_vram_layout.py.
lint-vram:
	@python3 devtools/check_vram_layout.py

# Aggregate lint target — runs every lint we have. Run before opening a PR.
lint: lint-docs
	@python3 devtools/lint_asm.py
	@python3 devtools/check_lib_rodata.py
	@python3 devtools/check_bank_reads.py --selftest
	@python3 devtools/check_corpus_fresh.py
	@$(MAKE) lint-asm-abi
	@$(MAKE) lint-vram
	@$(MAKE) lint-cppcheck
	@$(MAKE) lint-commits

compiler: submodules verify-toolchain
	$(MAKE) -C $(COMPILER_PATH)
	$(MAKE) -C $(COMPILER_PATH) install

tools: compiler
	$(MAKE) -C $(TOOLS_PATH)
	$(MAKE) -C $(TOOLS_PATH) install

lib: compiler
	$(MAKE) -C $(LIB_PATH)

examples: compiler tools lib
	$(MAKE) -C $(EXAMPLES_PATH)

tests: test-compiler
	@# Corpus freshness guard (issue #105): incremental trees have produced
	@# ROMs that differ from clean builds; baselines captured from them get
	@# rejected by CI. Refuse to test a corpus older than the lib outputs.
	@python3 devtools/check_corpus_fresh.py
	@scripts/install-luna.sh
	@python3 tools/luna-test/luna_runner.py --coverage
	@# Same liveness pass from pseudo-random RAM (fixed seed): a ROM that
	@# reads memory it never initialised passes on luna's zero-fill and
	@# fails here (gaps review item R1; the v0.40.0/v0.41.1 reset-vector
	@# fix class). Report file untouched — the committed one is the
	@# zero-fill pass.
	@python3 tools/luna-test/luna_runner.py --coverage --power-on random=1
	@python3 tools/luna-test/luna_runner.py --compare
	@# The same visual baselines must hold from pseudo-random RAM: since
	@# 2026-09-12 every example does (six audio examples cleared VRAM, the
	@# vertical-scroll -1 landed in the lib), so this is a gate, not a report.
	@python3 tools/luna-test/luna_runner.py --compare --power-on random=1
	@# Measured ROM coverage of the public lib API (luna profile --pc-set):
	@# a public function no example executes must already be in
	@# baselines/never_executed.txt — the ratchet may shrink, never grow
	@# (gaps review item R5). The library fixture is one of the ROMs it
	@# profiles, so it is built first (rebuilt clean for its own asserts below),
	@# and so are the compiler's runtime ROMs.
	@$(MAKE) -s -C devtools/libtests
	@for d in a6_farptr a7_32bit b2_far_ram c_features debug_channel; do \
		$(MAKE) -s -C devtools/compiler-tests/runtime/$$d || exit 1; done
	@python3 tools/luna-test/rom_coverage.py
	@# APU output hashed for four self-playing audio examples (luna
	@# --audio-out, gaps review R6): a changed hash means "the sound
	@# changed, go listen" — the only audio oracle beyond driver liveness.
	@python3 tools/luna-test/audio_regress.py
	@# The NMI handler must fit in VBlank (~51 800 master cycles), measured
	@# by luna on a representative subset (gaps review R4). `make tests`
	@# proved the handler correct but never short enough.
	@python3 tools/luna-test/nmi_budget.py
	@$(MAKE) -s test-manifests
	@# The per-frame WRAM oracle runs here too, not only in CI. It used to
	@# be a separate target, so `make tests` could be green on a codegen
	@# change that CI then rejected on all five platforms — which is
	@# exactly what happened on 2026-07-22. The gate a contributor is told
	@# to run must be the gate CI runs.
	@python3 tools/luna-test/wram_regress.py
	@# Runtime fixture ROMs are rebuilt from clean: a stale .sfc built with an
	@# experimental toolchain once produced misleading XPASSes (a6_farptr trap,
	@# 2026-07-04). Each is a single-TU ROM; the clean rebuild costs seconds.
	@# clean and the build are SEPARATE invocations — this Makefile exports
	@# -j, and `clean all` in one command runs both goals concurrently
	@# (clean deleted crt0.o mid-link on the first parallel run).
	@$(MAKE) -s -C devtools/compiler-tests/runtime/a7_32bit clean
	@$(MAKE) -s -C devtools/compiler-tests/runtime/a7_32bit
	@python3 devtools/compiler-tests/runtime/a7_32bit/test_a7_32bit.py
	@$(MAKE) -s -C devtools/compiler-tests/runtime/c_features clean
	@$(MAKE) -s -C devtools/compiler-tests/runtime/c_features
	@python3 devtools/compiler-tests/runtime/c_features/test_c_features.py
	@$(MAKE) -s -C devtools/compiler-tests/runtime/debug_channel clean
	@$(MAKE) -s -C devtools/compiler-tests/runtime/debug_channel
	@python3 devtools/compiler-tests/runtime/debug_channel/test_debug_channel.py
	@$(MAKE) -s -C devtools/compiler-tests/runtime/a6_farptr clean
	@$(MAKE) -s -C devtools/compiler-tests/runtime/a6_farptr
	@python3 devtools/compiler-tests/runtime/a6_farptr/test_a6_farptr.py
	@$(MAKE) -s -C devtools/compiler-tests/runtime/b2_far_ram clean
	@$(MAKE) -s -C devtools/compiler-tests/runtime/b2_far_ram
	@python3 devtools/compiler-tests/runtime/b2_far_ram/test_b2_far_ram.py
	@$(MAKE) -s -C devtools/libtests clean
	@$(MAKE) -s -C devtools/libtests
	@python3 devtools/libtests/test_libtest.py
	@python3 devtools/link_modules.py
	@# docs/tools/luna.md must be the pinned luna's own --help (review D3)
	@python3 devtools/gen_luna_doc.py --check
	@$(MAKE) -s test-project
	@echo "ALL CHECKS PASSED (luna)"

# The VBlank time budget on its own (gaps review R4): the same gate `make
# tests` runs, handy while tuning the NMI handler. `--report` prints the
# numbers without failing.
test-nmi-budget:
	@scripts/install-luna.sh
	@python3 tools/luna-test/nmi_budget.py

# PAL pass (gaps review R2): the whole corpus booted at 312 lines / 50 Hz
# (luna --force-region pal) plus the lib fixture asserting getRegion() /
# isPAL(). Not in `make tests` (a second corpus pass for one video
# standard); the weekly `pal.yml` workflow runs it, and so should anyone
# touching V-timer, frame-budget or region code.
test-pal:
	@scripts/install-luna.sh
	@python3 tools/luna-test/luna_runner.py --coverage --region pal
	@python3 devtools/libtests/test_libtest.py --region pal

# User-project test story (init → build → test-update → test → FAIL path),
# exactly as a user runs it. Was CI-only until 2026-09-11, when a harness
# rename broke project_test.py and `make tests` stayed green — the gate a
# contributor runs must include everything CI runs. The deliberately wrong
# assert must make `make test` exit non-zero. The sed is done in Python so
# the target behaves the same on macOS (BSD sed) and Linux.
TEST_PROJECT_DIR ?= /tmp/opensnes_test_project
test-project:
	@rm -rf $(TEST_PROJECT_DIR)
	@OPENSNES_HOME=$(CURDIR) scripts/opensnes init $(TEST_PROJECT_DIR) --template game >/dev/null
	@OPENSNES_HOME=$(CURDIR) $(MAKE) -s -C $(TEST_PROJECT_DIR) >/dev/null
	@OPENSNES_HOME=$(CURDIR) $(MAKE) -s -C $(TEST_PROJECT_DIR) test-update >/dev/null
	@OPENSNES_HOME=$(CURDIR) $(MAKE) -s -C $(TEST_PROJECT_DIR) test
	@python3 -c "import pathlib; p = pathlib.Path('$(TEST_PROJECT_DIR)/test/manifest.toml'); p.write_text(p.read_text().replace('player_x = 7800', 'player_x = 9999'))"
	@if OPENSNES_HOME=$(CURDIR) $(MAKE) -s -C $(TEST_PROJECT_DIR) test >/dev/null 2>&1; then \
		echo "ERROR: broken assert did not fail 'make test'"; exit 1; fi
	@echo "user-project test story: OK (incl. the FAIL path)"

# Measured lib API coverage on its own (the `tests` target runs the check).
rom-coverage:
	@$(MAKE) -s -C devtools/libtests
	@for d in a6_farptr a7_32bit b2_far_ram c_features debug_channel; do \
		$(MAKE) -s -C devtools/compiler-tests/runtime/$$d || exit 1; done
	@python3 tools/luna-test/rom_coverage.py

# Clean example build artifacts only — keeps the toolchain binaries in bin/
# (a full `make clean` wipes bin/ and forces a compiler rebuild).
clean-examples:
	$(MAKE) -C $(EXAMPLES_PATH) clean

# Compile-time cc65816 C→ASM pattern checks (no emulator needed).
test-compiler:
	@python3 devtools/compiler-tests/run.py

# Golden-output tests for every asset tool. Byte-compares tool output
# against committed goldens — needs `make tools` first. Also the CI job
# `tools-golden` (lint.yml) runs exactly this target (review P4).
test-tools:
	@python3 tools/gfx4snes/tests/run_golden.py
	@python3 tools/tmx2snes/tests/run_golden.py
	@python3 tools/smconv/tests/run_golden.py
	@python3 tools/wav2brr/tests/run_golden.py
	@python3 tools/palplan/tests/run_golden.py
	@python3 tools/aseprite2snes/tests/run_golden.py
	@python3 tools/font2snes/tests/run_golden.py
	@python3 tools/img2snes/tests/run_golden.py

# Host-side sanitizer pass (gaps review H3, 2026-09-12). Rebuilds cproc-qbe,
# QBE, wla-dx and the asset tools from clean with ASan + UBSan (SANITIZE=1:
# compiler/Makefile and tools/*/Makefile swap -static and -O2 for the
# sanitizer flags), then runs everything that exercises them: the compiler
# fixtures, the lib build, the tool goldens and the whole example corpus.
# halt_on_error turns every report into a non-zero exit, so one finding
# fails the target; leaks are not checked (one-shot processes). The first
# run found seven bugs in five programs (wla-65816 read before its token
# buffer on one-character macro labels, wlalink READ_T signed-shift
# overflow, QBE memset on a NULL table, smconv int stores through u16
# fields and negative shifts in the BRR encoder, wav2brr negative shift,
# tmx2snes offsetof through NULL). The tree is left with SANITIZED binaries
# in bin/ — run `make clean && make` afterwards before anything else.
SAN_ENV := ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
test-sanitizers:
	$(MAKE) -C $(COMPILER_PATH) clean
	$(MAKE) -C $(TOOLS_PATH) clean
	$(MAKE) -C $(LIB_PATH) clean
	$(MAKE) -C $(EXAMPLES_PATH) clean
	$(MAKE) SANITIZE=1 compiler tools
	$(SAN_ENV) python3 devtools/compiler-tests/run.py
	$(SAN_ENV) $(MAKE) SANITIZE=1 lib
	$(SAN_ENV) $(MAKE) SANITIZE=1 test-tools
	$(SAN_ENV) $(MAKE) SANITIZE=1 examples
	$(SAN_ENV) $(MAKE) test-toolchain-suites
	$(MAKE) -s fuzz-replay
	@echo "SANITIZERS: OK — cproc-qbe, qbe, wla-dx and the asset tools ran the fixtures, the lib, the goldens, the corpus and the upstream suites without an ASan/UBSan report"

# Host coverage of the compiler (gaps review H7): QBE and cproc-qbe rebuilt
# with clang source-based coverage (COVERAGE=1), the compiler fixtures and
# the lib build run through them, then llvm-cov reports line coverage —
# the whole tree first, then the files this fork owns (the w65816 backend
# and cproc's target/IR-emission side). A report, not a gate: the numbers
# are the map of what the fixtures never reach. Leaves instrumented
# binaries in bin/ — `make clean && make` afterwards.
LLVM_PROFDATA ?= $(shell command -v llvm-profdata || command -v llvm-profdata-18 || ls /usr/lib64/llvm*/bin/llvm-profdata 2>/dev/null | tail -1)
LLVM_COV      ?= $(shell command -v llvm-cov || command -v llvm-cov-18 || ls /usr/lib64/llvm*/bin/llvm-cov 2>/dev/null | tail -1)
COV_DIR       := /tmp/opensnes_coverage
coverage-host:
	@test -n "$(LLVM_PROFDATA)" -a -n "$(LLVM_COV)" || { echo "coverage-host: llvm-profdata / llvm-cov not found (install llvm)"; exit 1; }
	$(MAKE) -C $(COMPILER_PATH) clean
	$(MAKE) -C $(LIB_PATH) clean
	rm -rf $(COV_DIR) && mkdir -p $(COV_DIR)
	$(MAKE) COVERAGE=1 compiler
	LLVM_PROFILE_FILE=$(COV_DIR)/fixtures-%p.profraw python3 devtools/compiler-tests/run.py
	LLVM_PROFILE_FILE=$(COV_DIR)/lib-%p.profraw $(MAKE) lib
	$(LLVM_PROFDATA) merge -sparse $(COV_DIR)/*.profraw -o $(COV_DIR)/merged.profdata
	$(LLVM_COV) report compiler/qbe/qbe -object compiler/cproc/cproc-qbe -instr-profile=$(COV_DIR)/merged.profdata > $(COV_DIR)/report_all.txt
	$(LLVM_COV) report compiler/qbe/qbe -object compiler/cproc/cproc-qbe -instr-profile=$(COV_DIR)/merged.profdata \
		-ignore-filename-regex='(cproc/(cpp|decl|expr|init|map|pp|scan|siphash|stmt|token|tree|util)\.c|qbe/(abi|alias|amd64|arm64|rv64|cfg|copy|fold|gcm|gvn|live|load|main|mem|parse|rega|simpl|spill|ssa|util)|test/)' > $(COV_DIR)/report_fork.txt
	@echo "== coverage: fork-owned files (w65816 backend, cproc target side)"; cat $(COV_DIR)/report_fork.txt
	@echo "== coverage: whole tree in $(COV_DIR)/report_all.txt"; tail -1 $(COV_DIR)/report_all.txt
	$(LLVM_COV) export compiler/qbe/qbe -object compiler/cproc/cproc-qbe -instr-profile=$(COV_DIR)/merged.profdata -format=lcov > $(COV_DIR)/coverage.lcov
	@echo "COVERAGE: reports in $(COV_DIR) (report_all.txt, report_fork.txt, coverage.lcov) — instrumented binaries left in bin/, run make clean && make"

# luna bench over the whole corpus (gaps review R8): luna's own anomaly scan
# — crashes, freezes, dead APU, missing firmware — with one markdown bug file
# per finding. It wants a flat directory, so the ROMs are collected first.
# Only a "bug" verdict fails the target; "suspect" is luna's word for a
# screen that never changes while the CPU runs, which is what most examples
# do on purpose (first run 2026-09-15: 28 ok, 0 bug, 57 suspect). The
# report is the artifact to read. Nightly in luna-bench.yml; locally when a
# luna release lands.
BENCH_ROMS := /tmp/opensnes_bench_roms
BENCH_OUT  := /tmp/opensnes_bench
luna-bench:
	@scripts/install-luna.sh
	@rm -rf $(BENCH_ROMS) && mkdir -p $(BENCH_ROMS)
	@for m in $$(git ls-files 'examples/**/main.c' 'examples/*/*/main.c'); do d=$$(dirname $$m); \
		for r in $$d/*.sfc; do [ -f "$$r" ] && cp "$$r" "$(BENCH_ROMS)/$$(echo $$d | sed 's|examples/||; s|/|_|g').sfc"; done; done; \
		echo "luna-bench: $$(ls $(BENCH_ROMS) | wc -l) ROMs"
	tools/luna-test/bin/luna bench $(BENCH_ROMS) --out $(BENCH_OUT) -f $${BENCH_FRAMES:-600}
	@ls $(BENCH_OUT); n=$$(ls $(BENCH_OUT)/*.md 2>/dev/null | grep -vc "report.md\|README"); \
		echo "luna-bench: $$n bug file(s) under $(BENCH_OUT)"; [ "$$n" -eq 0 ]

# The upstream test suites of the three toolchain submodules, run on the
# fork's own binaries against known-fail ratchets (gaps review H1,
# devtools/toolchain-suites/*.txt). cproc: 63/170 expected (16-bit int, rodata
# sectioning); QBE: 56/56 on the host target — the one execution test of the
# shared passes the fork patched; wla-dx: 31/32 (base_test_1 is the .BASE
# divergence the fork carries on purpose). Also run at the end of
# test-sanitizers, so CI exercises the suites under ASan + UBSan.
test-toolchain-suites:
	@python3 devtools/toolchain_suites.py

# Every lib module links — alone with its declared dependencies (a missing
# _DEP_ in make/common.mk fails here, not in a user's project) and in two
# all-together groups (a module no example lists still gets built). Gaps
# review L2a; first run found seven undeclared dependencies and a fixed32
# operand WLA sized as direct page. Also runs inside `make tests`.
test-link-modules:
	@python3 devtools/link_modules.py

# Fuzzing the asset parsers (gaps review H5): libFuzzer harnesses under
# tools/fuzz/ for lodepng (gfx4snes, img2snes) and smconv's IT loader,
# built with ASan + UBSan. `fuzz` runs each for FUZZ_SECONDS from the golden
# fixtures (the nightly workflow fuzz.yml gives it 600 s per target);
# `fuzz-replay` runs the committed regression inputs under
# tools/fuzz/crashes/ once — cheap, part of test-sanitizers.
FUZZ_SECONDS ?= 60
fuzz:
	@$(MAKE) -s -C tools/fuzz run FUZZ_SECONDS=$(FUZZ_SECONDS)

fuzz-replay:
	@$(MAKE) -s -C tools/fuzz replay

# Native `luna test` manifests (issue #181) — probes migrated off the Python
# harness onto luna's own manifest runner (the luna-first direction). Builds
# the stress ROMs, then runs the manifests through `luna test` (exit 0/1/2).
test-manifests:
	@$(MAKE) -s -C tools/luna-test/stress/hwmath
	@$(MAKE) -s -C tools/luna-test/stress/ppumul
	@$(MAKE) -s -C tools/luna-test/stress/openbus
	@$(MAKE) -s -C tools/luna-test/stress/bcd
	@$(MAKE) -s -C tools/luna-test/stress/sprite_overflow
	@$(MAKE) -s -C devtools/libtests            # audio_v2.toml fixture
	@tools/luna-test/bin/luna test \
		tools/luna-test/stress/hwmath/hwmath.toml \
		tools/luna-test/stress/ppumul/ppumul.toml \
		tools/luna-test/stress/openbus/openbus.toml \
		tools/luna-test/stress/bcd/bcd.toml \
		tools/luna-test/stress/sprite_overflow/sprite_overflow.toml \
		tools/luna-test/manifests

# WRAM-state regression ("did my change alter invisible runtime state?").
# CI-gated on 54/56 examples — the two whose WRAM stream is arch-dependent
# (mapandobjects, slope_collision) are skipped by default; add --all on a machine
# matching the baseline capture arch. Re-baseline after an intentional change
# with `python3 tools/luna-test/wram_regress.py --update` (same commit).
test-wram:
	@python3 tools/luna-test/wram_regress.py

# PPU resource-budget report (VRAM/CGRAM/OAM footprint per example, via luna).
# The PPU-side twin of symmap's bank $00 / C-RAM checks. Report-only; pairs
# with docs/craft/planning.md. `make budget ARGS="--only mode2"` to focus.
budget:
	@python3 tools/luna-test/budget.py $(ARGS)

# Static asset-budget report (VRAM/CGRAM weight of the converted graphics on
# disk, no ROM run). The build-time twin of `make budget`: that measures the
# runtime footprint via luna, this weighs the assets you built. Report-only —
# an inventory upper bound, not a gate. `make asset-budget ARGS="--only mode1"`.
asset-budget:
	@python3 devtools/asset_budget.py $(ARGS)

# Compiler cycle-count regression guard (static estimate vs committed baseline).
bench:
	@python3 devtools/cyclecount/bench.py

# DOXY_STRICT=1 (set by the docs-strict target) turns Doxygen warnings into
# errors. It is NOT the default: `release` depends on `docs`, and Doxygen
# resolves some directory links differently on Windows, so a doc warning must
# not be able to block a release build on another platform (it did, on the
# first push of the gaps-review P7 gate).
docs:
	cd docs && { cat Doxyfile; $(if $(DOXY_STRICT),echo "WARN_AS_ERROR = FAIL_ON_WARNINGS";) } | doxygen -
	@# The showcase landing page is the site's front door. Doxygen emits the
	@# documentation hub (mainpage.md) as index.html; preserve it as
	@# documentation.html, then install the showcase as the root index.html.
	@# Every other generated page (getting_started.html, tools.html, …) is
	@# untouched, so no doc URL breaks. Kept in the Makefile so a local
	@# `make docs` and the CI deploy build the identical site.
	@cp docs/build/html/index.html docs/build/html/documentation.html
	@cp docs/landing/index.html docs/build/html/index.html
	@echo "========================================="
	@echo "Documentation generated in docs/build/html/"
	@echo "  index.html         -> showcase landing (docs/landing/index.html)"
	@echo "  documentation.html -> Doxygen docs hub (mainpage.md)"
	@echo "========================================="

# The P7 gate: the same build with warnings as errors, run by the doc-render
# job on Linux with the pinned Doxygen and the CSS submodule checked out.
docs-strict: DOXY_STRICT := 1
docs-strict: docs

#------------------------------------------------------------------------------
# Release packaging
#------------------------------------------------------------------------------

release: all docs
	@echo ""
	@echo "=========================================="
	@echo "Creating OpenSNES SDK release package..."
	@echo "=========================================="
	@mkdir -p $(RELEASE_DIR)/opensnes
	@mkdir -p $(RELEASE_DIR)/opensnes/bin
	@mkdir -p $(RELEASE_DIR)/opensnes/lib
	@mkdir -p $(RELEASE_DIR)/opensnes/make
	@mkdir -p $(RELEASE_DIR)/opensnes/templates
	@mkdir -p $(RELEASE_DIR)/opensnes/docs
	@cp -r bin/* $(RELEASE_DIR)/opensnes/bin/ 2>/dev/null || true
	@cp -r lib/include $(RELEASE_DIR)/opensnes/lib/
	@cp -r lib/build $(RELEASE_DIR)/opensnes/lib/
	@cp -r make/* $(RELEASE_DIR)/opensnes/make/
	@cp -r templates/* $(RELEASE_DIR)/opensnes/templates/
	@# Starter project — extract the zip and `make` in opensnes/starter/ works
	@# with zero config (its OPENSNES default resolves to the SDK root at ..).
	@cp -r starter $(RELEASE_DIR)/opensnes/
	@# Project test harness (`make test` in user projects) + the pinned-luna
	@# installer. Only the pieces project_test.py imports — not the SDK's
	@# corpus manifest/baselines.
	@mkdir -p $(RELEASE_DIR)/opensnes/tools/luna-test/probes
	@mkdir -p $(RELEASE_DIR)/opensnes/scripts
	@cp tools/luna-test/project_test.py tools/luna-test/luna_runner.py \
		tools/luna-test/luna.version $(RELEASE_DIR)/opensnes/tools/luna-test/
	@cp tools/luna-test/probes/lib.py $(RELEASE_DIR)/opensnes/tools/luna-test/probes/
	@cp scripts/install-luna.sh $(RELEASE_DIR)/opensnes/scripts/
	@# Post-link checks common.mk runs on every user build (bank $$00
	@# overflow ratchet + NMI/WRAM race lint) — without these the zip's
	@# make/common.mk references scripts that don't exist.
	@mkdir -p $(RELEASE_DIR)/opensnes/devtools/symmap
	@cp devtools/symmap/symmap.py $(RELEASE_DIR)/opensnes/devtools/symmap/
	@cp devtools/check_nmi_wram_race.py $(RELEASE_DIR)/opensnes/devtools/
	@cp -r examples $(RELEASE_DIR)/opensnes/examples/
	@mkdir -p $(RELEASE_DIR)/opensnes/examples/bin
	@find $(RELEASE_DIR)/opensnes/examples -path "*/bin" -prune -o -name "*.sfc" -exec cp {} $(RELEASE_DIR)/opensnes/examples/bin/ \;
	@cp -r docs/build/html $(RELEASE_DIR)/opensnes/docs/ 2>/dev/null || true
	@cp README.md $(RELEASE_DIR)/opensnes/ 2>/dev/null || true
	@cp LICENSE $(RELEASE_DIR)/opensnes/ 2>/dev/null || true
	@cp CHANGELOG.md $(RELEASE_DIR)/opensnes/ 2>/dev/null || true
	@cp ATTRIBUTION.md $(RELEASE_DIR)/opensnes/ 2>/dev/null || true
	@cd $(RELEASE_DIR) && zip -q -r $(RELEASE_NAME).zip opensnes
	@rm -rf $(RELEASE_DIR)/opensnes
	@echo ""
	@echo "=========================================="
	@echo "Release created: $(RELEASE_DIR)/$(RELEASE_NAME).zip"
	@echo "=========================================="

clean-release:
	-rm -rf $(RELEASE_DIR)

#------------------------------------------------------------------------------
# Help
#------------------------------------------------------------------------------

# The ROMs of the hardware verification protocol (docs/HARDWARE_VERIFICATION.md),
# numbered in grid order, for a flash cart's SD card.
hardware-kit:
	@sh scripts/hardware-kit.sh

help:
	@echo "OpenSNES SDK Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build everything (default)"
	@echo "  compiler  - Build cc65816 (cproc+QBE) and WLA-DX"
	@echo "  tools     - Build asset tools (font2snes)"
	@echo "  lib       - Build OpenSNES library"
	@echo "  examples  - Build all example ROMs"
	@echo "  tests     - Build test ROMs"
	@echo "  docs      - Generate API documentation (requires doxygen)"
	@echo "  release   - Create SDK release package (zip)"
	@echo "  hardware-kit - Collect the real-console protocol ROMs (docs/HARDWARE_VERIFICATION.md)"
	@echo "  clean     - Clean all build artifacts"
	@echo "  install   - Install binaries to bin/"
	@echo "  verify-toolchain - Check that compiler submodules match compiler/PINS.md"
	@echo "  lint-commits - Validate commit messages in origin/develop..HEAD (RANGE=... overrides)"
	@echo "  lint-docs - Check anchored doc claims (version macros, ROADMAP status, examples count)"
	@echo "  lint      - Run every lint we have (lint-docs + lint_asm + lint-commits)"
	@echo "  test-sanitizers - Rebuild the host toolchain and tools with ASan+UBSan and run fixtures, lib, goldens, corpus (leaves sanitized binaries: make clean && make after)"
	@echo "  test-toolchain-suites - Run cproc / QBE / wla-dx upstream test suites on the fork binaries (known-fail ratchets in devtools/toolchain-suites/)"
	@echo "  test-link-modules - Link every lib module alone (declared deps only) and in two all-together groups"
	@echo "  fuzz      - Fuzz lodepng and the IT loader with libFuzzer for FUZZ_SECONDS each (ASan+UBSan)"
	@echo "  fuzz-replay - Replay the committed fuzz regression inputs (tools/fuzz/crashes/)"
	@echo "  help      - Show this help"
