#==============================================================================
# OpenSNES Common Makefile Rules
#==============================================================================
#
# Include this file in your example Makefiles to get standard build rules.
#
# Required variables (set before including):
#   TARGET    - Output ROM filename (e.g., mygame.sfc)
#   ROM_NAME  - 21-character ROM name for header (pad with spaces)
#
# Optional variables:
#   CSRC      - C source files (default: main.c)
#   ASMSRC    - Additional ASM files to include
#   GFXSRC    - PNG files to convert (uses gfx2snes)
#   CFLAGS    - Additional C compiler flags
#   SPRITE_SIZE - Sprite size for gfx2snes (default: 8)
#   BPP       - Bits per pixel for graphics (default: 4)
#
# Usage:
#   OPENSNES := $(shell cd ../../.. && pwd)
#   TARGET   := mygame.sfc
#   ROM_NAME := "MY GAME NAME        "
#   include $(OPENSNES)/make/common.mk
#
#==============================================================================

#------------------------------------------------------------------------------
# Paths
#------------------------------------------------------------------------------

# OPENSNES must be set by the including Makefile (absolute path)
ifndef OPENSNES
$(error OPENSNES is not set. Set it to the absolute path of the opensnes directory)
endif

# Tool binaries
CC       := $(OPENSNES)/bin/cc65816
AS       := $(OPENSNES)/bin/wla-65816
LD       := $(OPENSNES)/bin/wlalink
GFX2SNES := $(OPENSNES)/bin/gfx2snes

# Shared templates
TEMPLATES := $(OPENSNES)/templates/common

#------------------------------------------------------------------------------
# Default Configuration
#------------------------------------------------------------------------------

CSRC        ?= main.c
ASMSRC      ?=
GFXSRC      ?=
SPRITE_SIZE ?= 8
BPP         ?= 4

# Include paths
INCLUDES := -I$(OPENSNES)/lib/include -I.

# All C flags
ALL_CFLAGS := $(INCLUDES) $(CFLAGS)

#------------------------------------------------------------------------------
# Derived Variables
#------------------------------------------------------------------------------

# Generate header filenames from graphics sources (output to current dir)
GFX_HEADERS := $(notdir $(addsuffix .h,$(basename $(GFXSRC))))

# Object files
OBJS := combined.obj

#------------------------------------------------------------------------------
# Build Targets
#------------------------------------------------------------------------------

.PHONY: all clean

all: $(TARGET)
	@echo "==============================================="
	@echo "  Built: $(TARGET)"
	@echo "  Size: $$(wc -c < $(TARGET) | tr -d ' ') bytes"
	@echo "==============================================="

#------------------------------------------------------------------------------
# Graphics Conversion
#------------------------------------------------------------------------------

# Convert each PNG in GFXSRC to a .h file in current directory
# We define explicit rules for each graphics file
define GFX_RULE
$(notdir $(basename $(1)).h): $(1)
	@echo "[GFX] $$< -> $$@"
	@$$(GFX2SNES) -s $$(SPRITE_SIZE) -b $$(BPP) -c $$< $$@
endef

$(foreach src,$(GFXSRC),$(eval $(call GFX_RULE,$(src))))

#------------------------------------------------------------------------------
# C Compilation
#------------------------------------------------------------------------------

# cc65816 (cproc+QBE): compile C to WLA-DX assembly
main.c.asm: $(CSRC) $(GFX_HEADERS)
	@echo "[CC] $(CSRC)"
	@$(CC) $(ALL_CFLAGS) $(CSRC) -o $@

#------------------------------------------------------------------------------
# Assembly Generation
#------------------------------------------------------------------------------

# Generate project-specific header with ROM name
project_hdr.asm: $(TEMPLATES)/hdr.asm
	@echo "[HDR] Generating project header..."
	@sed 's/__ROM_NAME__/$(ROM_NAME)/' $(TEMPLATES)/hdr.asm > $@

# Combine all assembly sources
# crt0.asm includes project_hdr.asm, then we append compiled C
combined.asm: $(TEMPLATES)/crt0.asm main.c.asm project_hdr.asm $(ASMSRC)
	@echo "[ASM] Combining sources..."
	@cat $(TEMPLATES)/crt0.asm > $@
	@echo "" >> $@
	@echo ";==============================================================================" >> $@
	@echo "; Compiled C Code" >> $@
	@echo ";==============================================================================" >> $@
	@echo "" >> $@
	@grep -v '^/\* end' main.c.asm >> $@
ifneq ($(ASMSRC),)
	@echo "" >> $@
	@echo ";==============================================================================" >> $@
	@echo "; Additional Assembly" >> $@
	@echo ";==============================================================================" >> $@
	@for f in $(ASMSRC); do cat $$f >> $@; echo "" >> $@; done
endif

#------------------------------------------------------------------------------
# Linking
#------------------------------------------------------------------------------

# Assemble to object file
combined.obj: combined.asm
	@echo "[AS] $<"
	@$(AS) -o $@ $<

# Create linker file
linkfile: combined.obj
	@echo "[objects]" > $@
	@echo "combined.obj" >> $@

# Link to final ROM
$(TARGET): combined.obj linkfile
	@echo "[LD] $@"
	@$(LD) -S linkfile $@

#------------------------------------------------------------------------------
# Cleanup
#------------------------------------------------------------------------------

clean:
	@echo "Cleaning $(TARGET)..."
	@rm -f *.c.asm combined.asm project_hdr.asm *.obj linkfile *.sym $(TARGET)
ifneq ($(GFX_HEADERS),)
	@rm -f $(GFX_HEADERS)
endif
