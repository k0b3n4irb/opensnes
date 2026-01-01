#==============================================================================
# OpenSNES SDK - Master Makefile
#==============================================================================
#
# Usage:
#   make              Build everything (compiler, tools, library)
#   make compiler     Build 816-tcc compiler only
#   make lib          Build library only
#   make tools        Build tools only
#   make clean        Clean all build artifacts
#   make test         Run test suite
#
# Prerequisites:
#   - GCC or Clang
#   - Make
#   - Git (for submodules)
#
#==============================================================================

.PHONY: all compiler lib tools clean test help submodules

# Default target
all: submodules compiler lib
	@echo ""
	@echo "=========================================="
	@echo "OpenSNES SDK build complete!"
	@echo "=========================================="
	@echo ""
	@echo "Next steps:"
	@echo "  export OPENSNES_HOME=$(shell pwd)"
	@echo "  cd templates/platformer && make"
	@echo ""

#------------------------------------------------------------------------------
# Submodules
#------------------------------------------------------------------------------

submodules:
	@echo "[SUBMODULES] Initializing..."
	@git submodule update --init --recursive

#------------------------------------------------------------------------------
# Compiler (816-tcc)
#------------------------------------------------------------------------------

compiler: submodules
	@echo "[COMPILER] Building 816-tcc..."
	$(MAKE) -C compiler/tcc
	@mkdir -p bin
	@cp compiler/tcc/816-tcc bin/
	@echo "[COMPILER] Done. Binary: bin/816-tcc"

#------------------------------------------------------------------------------
# Library
#------------------------------------------------------------------------------

lib:
	@echo "[LIBRARY] Building OpenSNES library..."
	$(MAKE) -C lib
	@echo "[LIBRARY] Done."

#------------------------------------------------------------------------------
# Tools
#------------------------------------------------------------------------------

tools:
	@echo "[TOOLS] Building tools..."
	$(MAKE) -C tools
	@echo "[TOOLS] Done."

#------------------------------------------------------------------------------
# Testing
#------------------------------------------------------------------------------

test:
	@echo "[TEST] Running test suite..."
	@cd tests && ./run_tests.sh

#------------------------------------------------------------------------------
# Clean
#------------------------------------------------------------------------------

clean:
	@echo "[CLEAN] Cleaning build artifacts..."
	-$(MAKE) -C compiler/tcc clean 2>/dev/null || true
	-$(MAKE) -C lib clean 2>/dev/null || true
	-$(MAKE) -C tools clean 2>/dev/null || true
	-rm -rf bin/
	@echo "[CLEAN] Done."

#------------------------------------------------------------------------------
# Help
#------------------------------------------------------------------------------

help:
	@echo "OpenSNES SDK Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build everything (default)"
	@echo "  compiler  - Build 816-tcc compiler"
	@echo "  lib       - Build OpenSNES library"
	@echo "  tools     - Build asset conversion tools"
	@echo "  test      - Run test suite"
	@echo "  clean     - Remove build artifacts"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Environment:"
	@echo "  OPENSNES_HOME - Set to SDK root directory"
