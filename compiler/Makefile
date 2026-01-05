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
	$(MAKE) -C $(QBE_DIR)

# Generate cproc config.h if missing (required for non-Linux platforms)
$(CPROC_DIR)/config.h:
	@echo "[CPROC] Generating config.h for w65816 target..."
	@echo '/* cproc configuration for w65816 (SNES) target */' > $@
	@echo '' >> $@
	@echo 'static const char target[] = "w65816";' >> $@
	@echo '' >> $@
	@echo 'static const char *const defaultdefines[] = {' >> $@
	@echo '    "-D", "__w65816__",' >> $@
	@echo '    "-D", "__SNES__",' >> $@
	@echo '    "-D", "__INT_MAX__=32767",' >> $@
	@echo '    "-D", "__LONG_MAX__=2147483647",' >> $@
	@echo '    "-D", "__SIZEOF_INT__=2",' >> $@
	@echo '    "-D", "__SIZEOF_LONG__=4",' >> $@
	@echo '    "-D", "__SIZEOF_POINTER__=2",' >> $@
	@echo '    NULL,' >> $@
	@echo '};' >> $@
	@echo '' >> $@
	@echo 'static const char *const defaultlinkflags[] = {NULL};' >> $@
	@echo '' >> $@
	@echo 'static const char *const preprocesscmd[] = {' >> $@
	@echo '    "cpp",' >> $@
	@echo '    "-U", "__GNUC__",' >> $@
	@echo '    "-P",' >> $@
	@echo '};' >> $@
	@echo '' >> $@
	@echo 'static const char *const codegencmd[] = {' >> $@
	@echo '    "../qbe/qbe",' >> $@
	@echo '};' >> $@
	@echo '' >> $@
	@echo 'static const char *const assemblecmd[] = {' >> $@
	@echo '    "wla-65816",' >> $@
	@echo '    "-o",' >> $@
	@echo '};' >> $@
	@echo '' >> $@
	@echo 'static const char *const linkcmd[] = {' >> $@
	@echo '    "wlalink",' >> $@
	@echo '};' >> $@
	@echo '' >> $@
	@echo 'static const char *const startfiles[] = {NULL};' >> $@
	@echo 'static const char *const endfiles[] = {NULL};' >> $@

$(CPROC_DIR)/cproc-qbe: $(CPROC_DIR)/config.h
# Only build cproc-qbe (not cproc driver which uses POSIX spawn.h)
# cproc Makefile uses 'c99' which doesn't exist on MSYS2/Windows
# Use 'cc -std=c99' instead (works on all platforms)
	$(MAKE) -C $(CPROC_DIR) CC="cc -std=c99" cproc-qbe

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

wla:
	cd $(WLA_DIR) && cmake -G "$(CMAKE_TYPE)" . && $(MAKE) wla-65816 wla-spc700 wlalink

wla-clean:
	-cd $(WLA_DIR) && $(MAKE) clean 2>/dev/null || true
	-rm -f $(WLA_DIR)/CMakeCache.txt

wla-install:
	@mkdir -p $(BIN_DIR)
ifeq ($(OS),Windows_NT)
	@cp $(WLA_DIR)/binaries/wla-65816.exe $(WLA_DIR)/binaries/wla-spc700.exe $(WLA_DIR)/binaries/wlalink.exe $(BIN_DIR)/
else
	@cp $(WLA_DIR)/binaries/wla-65816 $(WLA_DIR)/binaries/wla-spc700 $(WLA_DIR)/binaries/wlalink $(BIN_DIR)/
endif
