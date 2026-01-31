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

# Parallel builds
UNAME := $(shell uname -s)
ifeq ($(OS),Windows_NT)
    MAKEFLAGS += -j$(NUMBER_OF_PROCESSORS)
else ifeq ($(UNAME),Darwin)
    MAKEFLAGS += -j$(shell sysctl -n hw.ncpu)
else ifeq ($(UNAME),Linux)
    MAKEFLAGS += -j$(shell nproc)
endif

# Paths
COMPILER_PATH := compiler
TOOLS_PATH    := tools
LIB_PATH      := lib
EXAMPLES_PATH := examples
TESTS_PATH    := tests

.DEFAULT_GOAL := all
.PHONY: all clean install compiler tools lib examples tests submodules docs help

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
	$(MAKE) -C $(TESTS_PATH) clean
	-rm -rf bin/

install: compiler tools
	$(MAKE) -C $(COMPILER_PATH) install
	$(MAKE) -C $(TOOLS_PATH) install

#------------------------------------------------------------------------------
# Components
#------------------------------------------------------------------------------

submodules:
	@git submodule update --init --recursive

compiler: submodules
	$(MAKE) -C $(COMPILER_PATH)
	$(MAKE) -C $(COMPILER_PATH) install

tools: compiler
	$(MAKE) -C $(TOOLS_PATH)
	$(MAKE) -C $(TOOLS_PATH) install

lib: compiler
	$(MAKE) -C $(LIB_PATH)

examples: compiler tools lib
	$(MAKE) -C $(EXAMPLES_PATH)

tests: compiler tools
	$(MAKE) -C $(TESTS_PATH)

docs:
	doxygen Doxyfile
	@echo "========================================="
	@echo "Documentation generated in doc/html/"
	@echo "Open doc/html/index.html in a browser"
	@echo "========================================="

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
	@echo "  clean     - Clean all build artifacts"
	@echo "  install   - Install binaries to bin/"
	@echo "  help      - Show this help"
