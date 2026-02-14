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
#   GFXSRC    - PNG files to convert (uses gfx4snes, outputs .pic/.pal)
#   CFLAGS    - Additional C compiler flags
#   SPRITE_SIZE - Sprite/tile size for gfx4snes (default: 8)
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
$(error OPENSNES is not set. Add this to your Makefile: OPENSNES := /path/to/opensnes)
endif

# Check that OPENSNES directory exists
ifeq ($(wildcard $(OPENSNES)/make/common.mk),)
$(error OPENSNES path is invalid: $(OPENSNES) - directory does not exist or is not an OpenSNES installation)
endif

# Tool binaries
CC       := $(OPENSNES)/bin/cc65816
AS       := $(OPENSNES)/bin/wla-65816
LD       := $(OPENSNES)/bin/wlalink
GFX4SNES := $(OPENSNES)/bin/gfx4snes
SMCONV   := $(OPENSNES)/bin/smconv

# Check that compiler toolchain exists (skip for 'clean' target)
ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(CC)),)
$(info )
$(info ========================================================================)
$(info  ERROR: OpenSNES compiler not found at $(CC))
$(info )
$(info  The compiler toolchain has not been built. Run:)
$(info    cd $(OPENSNES) && make compiler)
$(info )
$(info  If that fails, ensure submodules are initialized:)
$(info    cd $(OPENSNES) && git submodule update --init --recursive)
$(info ========================================================================)
$(info )
$(error Compiler not built - see instructions above)
endif
endif

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

#------------------------------------------------------------------------------
# Library Module Dependencies
#------------------------------------------------------------------------------
# When using specific library modules, some depend on others:
#
#   sprite  -> dma      (OAM buffer is DMA'd to hardware in NMI handler)
#   text    -> dma      (Font loading uses DMA transfers)
#   snesmod -> console  (SNESMOD requires NMI handling from console)
#   hdma    -> (none)   (Standalone, but often used with console for VBlank)
#
# The build system does NOT auto-resolve dependencies. If you use 'sprite',
# you MUST also include 'dma' in LIB_MODULES:
#
#   LIB_MODULES = console sprite dma    # Correct
#   LIB_MODULES = console sprite        # Wrong - missing dma!
#
# Common module combinations:
#   Basic game:     console sprite dma input background
#   With audio:     console sprite dma input snesmod
#   With effects:   console sprite dma hdma
#------------------------------------------------------------------------------

# Library directory (select lorom or hirom based on USE_HIROM)
ifeq ($(USE_HIROM),1)
LIBDIR      := $(OPENSNES)/lib/build/hirom
LIBMODE     := HiROM
else
LIBDIR      := $(OPENSNES)/lib/build/lorom
LIBMODE     := LoROM
endif
LIB_MODULES ?= console

# Check library is built when USE_LIB=1 (skip for 'clean' target)
ifneq ($(MAKECMDGOALS),clean)
ifeq ($(USE_LIB),1)
ifeq ($(wildcard $(LIBDIR)/console.o),)
$(info )
$(info ========================================================================)
$(info  ERROR: OpenSNES library ($(LIBMODE)) not built)
$(info )
$(info  Your project uses USE_LIB=1 but the $(LIBMODE) library has not been compiled.)
$(info  Run:)
$(info    cd $(OPENSNES) && make lib)
$(info )
$(info  Or build everything:)
$(info    cd $(OPENSNES) && make)
$(info ========================================================================)
$(info )
$(error Library not built - see instructions above)
endif
endif
endif

# OAM helpers (when USE_LIB=0, we need standalone OAM functions)
ifeq ($(USE_LIB),0)
OAM_HELPERS := $(TEMPLATES)/oam_helpers.asm
else
OAM_HELPERS :=
endif

#------------------------------------------------------------------------------
# SRAM Configuration (set USE_SRAM=1 to enable battery-backed save)
#------------------------------------------------------------------------------
# USE_SRAM    - Set to 1 to enable SRAM for save games
# SRAM_SIZE   - SRAM size code: 0=None, 1=2KB, 2=4KB, 3=8KB, 4=16KB, 5=32KB

USE_SRAM    ?= 0
SRAM_SIZE   ?= 3

