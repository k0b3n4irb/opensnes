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
# Paths and Tools
#------------------------------------------------------------------------------

ifndef OPENSNES
$(error OPENSNES is not set. Add this to your Makefile: OPENSNES := /path/to/opensnes)
endif
ifeq ($(wildcard $(OPENSNES)/make/common.mk),)
$(error OPENSNES path is invalid: $(OPENSNES))
endif

CC       := $(OPENSNES)/bin/cc65816
AS       := $(OPENSNES)/bin/wla-65816
LD       := $(OPENSNES)/bin/wlalink
GFX4SNES := $(OPENSNES)/bin/gfx4snes
SMCONV   := $(OPENSNES)/bin/smconv
TEMPLATES := $(OPENSNES)/templates

# Check toolchain exists (skip for 'clean' target)
ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(CC)),)
$(error Compiler not built. Run: cd $(OPENSNES) && make compiler)
endif
endif

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------

CSRC        ?= main.c
ASMSRC      ?=
GFXSRC      ?=
SPRITE_SIZE ?= 8
BPP         ?= 4
USE_LIB     ?= 0
USE_HIROM   ?= 0
USE_FASTROM ?= 0
USE_SRAM    ?= 0
USE_SNESMOD ?= 0
SRAM_SIZE   ?= 3
SOUNDBANK_SRC ?=
SOUNDBANK_OUT ?= soundbank
SOUNDBANK_BANK ?= 1
ROMSIZE     ?= $$08

# Derived configuration (one-liners using $(if))
LIBDIR       := $(OPENSNES)/lib/build/$(if $(filter 1,$(USE_HIROM)),hirom,lorom)
HDR_TEMPLATE := $(TEMPLATES)/$(if $(filter 1,$(USE_HIROM)),hdr_hirom.asm,hdr.asm)
MEMMAP_INC   := $(if $(filter 1,$(USE_HIROM)),memmap_hirom.inc,memmap.inc)
CARTRIDGETYPE := $(if $(filter 1,$(USE_SRAM)),$$02,$$00)
SRAMSIZE     := $(if $(filter 1,$(USE_SRAM)),$$0$(SRAM_SIZE),$$00)
_HAS_SOUNDBANK := $(and $(filter 1,$(USE_SNESMOD)),$(SOUNDBANK_SRC))

# SRAM/SNESMOD auto-add modules
LIB_MODULES ?= console
ifeq ($(USE_SRAM),1)
LIB_MODULES += sram
endif
ifeq ($(USE_SNESMOD),1)
LIB_MODULES += snesmod
endif

# Assembler flags
ASFLAGS := $(if $(filter 1,$(USE_HIROM)),-D HIROM) $(if $(filter 1,$(USE_FASTROM)),-D FASTROM)

# OAM helpers (standalone projects without library)
OAM_HELPERS_OBJ := $(if $(filter 0,$(USE_LIB)),oam_helpers.o)

# Check library is built (skip for 'clean')
ifneq ($(MAKECMDGOALS),clean)
ifeq ($(USE_LIB),1)
ifeq ($(wildcard $(LIBDIR)/console.o),)
$(error Library not built. Run: cd $(OPENSNES) && make lib)
endif
endif
endif

#------------------------------------------------------------------------------
# Module Dependency Auto-Resolution
#------------------------------------------------------------------------------

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
# Library Object Resolution
#------------------------------------------------------------------------------

LIB_OBJS := $(foreach mod,$(LIB_MODULES),$(wildcard $(LIBDIR)/$(mod).o) $(wildcard $(LIBDIR)/$(mod)-asm.o))
INCLUDES := -I$(OPENSNES)/lib/include -I.
ALL_CFLAGS := $(INCLUDES) $(CFLAGS)

#------------------------------------------------------------------------------
# Derived Variables
#------------------------------------------------------------------------------

GFX_HEADERS := $(notdir $(addsuffix .h,$(basename $(GFXSRC))))
C_OBJS := $(patsubst %.c,%.c.o,$(CSRC))
ASM_OBJS := $(patsubst %.asm,%.o,$(ASMSRC))
SOUNDBANK_OBJ := $(if $(_HAS_SOUNDBANK),$(SOUNDBANK_OUT).o)

# Add soundbank header for C compilation dependency
ifneq ($(_HAS_SOUNDBANK),)
GFX_HEADERS += $(SOUNDBANK_OUT).h
endif

# Extract .incbin dependencies from ASMSRC
INCBIN_DEPS := $(if $(ASMSRC),$(shell grep -hi '\.incbin' $(ASMSRC) 2>/dev/null | \
    sed -n 's/.*\.incbin[[:space:]]*"\([^"]*\)".*/\1/p' | sort -u))

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

