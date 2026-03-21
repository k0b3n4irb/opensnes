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
# ROM configuration options:
#   USE_HIROM     - Set to 1 for HiROM mode (64KB banks instead of 32KB)
#   USE_FASTROM   - Set to 1 for FastROM (~33% faster ROM access)
#   USE_SRAM      - Set to 1 to enable battery-backed save (8KB default)
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
TEMPLATES := $(OPENSNES)/templates

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
#   object  -> map      (Object collision uses map tile lookups)
#   map     -> dma      (Map VBlank updates use DMA transfers)
#   snesmod -> console  (SNESMOD requires NMI handling from console)
#   mp5     -> input    (MultiPlayer5 uses input state variables)
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
#   Platformer:     console sprite dma input background map object
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
# ROM Size Configuration
#------------------------------------------------------------------------------
# ROMSIZE = log2(ROM_bytes / 1024). For 8 banks (256KB): 1024 << 8 = 256KB → $08
ROMSIZE ?= $$08

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

# Select header template and memmap include based on ROM mode
ifeq ($(USE_HIROM),1)
HDR_TEMPLATE := $(TEMPLATES)/hdr_hirom.asm
MEMMAP_INC   := memmap_hirom.inc
else
HDR_TEMPLATE := $(TEMPLATES)/hdr.asm
MEMMAP_INC   := memmap.inc
endif

#------------------------------------------------------------------------------
# FastROM Configuration (set USE_FASTROM=1 for faster ROM access)
#------------------------------------------------------------------------------
# FastROM reduces ROM read cycles from 8 to 6 master cycles (~33% faster).
# At boot, the CPU jumps to the bank $80 mirror and sets $420D = $01.
# Compatible with both LoROM and HiROM. All existing code works unchanged.

USE_FASTROM    ?= 0

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
# Module Dependency Auto-Resolution
#------------------------------------------------------------------------------
# Automatically adds transitive dependencies so users don't need to list them
# manually. For example, LIB_MODULES = sprite auto-adds dma.
# Three nested _resolve_one calls handle chains up to 3 deep.
# $(sort) deduplicates — existing explicit deps are harmless.

_DEP_sprite    := dma sprite_oamset
_DEP_text      := dma
_DEP_text4bpp  := dma
_DEP_object    := map
_DEP_map       := dma
_DEP_snesmod   := console
_DEP_mp5       := input

_resolve_one = $(1) $(foreach m,$(1),$(_DEP_$(m)))
_resolve_deps = $(sort $(call _resolve_one,$(call _resolve_one,$(call _resolve_one,$(1)))))

LIB_MODULES := $(call _resolve_deps,$(LIB_MODULES))

#------------------------------------------------------------------------------
# Library Object Resolution (must come after dependency resolution)
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

# Object files from C sources (one .o per .c file)
C_OBJS := $(patsubst %.c,%.c.o,$(CSRC))


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
	@# smconv generates FORCE sections with .ORG 0 and SOUNDBANK_BANK in header.
	@# Do NOT pass -i for HiROM — our SNESMOD reads via LoROM-style mirror
	@# ($XX:$8000-$FFFF, 32KB per bank). Splitting works for both ROM types.
	@$(SMCONV) -s -o $(SOUNDBANK_OUT) -b $(SOUNDBANK_BANK) -n -p $(SOUNDBANK_OUT) $(SOUNDBANK_SRC)
ifeq ($(USE_HIROM),1)
	@# HiROM: override .ORG 0 to .ORG $$8000. SNESMOD reads via bank mirrors
	@# ($$00-$$3F:$$8000-$$FFFF), so data MUST be at $$8000+ within each bank.
	@sed -i 's/^\.ORG 0$$/.ORG $$8000/' $(SOUNDBANK_OUT).asm
endif

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
# C Compilation (multi-file support)
#------------------------------------------------------------------------------

# Wrap an ASM source with memmap include and assemble to object.
# Usage: $(call wrap_asm,source.asm,output.o)
define wrap_asm
	@{ echo '.include "$(MEMMAP_INC)"'; echo ''; cat $(1); } > $(basename $(2)).wrap.asm
	@$(AS) $(ASFLAGS) -I $(TEMPLATES) -o $(2) $(basename $(2)).wrap.asm
endef

# cc65816 (cproc+QBE): compile each C source to a standalone WLA-DX object.
# Pipeline: file.c → file.c.asm (compiler) → file.c.wrap.asm (+ memmap) → file.c.o
#
# Each .c file is compiled independently and linked as a separate object.
# Cross-file references (function calls, extern variables) are resolved by
# the WLA-DX linker — labels are globally visible by default.
#
# CSRC defaults to main.c for backward compatibility. Multi-file projects
# simply list all C sources: CSRC := main.c engine/player.c ui/hud.c
#
# GFX_HEADERS dependency ensures graphics/soundbank conversion completes
# before C compilation starts (soundbank.h must exist for #include).
%.c.o: %.c $(GFX_HEADERS)
	@echo "[CC] $<"
	@$(CC) $(ALL_CFLAGS) $< -o $*.c.asm
	$(call wrap_asm,$*.c.asm,$@)

#------------------------------------------------------------------------------
# Assembly Generation
#------------------------------------------------------------------------------

# Generate project header: sed for ROM_NAME (WLA-DX .SNESHEADER NAME requires
# literal string), .DEFINE for numeric values via project_config.inc.
project_config.inc:
	@echo '.DEFINE CARTRIDGETYPE $(CARTRIDGETYPE)' > $@
	@echo '.DEFINE ROMSIZE_VAL $(ROMSIZE)' >> $@
	@echo '.DEFINE SRAMSIZE_VAL $(SRAMSIZE)' >> $@

