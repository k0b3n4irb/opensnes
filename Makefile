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
.PHONY: all clean install compiler tools lib examples tests test-compiler test-wram bench submodules verify-toolchain lint-commits lint-docs lint docs help release clean-release

#------------------------------------------------------------------------------
# Main targets
#------------------------------------------------------------------------------

all: submodules compiler tools lib examples
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

install: compiler tools lib
	$(MAKE) -C $(COMPILER_PATH) install
	$(MAKE) -C $(TOOLS_PATH) install

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

# ASM ↔ C signature ABI consistency. Catches the class of bug that bit us
# at chantier A6+A7 hdmaSetupBank: hand-written ASM reading a param at an
# offset that contradicts the C signature's calling-convention layout.
# See devtools/check_asm_abi.py for the matching rules.
lint-asm-abi:
	@python3 devtools/check_asm_abi.py --quiet

# Aggregate lint target — runs every lint we have. Run before opening a PR.
lint: lint-docs
	@python3 devtools/lint_asm.py
	@$(MAKE) lint-asm-abi
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
	@scripts/install-luna.sh
	@python3 tools/luna-test/luna_runner.py --coverage
	@python3 tools/luna-test/luna_runner.py --compare
	@python3 tools/luna-test/probes/run_all.py
	@echo "ALL CHECKS PASSED (luna)"

# Compile-time cc65816 C→ASM pattern checks (no emulator needed).
test-compiler:
	@python3 devtools/compiler-tests/run.py

# WRAM-state regression — a LOCAL, same-arch developer tool ("did my change alter
# invisible runtime state?"), NOT part of `make tests`/CI: raw WRAM content is not
# a luna cross-arch guarantee (the framebuffer is). Re-baseline on your machine
# with `python3 tools/luna-test/wram_regress.py --update`.
test-wram:
	@python3 tools/luna-test/wram_regress.py

# Compiler cycle-count regression guard (static estimate vs committed baseline).
bench:
	@python3 devtools/cyclecount/bench.py

docs:
	cd docs && doxygen Doxyfile
	@echo "========================================="
	@echo "Documentation generated in docs/build/html/"
	@echo "Open docs/build/html/index.html in a browser"
	@echo "========================================="

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
	@echo "  clean     - Clean all build artifacts"
	@echo "  install   - Install binaries to bin/"
	@echo "  verify-toolchain - Check that compiler submodules match compiler/PINS.md"
	@echo "  lint-commits - Validate commit messages in origin/develop..HEAD (RANGE=... overrides)"
	@echo "  lint-docs - Check anchored doc claims (version macros, ROADMAP status, examples count)"
	@echo "  lint      - Run every lint we have (lint-docs + lint_asm + lint-commits)"
	@echo "  help      - Show this help"
