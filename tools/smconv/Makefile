#==============================================================================
# smconv - SNESMOD Soundbank Converter
#==============================================================================
#
# This tool converts Impulse Tracker (.it) modules into SNESMOD soundbanks.
# It generates both assembly data and a C header with module/effect IDs.
#
# Requirements:
#   - Go 1.20 or later (https://go.dev/dl/)
#
# Usage:
#   smconv -s -o soundbank music.it effects.it
#
# Output:
#   - soundbank.asm - Assembly data for WLA-DX
#   - soundbank.h   - Module/effect ID constants
#
#==============================================================================

BINDIR := ../../bin
TARGET := $(BINDIR)/smconv

# SNESMOD source repository
SNESMOD_REPO := https://github.com/mukunda-/snesmod.git
SNESMOD_DIR  := snesmod

.PHONY: all clean install check-go

all: $(TARGET)

# Check for Go installation
check-go:
	@which go > /dev/null 2>&1 || { \
		echo ""; \
		echo "ERROR: Go is required to build smconv."; \
		echo ""; \
		echo "Install Go from: https://go.dev/dl/"; \
		echo ""; \
		echo "On macOS with Homebrew: brew install go"; \
		echo "On Ubuntu/Debian: sudo apt install golang"; \
		echo ""; \
		exit 1; \
	}

# Clone and build smconv from source
$(TARGET): check-go | $(BINDIR)
	@echo "[SMCONV] Cloning SNESMOD repository..."
	@if [ ! -d "$(SNESMOD_DIR)" ]; then \
		git clone --depth 1 $(SNESMOD_REPO) $(SNESMOD_DIR); \
	fi
	@echo "[SMCONV] Building smconv..."
	@cd $(SNESMOD_DIR)/smconv && go build -o ../../../$@ .
	@echo "[SMCONV] Built: $@"

$(BINDIR):
	@mkdir -p $@

install: $(TARGET)

clean:
	@echo "[CLEAN] Removing smconv build files..."
	@rm -rf $(SNESMOD_DIR)
	@rm -f $(TARGET)

#==============================================================================
# Manual download (if Go is not available)
#==============================================================================
#
# If you cannot install Go, you can use a pre-built smconv binary.
#
# For PVSnesLib users, smconv is included at:
#   /path/to/pvsneslib/devkitsnes/tools/smconv
#
# Copy it to: opensnes/bin/smconv
#
#==============================================================================
