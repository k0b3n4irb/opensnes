#==============================================================================
# OpenSNES Compiler - Makefile
#==============================================================================

# Directories
QBE_DIR := qbe
CPROC_DIR := cproc
WLA_DIR := wla-dx
BIN_DIR := ../bin

# CMake generator
ifeq ($(OS),Windows_NT)
    CMAKE_TYPE = MSYS Makefiles
else
    CMAKE_TYPE = Unix Makefiles
endif

# Parallel builds
UNAME := $(shell uname -s)
ifeq ($(OS),Windows_NT)
    MAKEFLAGS := -j$(NUMBER_OF_PROCESSORS)
else ifeq ($(UNAME), Darwin)
    MAKEFLAGS := -j$(shell sysctl -n hw.ncpu)
else ifeq ($(UNAME), Linux)
    MAKEFLAGS := -j$(shell nproc)
endif

.DEFAULT_GOAL := all
.PHONY: all clean install cc65816 cc65816-clean cc65816-install wla wla-clean wla-install

#------------------------------------------------------------------------------
# Main targets
#------------------------------------------------------------------------------

all: cc65816 wla

clean: cc65816-clean wla-clean

install: cc65816-install wla-install

#------------------------------------------------------------------------------
# cc65816 (cproc + QBE) - C compiler for WDC 65816
#------------------------------------------------------------------------------

cc65816: $(QBE_DIR)/qbe $(CPROC_DIR)/cproc-qbe
	@echo "[CC65816] Installing compiler..."
	@mkdir -p $(BIN_DIR)
ifeq ($(OS),Windows_NT)
	@cp $(QBE_DIR)/qbe.exe $(BIN_DIR)/qbe.exe
	@cp $(CPROC_DIR)/cproc-qbe.exe $(BIN_DIR)/cproc-qbe.exe
else
	@cp $(QBE_DIR)/qbe $(BIN_DIR)/qbe
	@cp $(CPROC_DIR)/cproc-qbe $(BIN_DIR)/cproc-qbe
endif
	@cp scripts/cc65816 $(BIN_DIR)/cc65816
	@chmod +x $(BIN_DIR)/cc65816

$(QBE_DIR)/qbe:
	$(MAKE) -C $(QBE_DIR) CC=clang

# Generate cproc config.h from template (required for non-Linux platforms)
# Using a template file is more maintainable than multi-line echo commands
$(CPROC_DIR)/config.h: config.h.in
	@echo "[CPROC] Generating config.h for w65816 target..."
	@cp $< $@

$(CPROC_DIR)/cproc-qbe: $(CPROC_DIR)/config.h
# Only build cproc-qbe (not cproc driver which uses POSIX spawn.h)
# Use clang for consistent builds across all platforms
	$(MAKE) -C $(CPROC_DIR) CC="clang -std=c99" cproc-qbe

cc65816-clean:
	$(MAKE) -C $(QBE_DIR) clean
	$(MAKE) -C $(CPROC_DIR) clean
	-rm -f $(BIN_DIR)/qbe $(BIN_DIR)/cproc-qbe $(BIN_DIR)/cc65816
	-rm -f $(CPROC_DIR)/config.h

cc65816-install: cc65816
	@echo "[INSTALL] cc65816 installed to $(BIN_DIR)/"

#------------------------------------------------------------------------------
# WLA-DX (assembler/linker)
#------------------------------------------------------------------------------

# Marker file to track if WLA-DX has been built
WLA_BUILT_MARKER := $(WLA_DIR)/.wla-built

wla: $(WLA_BUILT_MARKER)

# Only run CMake and build if binaries don't exist
# The marker file prevents unnecessary CMake reconfiguration on incremental builds
$(WLA_BUILT_MARKER):
	cd $(WLA_DIR) && cmake -G "$(CMAKE_TYPE)" -DCMAKE_C_COMPILER=clang . && $(MAKE) wla-65816 wla-spc700 wlalink
	@touch $@

wla-clean:
	-cd $(WLA_DIR) && $(MAKE) clean 2>/dev/null || true
	-rm -f $(WLA_DIR)/CMakeCache.txt
	-rm -f $(WLA_BUILT_MARKER)

wla-install:
	@mkdir -p $(BIN_DIR)
ifeq ($(OS),Windows_NT)
	@cp $(WLA_DIR)/binaries/wla-65816.exe $(WLA_DIR)/binaries/wla-spc700.exe $(WLA_DIR)/binaries/wlalink.exe $(BIN_DIR)/
else
	@cp $(WLA_DIR)/binaries/wla-65816 $(WLA_DIR)/binaries/wla-spc700 $(WLA_DIR)/binaries/wlalink $(BIN_DIR)/
endif