# When SRAM is enabled, add the sram module to library modules
ifeq ($(USE_SRAM),1)
LIB_MODULES += sram
CARTRIDGETYPE := $$02
SRAMSIZE := $$0$(SRAM_SIZE)
else
CARTRIDGETYPE := $$00
SRAMSIZE := $$00
endif

#------------------------------------------------------------------------------
# HiROM Configuration (set USE_HIROM=1 for 64KB bank ROM layout)
#------------------------------------------------------------------------------
# HiROM uses 64KB banks instead of LoROM's 32KB banks.
# This allows larger contiguous ROM access without frequent bank switching.
# Note: USE_SRAM + USE_HIROM is not yet fully supported (SRAM location differs).

USE_HIROM      ?= 0

# HiROM note: cartridge type ($FFD6) does NOT encode LoROM/HiROM.
# That's in the map mode byte ($FFD5), set by HIROM/LOROM directives.
# $FFD6 values: $00=ROM, $01=ROM+RAM, $02=ROM+RAM+Battery.
# The previous $21/$23 values incorrectly claimed coprocessor presence,
# causing emulators to misdetect the ROM type.

# Select header template based on ROM mode
ifeq ($(USE_HIROM),1)
HDR_TEMPLATE := $(TEMPLATES)/hdr_hirom.asm
else
HDR_TEMPLATE := $(TEMPLATES)/hdr.asm
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

# SNESMOD HiROM note:
# Do NOT pass -i to smconv. The -i flag generates $0000-based internal addresses,
# but the SNESMOD driver code (snesmod.asm) uses hardcoded $8000 offsets
# (SB_SAMPCOUNT=$8000, SB_MODTABLE=$8004, etc.). Keep LoROM-style $8000 addresses.
#
# HiROM WLA-DX to CPU address mapping (bank $00-$3F mirror):
#   WLA-DX bank N, .ORG $XXXX → file offset = N * $10000 + $XXXX
#   CPU $NN:$8000 (mirror) → file offset = N * $10000 + $8000
#   Therefore: .ORG $8000 in WLA-DX bank 1 → file $18000 = CPU $01:$8000 ✓
SMCONV_HIROM_FLAG :=

# Generate soundbank from IT files
# smconv options: -s (soundbank mode), -b (bank), -n (no header), -p (symbol prefix)
$(SOUNDBANK_OUT).asm $(SOUNDBANK_OUT).h: $(SOUNDBANK_SRC)
	@echo "[SMCONV] Generating soundbank from: $(SOUNDBANK_SRC)"
	@$(SMCONV) -s -o $(SOUNDBANK_OUT) -b $(SOUNDBANK_BANK) -n -p $(SOUNDBANK_OUT) $(SOUNDBANK_SRC)
ifeq ($(USE_HIROM),1)
	@# HiROM: force soundbank to .ORG $$8000 within its WLA-DX bank.
	@# In HiROM, CPU $$01:$$8000 → file offset $$18000 = WLA-DX bank 1, .ORG $$8000.
	@# SNESMOD reads via bank $$01 ($00-$3F mirror), so data MUST be at $$8000+ offset.
	@sed -i -e 's/^\.BANK \([0-9]*\)$$/.BANK \1\n.ORG $$8000/' \
	        -e 's/\.SECTION "SOUNDBANK"/.SECTION "SOUNDBANK" FORCE/' $(SOUNDBANK_OUT).asm
endif
	@# Add SOUNDBANK_BANK constant to header for snesmodSetSoundbank()
	@mv $(SOUNDBANK_OUT).h $(SOUNDBANK_OUT).h.tmp
	@{ cat $(SOUNDBANK_OUT).h.tmp; echo ""; echo "#define SOUNDBANK_BANK $(SOUNDBANK_BANK)"; } > $(SOUNDBANK_OUT).h
	@rm -f $(SOUNDBANK_OUT).h.tmp

# Add soundbank header to graphics headers (for dependency tracking)
GFX_HEADERS += $(SOUNDBANK_OUT).h

endif
endif

#------------------------------------------------------------------------------
# Graphics Conversion
#------------------------------------------------------------------------------

