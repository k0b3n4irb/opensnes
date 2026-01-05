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
	@cp $(QBE_DIR)/qbe $(BIN_DIR)/qbe
	@cp $(CPROC_DIR)/cproc-qbe $(BIN_DIR)/cproc-qbe
	@cp scripts/cc65816 $(BIN_DIR)/cc65816
	@chmod +x $(BIN_DIR)/cc65816

$(QBE_DIR)/qbe:
	$(MAKE) -C $(QBE_DIR)

$(CPROC_DIR)/cproc-qbe:
	$(MAKE) -C $(CPROC_DIR)

cc65816-clean:
	$(MAKE) -C $(QBE_DIR) clean
	$(MAKE) -C $(CPROC_DIR) clean
	-rm -f $(BIN_DIR)/qbe $(BIN_DIR)/cproc-qbe

cc65816-install: cc65816
	@echo "[INSTALL] cc65816 installed to $(BIN_DIR)/"

#------------------------------------------------------------------------------
# WLA-DX (assembler/linker)
#------------------------------------------------------------------------------

wla:
	cd $(WLA_DIR) && cmake -G "$(CMAKE_TYPE)" . && $(MAKE) wla-65816 wla-spc700 wlalink

wla-clean:
	-cd $(WLA_DIR) && $(MAKE) clean 2>/dev/null || true
	-rm -f $(WLA_DIR)/CMakeCache.txt

wla-install:
	@mkdir -p $(BIN_DIR)
	@cp $(WLA_DIR)/binaries/wla-65816 $(WLA_DIR)/binaries/wla-spc700 $(WLA_DIR)/binaries/wlalink $(BIN_DIR)/