project_hdr.asm: $(HDR_TEMPLATE) project_config.inc
	@echo "[HDR] Generating project header ($(if $(filter 1,$(USE_HIROM)),HiROM,LoROM))..."
	@sed 's/__ROM_NAME__/$(ROM_NAME)/' $(HDR_TEMPLATE) > $@

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

# Extract .incbin file paths from ASMSRC and add as dependencies.
# This ensures touching a .pic/.pal/.map file triggers a rebuild
# without needing `make clean`.
ifneq ($(ASMSRC),)
INCBIN_DEPS := $(shell grep -hi '\.incbin' $(ASMSRC) 2>/dev/null | \
    sed -n 's/.*\.incbin[[:space:]]*"\([^"]*\)".*/\1/p' | sort -u)
else
INCBIN_DEPS :=
endif

#------------------------------------------------------------------------------
# Assembly Objects (each source compiled separately — no more combined.asm)
#------------------------------------------------------------------------------

# Assembler flags
ifeq ($(USE_HIROM),1)
ASFLAGS := -D HIROM
else
ASFLAGS :=
endif
ifeq ($(USE_FASTROM),1)
ASFLAGS += -D FASTROM
endif

# crt0: has its own MEMORYMAP via project_hdr.asm — no wrap needed
crt0.o: $(TEMPLATES)/crt0.asm project_hdr.asm project_config.inc
	@echo "[AS] crt0"
	@$(AS) $(ASFLAGS) -I $(TEMPLATES) -o $@ $<

# Runtime math library
runtime.o: $(TEMPLATES)/runtime.asm
	@echo "[AS] runtime"
	$(call wrap_asm,$<,$@)

# Initialized data start marker (APPENDTO base section)
data_init_start.o: $(TEMPLATES)/data_init_start.asm
	@echo "[AS] data_init_start"
	$(call wrap_asm,$<,$@)

# OAM helpers (only when USE_LIB=0)
ifeq ($(USE_LIB),0)
oam_helpers.o: $(OAM_HELPERS)
	@echo "[AS] oam_helpers"
	$(call wrap_asm,$<,$@)
OAM_HELPERS_OBJ := oam_helpers.o
else
OAM_HELPERS_OBJ :=
endif

# User ASM sources — each becomes its own object
ASM_OBJS := $(patsubst %.asm,%.o,$(ASMSRC))

# Generate explicit rules for each ASMSRC file (avoids matching library objects)
define ASM_OBJ_RULE
$(patsubst %.asm,%.o,$(1)): $(1) $(INCBIN_DEPS)
	@echo "[AS] $(1)"
	$$(call wrap_asm,$(1),$$@)
endef
$(foreach src,$(ASMSRC),$(eval $(call ASM_OBJ_RULE,$(src))))

# Soundbank object
ifeq ($(USE_SNESMOD),1)
ifneq ($(SOUNDBANK_SRC),)
SOUNDBANK_OBJ := $(SOUNDBANK_OUT).o
$(SOUNDBANK_OUT).o: $(SOUNDBANK_OUT).asm
	@echo "[AS] $(SOUNDBANK_OUT)"
	$(call wrap_asm,$<,$@)
else
SOUNDBANK_OBJ :=
endif
else
SOUNDBANK_OBJ :=
endif

# End marker for initialized data (must be linked LAST)
data_init_end.o: $(TEMPLATES)/data_init_end.asm
	@echo "[AS] data_init_end"
	$(call wrap_asm,$<,$@)

#------------------------------------------------------------------------------
# Linking
#------------------------------------------------------------------------------

# All objects in link order
LINK_OBJS := crt0.o runtime.o data_init_start.o $(OAM_HELPERS_OBJ) $(ASM_OBJS) $(C_OBJS)
ifeq ($(USE_LIB),1)
LINK_OBJS += $(LIB_OBJS)
endif
LINK_OBJS += $(SOUNDBANK_OBJ) data_init_end.o

# Create linker file
linkfile: $(LINK_OBJS)
	@echo "[objects]" > $@
ifeq ($(OS),Windows_NT)
	@$(foreach obj,$(LINK_OBJS),echo "$$(cygpath -m $(obj))" >> $@;)
else
	@$(foreach obj,$(LINK_OBJS),echo "$(obj)" >> $@;)
endif

# Link to final ROM
$(TARGET): linkfile
	@echo "[LD] $@"
	@$(LD) -S linkfile $@

#------------------------------------------------------------------------------
# Cleanup
#------------------------------------------------------------------------------

clean:
	@echo "Cleaning $(TARGET)..."
	@rm -f crt0.o runtime.o runtime.wrap.asm
	@rm -f data_init_start.o data_init_start.wrap.asm
	@rm -f oam_helpers.o oam_helpers.wrap.asm
	@rm -f $(ASM_OBJS) $(ASM_OBJS:.o=.wrap.asm)
	@rm -f $(CSRC:.c=.c.asm) $(CSRC:.c=.c.wrap.asm) $(CSRC:.c=.c.o)
	@rm -f data_init_end.o data_init_end.wrap.asm
	@rm -f project_hdr.asm project_config.inc linkfile *.sym $(TARGET)
	@rm -f $(GFX_HEADERS)
	@rm -f $(SOUNDBANK_OUT).asm $(SOUNDBANK_OUT).h $(SOUNDBANK_OUT).o $(SOUNDBANK_OUT).wrap.asm $(SOUNDBANK_OUT).bnk
