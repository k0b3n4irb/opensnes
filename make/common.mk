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
# SNESMOD audio options:
#   USE_SNESMOD   - Set to 1 to enable SNESMOD tracker audio
#   SOUNDBANK_SRC - IT files to convert (e.g., music.it sfx.it)
#   SOUNDBANK_OUT - Output name (default: soundbank)
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
SMCONV   := $(OPENSNES)/bin/smconv

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

# Library usage (set USE_LIB=1 to link with OpenSNES library)
USE_LIB     ?= 0

# Library directory
LIBDIR      := $(OPENSNES)/lib/build
LIB_MODULES ?= console

# OAM helpers (when USE_LIB=0, we need standalone OAM functions)
ifeq ($(USE_LIB),0)
OAM_HELPERS := $(TEMPLATES)/oam_helpers.asm
else
OAM_HELPERS :=
endif

#------------------------------------------------------------------------------
# SNESMOD Configuration (set USE_SNESMOD=1 to enable tracker-based audio)
#------------------------------------------------------------------------------

USE_SNESMOD    ?= 0
SOUNDBANK_SRC  ?=
SOUNDBANK_OUT  ?= soundbank

ifeq ($(USE_SNESMOD),1)
# Auto-add snesmod to library modules
LIB_MODULES += snesmod
endif

#------------------------------------------------------------------------------
# Library Object Resolution (must come after SNESMOD module addition)
#------------------------------------------------------------------------------
# Links both C objects (.o) and assembly objects (-asm.o) if they exist.

LIB_OBJS_C  := $(foreach mod,$(LIB_MODULES),$(wildcard $(LIBDIR)/$(mod).o))
LIB_OBJS_ASM := $(foreach mod,$(LIB_MODULES),$(wildcard $(LIBDIR)/$(mod)-asm.o))
LIB_OBJS    := $(LIB_OBJS_C) $(LIB_OBJS_ASM)

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
# SNESMOD Soundbank Conversion
#------------------------------------------------------------------------------

ifeq ($(USE_SNESMOD),1)
ifneq ($(SOUNDBANK_SRC),)

# Soundbank bank number (default: 1)
SOUNDBANK_BANK ?= 1

# Generate soundbank from IT files
# smconv options: -s (soundbank mode), -b (bank), -n (no header), -p (symbol prefix)
$(SOUNDBANK_OUT).asm $(SOUNDBANK_OUT).h: $(SOUNDBANK_SRC)
	@echo "[SMCONV] Generating soundbank from: $(SOUNDBANK_SRC)"
	@$(SMCONV) -s -o $(SOUNDBANK_OUT) -b $(SOUNDBANK_BANK) -n -p $(SOUNDBANK_OUT) $(SOUNDBANK_SRC)
	@# Add SOUNDBANK_BANK constant to header for snesmodSetSoundbank()
	@echo "" >> $(SOUNDBANK_OUT).h
	@echo "#define SOUNDBANK_BANK $(SOUNDBANK_BANK)" >> $(SOUNDBANK_OUT).h

# Add soundbank header to graphics headers (for dependency tracking)
GFX_HEADERS += $(SOUNDBANK_OUT).h

endif
endif

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
# The QBE w65816 backend now emits WLA-DX compatible assembly directly
main.c.asm: $(CSRC) $(GFX_HEADERS)
	@echo "[CC] $(CSRC)"
	@$(CC) $(ALL_CFLAGS) $(CSRC) -o $@

#------------------------------------------------------------------------------
# Assembly Generation
#------------------------------------------------------------------------------

# Generate project-specific header with ROM name
# Use shell variable substitution to avoid sed
project_hdr.asm: $(TEMPLATES)/hdr.asm
	@echo "[HDR] Generating project header..."
	@ROM_NAME='$(ROM_NAME)' && cat $(TEMPLATES)/hdr.asm | while IFS= read -r line; do echo "$${line//__ROM_NAME__/$$ROM_NAME}"; done > $@

# Runtime library (math functions, etc.)
RUNTIME := $(TEMPLATES)/runtime.asm

# Soundbank dependency (when USE_SNESMOD=1)
ifeq ($(USE_SNESMOD),1)
ifneq ($(SOUNDBANK_SRC),)
SOUNDBANK_DEP := $(SOUNDBANK_OUT).asm
else
SOUNDBANK_DEP :=
endif
else
SOUNDBANK_DEP :=
endif

# Combine all assembly sources
# crt0.asm includes project_hdr.asm, then runtime, then compiled C
combined.asm: $(TEMPLATES)/crt0.asm main.c.asm project_hdr.asm $(ASMSRC) $(OAM_HELPERS) $(RUNTIME) $(SOUNDBANK_DEP)
	@echo "[ASM] Combining sources..."
	@cat $(TEMPLATES)/crt0.asm > $@
	@echo "" >> $@
	@echo ";==============================================================================" >> $@
	@echo "; C Runtime Library" >> $@
	@echo ";==============================================================================" >> $@
	@cat $(RUNTIME) >> $@
ifeq ($(USE_LIB),0)
	@echo "" >> $@
	@echo ";==============================================================================" >> $@
	@echo "; OAM Helper Functions (standalone)" >> $@
	@echo ";==============================================================================" >> $@
	@cat $(OAM_HELPERS) >> $@
endif
	@echo "" >> $@
	@echo ";==============================================================================" >> $@
	@echo "; Compiled C Code" >> $@
	@echo ";==============================================================================" >> $@
	@echo "" >> $@
	@cat main.c.asm >> $@
ifneq ($(ASMSRC),)
	@echo "" >> $@
	@echo ";==============================================================================" >> $@
	@echo "; Additional Assembly" >> $@
	@echo ";==============================================================================" >> $@
	@for f in $(ASMSRC); do cat $$f >> $@; echo "" >> $@; done
endif
ifeq ($(USE_SNESMOD),1)
ifneq ($(SOUNDBANK_SRC),)
	@echo "" >> $@
	@echo ";==============================================================================" >> $@
	@echo "; SNESMOD Soundbank" >> $@
	@echo ";==============================================================================" >> $@
	@cat $(SOUNDBANK_OUT).asm >> $@
endif
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
ifeq ($(USE_LIB),1)
	@for obj in $(LIB_OBJS); do echo "$$obj" >> $@; done
endif

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
ifeq ($(USE_SNESMOD),1)
ifneq ($(SOUNDBANK_SRC),)
	@rm -f $(SOUNDBANK_OUT).asm $(SOUNDBANK_OUT).h
endif
endif