ifneq ($(_HAS_SOUNDBANK),)
$(SOUNDBANK_OUT).asm $(SOUNDBANK_OUT).h: $(SOUNDBANK_SRC)
	@echo "[SMCONV] Generating soundbank from: $(SOUNDBANK_SRC)"
	@$(SMCONV) -s -o $(SOUNDBANK_OUT) -b $(SOUNDBANK_BANK) -n -p $(SOUNDBANK_OUT) $(SOUNDBANK_SRC)
ifeq ($(USE_HIROM),1)
	@# HiROM: SNESMOD reads via $$00-$$3F:$$8000 mirrors, data must be at $$8000+
	@sed -i.bak 's/^\.ORG 0$$/.ORG $$8000/' $(SOUNDBANK_OUT).asm && rm -f $(SOUNDBANK_OUT).asm.bak
endif
endif

#------------------------------------------------------------------------------
# Graphics Conversion
#------------------------------------------------------------------------------

define GFX_RULE
$(notdir $(basename $(1)).pic) $(notdir $(basename $(1)).pal): $(1)
	@echo "[GFX] $$< -> $$(notdir $$(basename $$<)).pic/.pal"
	@$$(GFX4SNES) -s $$(SPRITE_SIZE) -p -i $$<
endef
$(foreach src,$(GFXSRC),$(eval $(call GFX_RULE,$(src))))

#------------------------------------------------------------------------------
# Compilation
#------------------------------------------------------------------------------

# Wrap ASM source with memmap include and assemble to object
define wrap_asm
	@{ echo '.include "$(MEMMAP_INC)"'; echo ''; cat $(1); } > $(basename $(2)).wrap.asm
	@$(AS) $(ASFLAGS) -I $(TEMPLATES) -o $(2) $(basename $(2)).wrap.asm
endef

# C sources → objects
%.c.o: %.c $(GFX_HEADERS)
	@echo "[CC] $<"
	@$(CC) $(ALL_CFLAGS) $< -o $*.c.asm
	$(call wrap_asm,$*.c.asm,$@)

#------------------------------------------------------------------------------
# Assembly Objects
#------------------------------------------------------------------------------

# Project config (numeric values via .DEFINE)
project_config.inc:
	@echo '.DEFINE CARTRIDGETYPE $(CARTRIDGETYPE)' > $@
	@echo '.DEFINE ROMSIZE_VAL $(ROMSIZE)' >> $@
	@echo '.DEFINE SRAMSIZE_VAL $(SRAMSIZE)' >> $@

# Project header (ROM_NAME via sed, rest via project_config.inc)
project_hdr.asm: $(HDR_TEMPLATE) project_config.inc
	@echo "[HDR] Generating project header ($(if $(filter 1,$(USE_HIROM)),HiROM,LoROM))..."
	@sed 's/__ROM_NAME__/$(ROM_NAME)/' $(HDR_TEMPLATE) > $@

# crt0: has its own MEMORYMAP via project_hdr.asm
crt0.o: $(TEMPLATES)/crt0.asm project_hdr.asm project_config.inc
	@echo "[AS] crt0"
	@$(AS) $(ASFLAGS) -I $(TEMPLATES) -o $@ $<

# Runtime math library
runtime.o: $(TEMPLATES)/runtime.asm
	@echo "[AS] runtime"
	$(call wrap_asm,$<,$@)

# Initialized data start marker
data_init_start.o: $(TEMPLATES)/data_init_start.asm
	@echo "[AS] data_init_start"
	$(call wrap_asm,$<,$@)

# OAM helpers (USE_LIB=0 only)
ifeq ($(USE_LIB),0)
oam_helpers.o: $(TEMPLATES)/oam_helpers.asm
	@echo "[AS] oam_helpers"
	$(call wrap_asm,$<,$@)
endif

# User ASM sources (explicit rules to avoid matching library objects)
define ASM_OBJ_RULE
$(patsubst %.asm,%.o,$(1)): $(1) $(INCBIN_DEPS)
	@echo "[AS] $(1)"
	$$(call wrap_asm,$(1),$$@)
endef
$(foreach src,$(ASMSRC),$(eval $(call ASM_OBJ_RULE,$(src))))

# Soundbank object
ifneq ($(_HAS_SOUNDBANK),)
$(SOUNDBANK_OUT).o: $(SOUNDBANK_OUT).asm
	@echo "[AS] $(SOUNDBANK_OUT)"
	$(call wrap_asm,$<,$@)
endif

# End marker (must be linked LAST)
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

linkfile: $(LINK_OBJS)
	@echo "[objects]" > $@
ifeq ($(OS),Windows_NT)
	@$(foreach obj,$(LINK_OBJS),echo "$$(cygpath -m $(obj))" >> $@;)
else
	@$(foreach obj,$(LINK_OBJS),echo "$(obj)" >> $@;)
endif

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