# Convert each PNG in GFXSRC to .pic and .pal files using gfx4snes
# Output: basename.pic (tiles) and basename.pal (palette)
define GFX_RULE
$(notdir $(basename $(1)).pic) $(notdir $(basename $(1)).pal): $(1)
	@echo "[GFX] $$< -> $$(notdir $$(basename $$<)).pic/.pal"
	@$$(GFX4SNES) -s $$(SPRITE_SIZE) -p -i $$<
endef

$(foreach src,$(GFXSRC),$(eval $(call GFX_RULE,$(src))))

#------------------------------------------------------------------------------
# C Compilation
#------------------------------------------------------------------------------

# cc65816 (cproc+QBE): compile C to WLA-DX assembly
# The QBE w65816 backend now emits WLA-DX compatible assembly directly
#
# LIMITATION: Only a single C source file (main.c) is supported by this pipeline.
# The build system compiles all files listed in CSRC together into a single
# main.c.asm output. While CSRC can technically contain multiple files, only
# main.c is used as the primary entry point.
#
# Workarounds for multi-file projects:
#   1. Use additional assembly files via ASMSRC variable
#   2. #include other C files directly into main.c
#   3. Implement additional logic in assembly and link via ASMSRC
#
main.c.asm: $(CSRC) $(GFX_HEADERS)
	@echo "[CC] $(CSRC)"
	@$(CC) $(ALL_CFLAGS) $(CSRC) -o $@

#------------------------------------------------------------------------------
# Assembly Generation
#------------------------------------------------------------------------------

# Generate project-specific header with ROM name and SRAM settings
# Uses sed for reliable multi-pattern substitution
# HDR_TEMPLATE is set based on USE_HIROM (hdr.asm or hdr_hirom.asm)
project_hdr.asm: $(HDR_TEMPLATE)
	@echo "[HDR] Generating project header ($(if $(filter 1,$(USE_HIROM)),HiROM,LoROM))..."
	@sed -e 's/__ROM_NAME__/$(ROM_NAME)/g' \
	     -e 's/__CARTRIDGETYPE__/$(CARTRIDGETYPE)/g' \
	     -e 's/__SRAMSIZE__/$(SRAMSIZE)/g' \
	     $(HDR_TEMPLATE) > $@

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
# Order: crt0 -> runtime -> data_init_start -> compiled C -> data_init_end
combined.asm: $(TEMPLATES)/crt0.asm main.c.asm project_hdr.asm $(ASMSRC) $(OAM_HELPERS) $(RUNTIME) $(SOUNDBANK_DEP)
	@echo "[ASM] Combining sources..."
	@cat $(TEMPLATES)/crt0.asm > $@
	@cat $(RUNTIME) >> $@
ifeq ($(USE_LIB),0)
	@cat $(OAM_HELPERS) >> $@
endif
	@cat $(TEMPLATES)/data_init_start.asm >> $@
	@cat main.c.asm >> $@
ifneq ($(ASMSRC),)
	@for f in $(ASMSRC); do cat $$f >> $@; done
endif
ifeq ($(USE_SNESMOD),1)
ifneq ($(SOUNDBANK_SRC),)
	@cat $(SOUNDBANK_OUT).asm >> $@
endif
endif
	@cat $(TEMPLATES)/data_init_end.asm >> $@

#------------------------------------------------------------------------------
# Linking
#------------------------------------------------------------------------------

# Assemble to object file
# Pass -D HIROM for HiROM builds so crt0.asm can use ORGA $8000
ifeq ($(USE_HIROM),1)
ASFLAGS := -D HIROM
else
ASFLAGS :=
endif

combined.obj: combined.asm
	@echo "[AS] $<"
	@$(AS) $(ASFLAGS) -o $@ $<

# Create linker file
# Note: On MSYS2/Windows, wlalink needs Windows-style paths, so we use cygpath -m
linkfile: combined.obj
	@echo "[objects]" > $@
	@echo "combined.obj" >> $@
ifeq ($(USE_LIB),1)
ifeq ($(OS),Windows_NT)
	@for obj in $(LIB_OBJS); do echo "$$(cygpath -m $$obj)" >> $@; done
else
	@for obj in $(LIB_OBJS); do echo "$$obj" >> $@; done
endif
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
