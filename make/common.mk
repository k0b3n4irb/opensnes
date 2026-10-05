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
#   (BRR)     - a .wav .incbin'd as its .brr auto-converts via wav2brr
#               (one-shot SFX; --loop samples: run wav2brr by hand)
#   CFLAGS    - Additional C compiler flags
#   SPRITE_SIZE - Sprite/tile size for gfx4snes (default: 8)
#
# Every variable, with its default: docs/tools/build.md (the doc sentinel
# fails when a `?=` variable below is missing from that page).
#
# ROM configuration options:
#   USE_HIROM     - Set to 1 for HiROM mode (64KB banks instead of 32KB)
#   USE_FASTROM   - Set to 1 for FastROM (~33% faster ROM access)
#   USE_SRAM      - Set to 1 to enable battery-backed save (8KB default)
#   ROM_BANKS     - Linker bank count (default 8: 256 KB LoROM / 512 KB HiROM);
#                   the header's ROMSIZE byte and the asset bank range follow it
#   GSU_RAM_KB    - Super FX Game Pak RAM declared in the extended header
#                   ($FFBD; default 64)
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
GSU_AS   := $(OPENSNES)/bin/wla-superfx
SPC_AS   := $(OPENSNES)/bin/wla-spc700
LD       := $(OPENSNES)/bin/wlalink
GFX4SNES := $(OPENSNES)/bin/gfx4snes
SMCONV   := $(OPENSNES)/bin/smconv
WAV2BRR  := $(OPENSNES)/bin/wav2brr
PALPLAN  := $(OPENSNES)/bin/palplan
ASEPRITE2SNES := $(OPENSNES)/bin/aseprite2snes
TEMPLATES := $(OPENSNES)/templates

# Bank $00 imminent-overflow hard-fail threshold (bytes free). 0 = disabled.
# 1024 since 2026-09-23, when the examples' data moved to ASSET_SECTION and
# the corpus minimum rose from 12 bytes to 1912 (tetris). History, policy
# and the re-measure loop: .claude/rules/bank0_budget.md.
BANK0_FAIL_THRESHOLD ?= 1024

# C RAM band ($00:0000-$1FFF) budget — the RAM twin of the ROM ratchet
# above (structural defect B2: all C-accessible RAM must sit in the 8 KB
# WRAM mirror; anything higher is silently wrong-banked). Placement past
# $2000 always hard-fails; the free-space ratchet is set just below the
# corpus minimum (breakout: 668 bytes free as of 2026-07-11) so the next
# big RAMSECTION fails fast instead of corrupting at runtime. The warn
# threshold gives early drift visibility (breakout/tetris warn today —
# deliberate: they ARE within 1 KB of the ceiling).
RAM_FAIL_THRESHOLD ?= 512
RAM_WARN_THRESHOLD ?= 1024

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
SPCSRC      ?=
GFXSRC      ?=
SPRITE_SIZE ?= 8
USE_LIB     ?= 0
USE_HIROM   ?= 0
USE_FASTROM ?= 0
USE_SRAM    ?= 0
USE_SA1     ?= 0
USE_SUPERFX ?= 0
USE_DSP1    ?= 0
USE_SNESMOD ?= 0
SRAM_SIZE   ?= 3
# SA-1 BW-RAM declared in the header ($FFD8, 1 KB << n): 5 = 32 KB, the size
# every SA-1 example and the sram fixture assume; it is the work RAM of the
# cartridge whether or not the game saves (USE_SRAM adds the battery).
SA1_BWRAM_SIZE ?= 5
SOUNDBANK_SRC ?=
SOUNDBANK_OUT ?= soundbank
SOUNDBANK_BANK ?= 1
GSUSRC      ?=
# Bytes of WRAM, at the top of bank $7E, for code that runs from RAM
# (RAM_CODE_SECTION, templates/ram_code_start.asm). 0 = no window.
RAM_CODE_SIZE ?= 0
# ROM size as a project knob (2026-09-24). ROM_BANKS is the linker's bank
# count (32 KB banks on LoROM / SA-1 / Super FX, 64 KB on HiROM); the header
# byte ROMSIZE (1 KB << n) and the asset bank range follow it unless set by
# hand. Defaults reproduce the historical 8 banks: 256 KB LoROM, 512 KB HiROM
# (whose header used to claim 256 KB — a lie fixed by computing it).
ROM_BANKS   ?= 8
ROM_BANK_KB := $(if $(filter 1,$(USE_HIROM)),64,32)
# Rounded UP to the next power of two: snesdev-wiki, ROM header ($FFD7,
# "rounded up"); int(log2) declared 256 KB for a 384 KB ROM until 2026-09-26.
# Shell arithmetic, not Python: a user build must not need an interpreter
# (.claude/rules/two_audiences.md, 2026-10-05).
ROMSIZE     ?= $(shell n=0; s=$$(( $(ROM_BANKS) * $(ROM_BANK_KB) )); while [ $$(( 1 << n )) -lt $$s ]; do n=$$(( n + 1 )); done; printf '$$%02X' $$n)
ASSET_BANKS_RANGE ?= $(shell echo $$(( $(ROM_BANKS) - 1 )))-1
# Super FX Game Pak RAM declared in the extended header ($FFBD, 1 KB << n).
GSU_RAM_KB  ?= 64
GSU_RAM_SIZE_VAL := $(shell n=0; while [ $$(( 1 << (n + 1) )) -le $(GSU_RAM_KB) ]; do n=$$(( n + 1 )); done; printf '$$%02X' $$n)
# Region the cartridge declares in its header, $FFD9 (2026-10-02; it was
# $01, North America, on every ROM). fullsnes "Country (also implies
# PAL/NTSC)" and snesdev-wiki ROM header: $00 Japan and $01 USA are NTSC,
# $02 Europe is PAL. A console runs at its own standard whatever the byte
# says (STAT78 bit 4 is PPU2 pin 30, fullsnes f355d4389d653957; getRegion()
# reads it), but emulators, luna included, pick 50 or 60 Hz from the byte.
ROM_REGION  ?= ntsc
COUNTRY_VAL := $(if $(filter ntsc,$(ROM_REGION)),$$01,$(if $(filter pal,$(ROM_REGION)),$$02,$(if $(filter jp,$(ROM_REGION)),$$00,)))
ifeq ($(COUNTRY_VAL),)
$(error ROM_REGION=$(ROM_REGION) is not one of ntsc, pal, jp (the header byte $$FFD9: $$01 USA/NTSC, $$02 Europe/PAL, $$00 Japan/NTSC))
endif

# Derived configuration (one-liners using $(if))
LIBDIR       := $(OPENSNES)/lib/build/$(if $(filter 1,$(USE_SA1)),sa1,$(if $(filter 1,$(USE_SUPERFX)),superfx,$(if $(filter 1,$(USE_HIROM)),hirom,lorom)))
RUNTIME_OBJ  := $(LIBDIR)/runtime-asm.o
# 32-bit math helpers (always linked post-A1-followup chantier — every long
# arithmetic op may emit jsl __mul32 / jsl __[s|u]divmod32 once cproc maps
# the 4-byte non-float class to Kl).
MUL32_OBJ    := $(LIBDIR)/mul32-asm.o
DIV32_OBJ    := $(LIBDIR)/div32-asm.o
HDR_TEMPLATE := $(TEMPLATES)/$(if $(filter 1,$(USE_SA1)),hdr_sa1.asm,$(if $(filter 1,$(USE_SUPERFX)),hdr_superfx.asm,$(if $(filter 1,$(USE_HIROM)),hdr_hirom.asm,hdr.asm)))
# No superfx memmap branch: GSU cartridges are LoROM-mapped on the 65816
# side (the GSU has its own ROM view), so plain memmap.inc is intentional.
MEMMAP_INC   := $(if $(filter 1,$(USE_SA1)),memmap_sa1.inc,$(if $(filter 1,$(USE_HIROM)),memmap_hirom.inc,memmap.inc))
# $FFD6 (snesdev-wiki, ROM header, cartouche dc4d4f3adf2cd2f3): high nibble
# = coprocessor ($0x DSP, $1x GSU, $3x SA-1), low nibble = what sits beside
# it ($x0 ROM, $x2 ROM+RAM+battery; with a coprocessor $x3, $x5 +RAM+battery).
# DSP-1 + SRAM is $05 (it was $03, "no RAM", with the sram module linked).
# SA-1: $34 = SA-1 + RAM (BW-RAM, always on the board), $35 = + battery when
# the game saves (snesdev-wiki ROM header, $x4 / $x5; until 2026-10-04 every
# SA-1 ROM declared a battery and luna wrote a .srm for it).
CARTRIDGETYPE := $(if $(filter 1,$(USE_SA1)),$(if $(filter 1,$(USE_SRAM)),$$35,$$34),$(if $(filter 1,$(USE_SUPERFX)),$(if $(filter 1,$(USE_SRAM)),$$15,$$13),$(if $(filter 1,$(USE_DSP1)),$(if $(filter 1,$(USE_SRAM)),$$05,$$03),$(if $(filter 1,$(USE_SRAM)),$$02,$$00))))
SRAMSIZE     := $(if $(filter 1,$(USE_SA1)),$$0$(SA1_BWRAM_SIZE),$(if $(filter 1,$(USE_SUPERFX)),$$00,$(if $(filter 1,$(USE_SRAM)),$$0$(SRAM_SIZE),$$00)))
_HAS_SOUNDBANK := $(and $(filter 1,$(USE_SNESMOD)),$(SOUNDBANK_SRC))

# SRAM/SNESMOD/SuperFX auto-add modules (duplicates are harmless — the
# dependency resolver below runs $(sort) which dedups)
LIB_MODULES ?= console
# Knob combinations that would build a ROM with a wrong header or three
# memory maps, refused with the reason (2026-09-26; until then only SA-1 +
# SRAM was). One ROM, one coprocessor, one mapping.
_COPROCS := $(strip $(filter 1,$(USE_SA1)) $(filter 1,$(USE_SUPERFX)) $(filter 1,$(USE_DSP1)))
ifneq ($(words $(_COPROCS)),$(filter 0 1,$(words $(_COPROCS))))
$(error USE_SA1, USE_SUPERFX and USE_DSP1 are exclusive: a cartridge has one coprocessor (header byte $$FFD6 names one))
endif
ifeq ($(USE_HIROM)$(USE_SUPERFX),11)
$(error USE_HIROM=1 with USE_SUPERFX=1 is not supported: Super FX carts are LoROM-mapped on the 65816 side (hdr_superfx.asm, memmap.inc); the build would mix the HiROM memory map into a LoROM header)
endif
ifeq ($(USE_HIROM)$(USE_SA1),11)
$(error USE_HIROM=1 with USE_SA1=1 is not supported: SA-1 has its own mapping (memmap_sa1.inc, hdr_sa1.asm))
endif
ifeq ($(USE_HIROM)$(USE_DSP1),11)
$(error USE_HIROM=1 with USE_DSP1=1 is not supported: the dsp1 module drives the LoROM board ($$30:8000 data, $$30:C000 status); the HiROM DSP-1 board maps it elsewhere)
endif
# GSU_BANK: link the GSU program at $$n:8000 (see the .sfx rule below).
GSU_BANK ?=
ifneq ($(GSU_BANK),)
ifneq ($(USE_SUPERFX),1)
$(error GSU_BANK is set without USE_SUPERFX=1: it places a Super FX program)
endif
ifneq ($(words $(GSUSRC)),1)
$(error GSU_BANK needs exactly one file in GSUSRC (got: $(GSUSRC)): one program is linked at $$$(GSU_BANK):8000 — .include the others from it)
endif
ifeq ($(GSU_BANK),0)
$(error GSU_BANK=0 is not a program bank: bank 0 holds the code and the header; use 1..$(shell expr $(ROM_BANKS) - 1) (docs/tools/build.md))
endif
endif
# ROM_NAME goes into the header as 21 ASCII bytes ($20-$7E) through sed:
# a longer name was cut without a word, a non-ASCII one wrote UTF-8 into
# the header, and `/` or `&` broke the sed (build-tools audit, 2026-10-04).
ifneq ($(shell printf '%s' '$(ROM_NAME)' | LC_ALL=C grep -c '[^ -~]'),0)
$(error ROM_NAME "$(ROM_NAME)" has a character outside printable ASCII ($$20-$$7E): the header holds 21 such bytes)
endif
ifneq ($(findstring /,$(ROM_NAME))$(findstring &,$(ROM_NAME))$(findstring \,$(ROM_NAME)),)
$(error ROM_NAME "$(ROM_NAME)" contains / & or \, which the header substitution cannot carry)
endif
ifneq ($(shell n=$$(printf '%s' '$(ROM_NAME)' | LC_ALL=C wc -c); [ "$$n" -gt 21 ] && echo long),)
$(error ROM_NAME "$(ROM_NAME)" is longer than the 21 characters of the header title)
endif
ifeq ($(USE_SRAM),1)
ifeq ($(filter 1 2 3 4 5 6 7,$(SRAM_SIZE)),)
$(error SRAM_SIZE=$(SRAM_SIZE) is out of range: the header byte $$FFD8 is 1 KB << n, n = 1..7 (2 KB..128 KB); 3 = 8 KB is the default)
endif
endif
ifeq ($(shell [ "$(ROM_BANKS)" -gt 0 ] 2>/dev/null && echo ok),)
$(error ROM_BANKS=$(ROM_BANKS) must be a positive number of banks)
endif
# Upper bounds per mapping (2026-10-03): past them the linker still places
# data, in memory the cartridge does not map as ROM, and the build stays
# green — a string at $7E:8000 is WRAM, .rodata at $40:0000 is SA-1 BW-RAM,
# a HiROM label past bank $7F has no 24-bit address. The DSP-1 board the
# dsp1 module drives is the 1 MB LoROM one (fullsnes, DSP-1 LoROM: data and
# status in $30-$3F:8000-$FFFF). Lower bound 8: the prebuilt library pins
# its asset sections to banks 7-1.
ROM_BANKS_MAX := $(if $(filter 1,$(USE_HIROM)),64,$(if $(filter 1,$(USE_SA1)),64,$(if $(filter 1,$(USE_SUPERFX)),64,$(if $(filter 1,$(USE_DSP1)),32,126))))
ifeq ($(shell [ "$(ROM_BANKS)" -ge 8 ] && [ "$(ROM_BANKS)" -le "$(ROM_BANKS_MAX)" ] 2>/dev/null && echo ok),)
$(error ROM_BANKS=$(ROM_BANKS) is out of range for this mapping: 8 to $(ROM_BANKS_MAX) (LoROM 126 banks of 32 KB before WRAM at $$7E; HiROM, SA-1 and Super FX 64; DSP-1 32, the 1 MB board))
endif
ifeq ($(filter 1,$(USE_SUPERFX)),1)
ifeq ($(filter 32 64 128,$(GSU_RAM_KB)),)
$(error GSU_RAM_KB=$(GSU_RAM_KB) must be 32, 64 or 128: the header byte $$FFBD is 1 KB << n and the GSU boards carry 32, 64 or 128 KB)
endif
endif
ifeq ($(USE_SRAM),1)
LIB_MODULES += sram
endif
ifeq ($(USE_SNESMOD),1)
LIB_MODULES += snesmod
endif
ifeq ($(USE_SUPERFX),1)
LIB_MODULES += superfx
endif
ifeq ($(USE_DSP1),1)
LIB_MODULES += dsp1
endif

# Assembler flags
ASFLAGS := -D ROM_BANKS_VAL=$(ROM_BANKS) -D 'ASSET_BANKS_VAL="$(ASSET_BANKS_RANGE)"' $(if $(filter 1,$(USE_HIROM)),-D HIROM) $(if $(filter 1,$(USE_SA1)),-D SA1) $(if $(filter 1,$(USE_SUPERFX)),-D SUPERFX) $(if $(filter 1,$(USE_DSP1)),-D DSP1) $(if $(filter 1,$(USE_FASTROM)),-D FASTROM)
ifneq ($(shell [ $(RAM_CODE_SIZE) -ge 0 ] && [ $(RAM_CODE_SIZE) -le 16384 ] && echo ok),ok)
$(error RAM_CODE_SIZE=$(RAM_CODE_SIZE): must be a byte count from 0 to 16384)
endif
# The SDK's own share of the window, added to the project's: a Super FX
# build keeps its interrupt entries and gsuLaunch's wait loop there
# (crt0 gsu_nmi_blob & co., lib superfx.asm), 2026-09-29.
ifneq ($(GSU_BANK),)
ASFLAGS += -D GSU_BANK_VAL=$(GSU_BANK)
endif
RAM_CODE_SDK   := $(if $(filter 1,$(USE_SUPERFX)),768,0)
RAM_CODE_TOTAL := $(shell echo $$(( $(RAM_CODE_SIZE) + $(RAM_CODE_SDK) )))
ifneq ($(RAM_CODE_TOTAL),0)
ASFLAGS += -D RAM_CODE -D RAM_CODE_ORG_VAL=$(shell echo $$(( 65536 - $(RAM_CODE_TOTAL) )))
RAM_CODE_START_OBJ := ram_code_start.o
RAM_CODE_END_OBJ   := ram_code_end.o
endif


# Check library is built (skip for 'clean')
# Runtime is always required (provides __mul16, __div16, etc.)
ifneq ($(MAKECMDGOALS),clean)
ifeq ($(wildcard $(RUNTIME_OBJ)),)
$(error Library not built. Run: cd $(OPENSNES) && make lib)
endif
endif

#------------------------------------------------------------------------------
# Module Dependency Auto-Resolution
#------------------------------------------------------------------------------

_DEP_sprite          := dma sprite_oamset
# The seven entries marked L2a were found by devtools/link_modules.py
# (2026-09-13), which links every module ALONE: each of these modules
# referenced a symbol of a module it never declared, and only linked in
# practice because every example also listed console / sprite / dma.
_DEP_sprite_dynamic  := sprite_dynamic_dispatch sprite_dynamic_helpers sprite sprite_lut   # L2a: oamInit, lkup32oamS
_DEP_sprite_dynamic_dispatch := sprite_dynamic                                  # L2a: oamInitDynamicSprite
_DEP_sprite_dynamic_meta     := sprite sprite_dynamic                           # L2a: oambuffer
_DEP_text            := dma background console                                  # L2a: consoleInit
_DEP_text4bpp        := dma
_DEP_object          := map sprite sprite_dynamic                               # L2a: oambuffer (sprite_dynamic's)
_DEP_map             := dma
_DEP_background      := dma                                                     # L2a: dmaCopyVram
_DEP_fixed32         := math                                                    # L2a: sine_table
_DEP_snesmod         := console
# console's C references clearNmiFlag/unmaskIrq/clearIrqFlag (dma.asm) —
# surfaced by the first example linking console WITHOUT dma (SPC700 arc)
_DEP_console         := dma
_DEP_superfx         := dma hdma background console
_DEP_hdma            := dma math_sqrt
# math splits into the small sqrt module (math_sqrt = sqrt16 + fixSqrt
# only) and the larger trig + arithmetic module (math = sine LUT +
# atan_lut + fixSin/fixCos/fixDiv/fixMul/fixAbs/fixClamp/fixLerp +
# atan2_8 + mul16/div16/mod16). hdma needs only sqrt16 (for iris-wipe
# radius), so we depend on math_sqrt to keep hdma users light. Anyone
# listing `math` in LIB_MODULES still gets sqrt because math depends
# on math_sqrt — the resolver flattens this transitively.
_DEP_math            := math_sqrt
_DEP_asset           := dma background
_DEP_panel           := dma console
_DEP_tile            :=                                                         # tileEncode*: pure C, no dependency
# audio v2: C layer needs the apu upload primitives + the embedded
# SPC700 driver image (audio_blob.asm -> audio_blob-asm.o)
_DEP_audio           := apu audio_blob

_resolve_one = $(1) $(foreach m,$(1),$(_DEP_$(m)))
_resolve_deps = $(sort $(call _resolve_one,$(call _resolve_one,$(call _resolve_one,$(1)))))
LIB_MODULES := $(call _resolve_deps,$(LIB_MODULES))

#------------------------------------------------------------------------------
# Library Object Resolution
#------------------------------------------------------------------------------

LIB_OBJS := $(foreach mod,$(LIB_MODULES),$(wildcard $(LIBDIR)/$(mod).o) $(wildcard $(LIBDIR)/$(mod)-asm.o))
# A module name that matches no object is an error, not a silent omission
# (until 2026-09-26 `LIB_MODULES := consol dma` linked dma alone and the typo
# surfaced as an "unknown symbol" from wlalink). superfx gets its own reason:
# its asm half exists only in the Super FX flavour of the lib.
ifeq ($(USE_LIB),1)
ifneq ($(filter superfx,$(LIB_MODULES)),)
ifneq ($(USE_SUPERFX),1)
$(error LIB_MODULES: `superfx` needs USE_SUPERFX=1 — its C half is in every lib flavour but its asm half (gsu_cfgr, gsuLaunch…) only in lib/build/superfx, so it cannot link elsewhere)
endif
endif
_LIB_AVAILABLE = $(sort $(patsubst %-asm,%,$(basename $(notdir $(wildcard $(LIBDIR)/*.o)))))
$(foreach mod,$(LIB_MODULES),$(if $(wildcard $(LIBDIR)/$(mod).o $(LIBDIR)/$(mod)-asm.o),,\
  $(error LIB_MODULES: no library module `$(mod)` in $(LIBDIR). Available: $(_LIB_AVAILABLE))))
endif
INCLUDES := -I$(OPENSNES)/lib/include -I.
ALL_CFLAGS := $(INCLUDES) $(CFLAGS)

#------------------------------------------------------------------------------
# Assets by their settings file (2026-10-05, docs/tools/CONVENTIONS.md)
#------------------------------------------------------------------------------
# The rules below come before `all:`; the default goal stays the ROM.
.DEFAULT_GOAL := all
# Every <asset>.toml beside a source (res/hero.png.toml) or named after what it
# produces (res/soundbank.toml) says which opensnes-* tool converts it, in its
# `tool` line, and which subcommand, in its one [table]. The build runs the
# tool once per file (a .done stamp beside the .toml), before any C or ASM
# object, then gathers every <stem>_data.as the tools wrote into
# assets_gen.asm — the .include of every <stem>_data.as (each carries its own
# ASSET_SECTIONs, one per blob), assembled with the
# project. The artist never opens this Makefile; a hand-written data.asm
# and the GFXSRC rule below keep working for the projects that have them.
# ASSET_TOML lists the files (default: *.toml and res/*.toml that name a tool).
ASSET_TOML ?= $(foreach t,$(wildcard *.toml res/*.toml),$(if $(shell grep -l '^tool = "opensnes-' $(t) 2>/dev/null),$(t),))
ASSET_STAMPS := $(addsuffix .done,$(ASSET_TOML))
# a level (opensnes-level) reads the .map its tileset's conversion wrote: it converts after the others
LEVEL_STAMPS := $(addsuffix .done,$(foreach t,$(ASSET_TOML),$(if $(shell grep -l '^tool = "opensnes-level"' $(t) 2>/dev/null),$(t),)))
ifneq ($(ASSET_TOML),)
ASMSRC += assets_gen.asm
endif
define ASSET_RULE
$(1).done: $(1) $(wildcard $(basename $(1))) .opensnes_config
	@tool=$$$$(sed -n 's/^tool *= *"\([^"]*\)".*/\1/p' $(1) | head -1); \
	 sub=$$$$(sed -n 's/^\[\([a-z_]*\)\].*/\1/p' $(1) | head -1); \
	 asset=$(basename $(1)); [ -f "$$$$asset" ] || asset=$(1); \
	 echo "[$$$$tool] $$$$sub $$$$asset"; \
	 $(OPENSNES)/bin/$$$$tool $$$$sub -q $$$$asset && touch $$@
endef
$(foreach t,$(ASSET_TOML),$(eval $(call ASSET_RULE,$(t))))
$(LEVEL_STAMPS): $(filter-out $(LEVEL_STAMPS),$(ASSET_STAMPS))
assets_gen.asm: $(ASSET_STAMPS)
	@{ echo '; generated by make/common.mk from the settings files of the assets - do not edit'; \
	   for t in $(ASSET_TOML); do stem=$${t%.toml}; echo $${stem%.*}; done | sort -u | \
	     while read stem; do [ -f "$$stem"_data.as ] && echo ".include \"$${stem}_data.as\""; done; } > $@

#------------------------------------------------------------------------------
# Derived Variables
#------------------------------------------------------------------------------

GFX_HEADERS := $(notdir $(addsuffix .h,$(basename $(GFXSRC))))
C_OBJS := $(patsubst %.c,%.c.o,$(CSRC))

# Example C objects must rebuild when the lib API changes (issue #105:
# incremental builds linked stale-ABI objects against a new lib; the
# aim_target WRAM stream diverged from a clean build twice before this).
# Coarse — any header change recompiles the example — but correct.
LIB_HEADERS := $(wildcard $(OPENSNES)/lib/include/snes.h) $(wildcard $(OPENSNES)/lib/include/snes/*.h)
ASM_OBJS := $(patsubst %.asm,%.o,$(ASMSRC))
GSU_BINS := $(patsubst %.sfx,%.sfx.bin,$(GSUSRC))
GSU_HEADERS := $(patsubst %.sfx,%.sfx.h,$(GSUSRC))
SPC_BINS := $(patsubst %.spc700.asm,%.spc700.bin,$(SPCSRC))
SOUNDBANK_OBJ := $(if $(_HAS_SOUNDBANK),$(SOUNDBANK_OUT).o)

# Add soundbank header for C compilation dependency
ifneq ($(_HAS_SOUNDBANK),)
GFX_HEADERS += $(SOUNDBANK_OUT).h
endif

# Extract .incbin dependencies from ASMSRC
INCBIN_DEPS := $(if $(ASMSRC),$(shell grep -hi '\.incbin' $(ASMSRC) 2>/dev/null | \
    sed -n 's/.*\.incbin[[:space:]]*"\([^"]*\)".*/\1/p' | sort -u))
# .incbin dependencies INSIDE SPC700 programs (surfaced on the SPC700 arc: a
# regenerated BRR sample did not rebuild the .spc700.bin)
SPC_INCBIN_DEPS := $(if $(SPCSRC),$(shell grep -hi '\.incbin' $(SPCSRC) 2>/dev/null | \
    sed -n 's/.*\.incbin[[:space:]]*"\([^"]*\)".*/\1/p' | sort -u))
# GSU/SPC binaries must be built before ASM objects that .incbin them
INCBIN_DEPS += $(GSU_BINS) $(SPC_BINS)

#------------------------------------------------------------------------------
# Build Targets
#------------------------------------------------------------------------------

.PHONY: all clean test test-update

all: $(TARGET)
	@echo "==============================================="
	@echo "  Built: $(TARGET)"
	@echo "  Size: $$(wc -c < $(TARGET) | tr -d ' ') bytes"
	@echo "==============================================="

#------------------------------------------------------------------------------
# SNESMOD Soundbank Conversion
#------------------------------------------------------------------------------

ifneq ($(_HAS_SOUNDBANK),)
# One recipe, one target: with both .asm and .h as targets of one rule, a
# parallel make ran smconv twice and the second run truncated the .bnk the
# first assembly was reading (2026-10-04, libtests_fx under make tests).
$(SOUNDBANK_OUT).h: $(SOUNDBANK_OUT).asm
	@test -f $@ || { echo "soundbank: $@ missing after smconv" >&2; exit 1; }
$(SOUNDBANK_OUT).asm: $(SOUNDBANK_SRC) .opensnes_config
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
$(notdir $(basename $(1)).pic) $(notdir $(basename $(1)).pal): $(1) .opensnes_config
	@echo "[GFX] $$< -> $$(notdir $$(basename $$<)).pic/.pal"
	@$$(GFX4SNES) -s $$(SPRITE_SIZE) -p -i $$<
endef
$(foreach src,$(GFXSRC),$(eval $(call GFX_RULE,$(src))))

#------------------------------------------------------------------------------
# BRR sample conversion — .wav → .brr (one-shot SFX)
#------------------------------------------------------------------------------
# Zero-config: drop a PCM .wav next to your source and .incbin the matching
# .brr in an ASM file. The .incbin dependency (INCBIN_DEPS above) makes the
# .brr a prerequisite, and this rule generates it with wav2brr. For a looping
# sample, run wav2brr --loop by hand and commit the .brr instead.
%.brr: %.wav
	@echo "[BRR] $< -> $@"
	@$(WAV2BRR) $< $@

#------------------------------------------------------------------------------
# SuperFX (GSU) Assembly — two-stage build: .sfx → .sfx.o → .sfx.bin
#------------------------------------------------------------------------------

ifneq ($(GSUSRC),)
# The link also writes <name>.sfx.h (2026-09-29): one #define per global
# label of the GSU program, its offset in the binary — the entry points C
# passes to gsuCall() / gsuStartCached(). gsu_job.sfx's label `add_job`
# becomes GSU_JOB_ADD_JOB. Labels starting with _ or @ are local, skipped.
#
# GSU_BANK = n (2026-10-03) links the program at its real address: it is
# assembled at $$8000 (memmap_gsu.inc) and GSU_SECTION (assets.inc) puts it
# at the start of ROM bank n, so its labels are addresses the GSU can jump
# to and read from. The .sfx.h keeps OFFSETS either way (label - $$8000):
# gsuCall() and gsuStartCached() add them to the program's address.
%.sfx.bin %.sfx.h: %.sfx $(TEMPLATES)/memmap_gsu.inc .opensnes_config
	@echo "[GSU] $< -> $*.sfx.bin, $*.sfx.h$(if $(GSU_BANK), (linked at bank $(GSU_BANK), \$$8000))"
	@$(GSU_AS) $(if $(GSU_BANK),-D GSU_BANK=$(GSU_BANK)) -I $(TEMPLATES) -o $*.sfx.o $<
	@echo "[objects]" > $*.sfx.link
	@echo "$*.sfx.o" >> $*.sfx.link
	@$(LD) -S -b $*.sfx.link $*.sfx.bin
	@P=$$(echo '$(notdir $*)' | tr 'a-z' 'A-Z' | tr -c 'A-Z0-9\n' '_'); \
	{ echo "/* Generated from $< by make/common.mk: GSU program entry points,"; \
	  echo " * offsets in $*.sfx.bin (gsuCall, gsuStartCached). Do not edit. */"; \
	  echo "#ifndef $${P}_SFX_H"; echo "#define $${P}_SFX_H"; \
	  awk -v P="$$P" '/^\[labels\]/{f=1;next} /^\[/{f=0} \
	    f && NF==2 && $$2 ~ /^[A-Za-z][A-Za-z0-9_]*$$/ { split($$1,a,":"); o=a[2]; \
	    d=index("89abcdef", tolower(substr(o,1,1))); \
	    if (d) o=(d-1) substr(o,2); \
	    printf "#define %s_%s 0x%su\n", P, toupper($$2), o }' $*.sfx.sym; \
	  echo "#endif"; } > $*.sfx.h
	@rm -f $*.sfx.o $*.sfx.link $*.sfx.sym
endif

# SPC700 (APU) programs: same two-stage shape as the GSU. %.spc700.asm
# assembles with wla-spc700 against memmap_spc700.inc into a flat
# binary that the lib's apuUpload() pushes over the IPL protocol.
ifneq ($(SPCSRC),)
%.spc700.bin: %.spc700.asm $(SPC_INCBIN_DEPS)
	@echo "[SPC] $< -> $@"
	@$(SPC_AS) -I $(TEMPLATES) -o $*.spc700.o $<
	@echo "[objects]" > $*.spc700.link
	@echo "$*.spc700.o" >> $*.spc700.link
	@$(LD) -b $*.spc700.link $@
	@rm -f $*.spc700.o $*.spc700.link
endif

#------------------------------------------------------------------------------
# Compilation
#------------------------------------------------------------------------------

# Wrap ASM source with memmap include and assemble to object.
# Every rule using `wrap_asm` MUST include $(MEMMAP_DEP) in its prerequisites
# so that a change to the memory-map template re-triggers the build — the
# .wrap.asm file is regenerated each invocation, but `make` only knows to
# invoke the rule when its declared deps change.
MEMMAP_DEP := $(TEMPLATES)/$(MEMMAP_INC)

define wrap_asm
	@{ echo '.include "$(MEMMAP_INC)"'; echo '.include "assets.inc"'; echo ''; cat $(1); } > $(basename $(2)).wrap.asm
	@$(AS) $(ASFLAGS) -I $(TEMPLATES) -o $(2) $(basename $(2)).wrap.asm
endef

# Lint flags for the optional clang syntax check (cproc ignores -W flags).
# Disable host-vs-target false positives: SNES has 16-bit pointers, so casting
# &fill_value (a u16*) to u16 and (vu8*)0x4300 from int are LEGITIMATE here
# even though clang's host model would flag them. -Wno-unused-parameter
# silences callback signatures (e.g. object engine init takes minx/maxx that
# specific objects ignore — the ABI requires them).
CLANG_LINT_FLAGS := -fsyntax-only -Wall -Wextra -Werror \
	-Wno-pointer-to-int-cast -Wno-int-to-pointer-cast \
	-Wno-unused-parameter -Wno-error=deprecated-declarations \
	-Wno-error=deprecated-pragma

# C sources → objects
# Step 1: clang syntax check (cproc has no built-in -W flags so a sibling
#         compiler runs the warnings cc65816 silently swallows). -Werror
#         makes any warning fail the build — the SDK is currently warning-
#         clean and stays that way.
# Step 2: cc65816 (cproc + QBE) → 65816 assembly.
# Step 3: wrap with memmap and assemble via wla-65816.
# SKIP_LINT=1 disables the syntax check (escape hatch for environments
# without clang; CI always runs with the check enabled).
# Local headers count too (2026-09-26): tetris's main.c includes board.h,
# piece.h, render.h and hud.h, and editing them rebuilt nothing. Every .h
# next to a C source, rather than exact -MD deps: cheap and never stale.
LOCAL_HEADERS := $(wildcard *.h $(addsuffix *.h,$(filter-out ./,$(sort $(dir $(CSRC))))))
%.c.o: %.c $(GFX_HEADERS) $(GSU_HEADERS) $(MEMMAP_DEP) $(LIB_HEADERS) $(LOCAL_HEADERS) .opensnes_config | $(ASSET_STAMPS)
ifneq ($(SKIP_LINT),1)
	@if command -v clang >/dev/null 2>&1; then \
		clang $(CLANG_LINT_FLAGS) -I $(OPENSNES)/lib/include $< || \
			(echo "  lint failed for $< — fix the warning or use SKIP_LINT=1 to bypass"; exit 1); \
	elif command -v python3 >/dev/null 2>&1; then \
		python3 $(OPENSNES)/devtools/check_upgrade.py -q $< || \
			echo "  (deprecated names above: removed at 1.0 — docs/UPGRADING.md; the clang pre-pass is absent on this machine)"; \
	fi
endif
	@echo "[CC] $<"
	@$(CC) $(ALL_CFLAGS) $< -o $*.c.asm
	$(call wrap_asm,$*.c.asm,$@)

#------------------------------------------------------------------------------
# Assembly Objects
#------------------------------------------------------------------------------

# Build-config stamp (2026-09-26). project_config.inc had no prerequisite:
# once it existed, changing USE_SRAM, USE_FASTROM, ROM_BANKS, SRAM_SIZE… was
# ignored, and the old header shipped. The stamp holds every knob that
# reaches the header, the assembler flags or the link; it is rewritten only
# when that text changes, so a rebuild with the same knobs stays a no-op.
_CONFIG_TEXT := $(CARTRIDGETYPE) $(ROMSIZE) $(SRAMSIZE) $(GSU_RAM_SIZE_VAL) $(COUNTRY_VAL) $(SPRITE_SIZE) \
  [$(ROM_NAME)] [$(ASFLAGS)] [$(CFLAGS)] [$(LIB_MODULES)] [$(LIBDIR)] \
  [$(USE_SNESMOD) $(SOUNDBANK_BANK)]
.opensnes_config: FORCE
	@printf '%s\n' '$(subst ','"'"',$(_CONFIG_TEXT))' > $@.tmp
	@if cmp -s $@.tmp $@; then rm -f $@.tmp; else mv $@.tmp $@; fi
.PHONY: FORCE
FORCE:

# Project config (numeric values via .DEFINE)
project_config.inc: .opensnes_config
	@echo '.DEFINE CARTRIDGETYPE $(CARTRIDGETYPE)' > $@
	@echo '.DEFINE ROMSIZE_VAL $(ROMSIZE)' >> $@
	@echo '.DEFINE SRAMSIZE_VAL $(SRAMSIZE)' >> $@
	@echo '.DEFINE GSU_RAM_SIZE_VAL $(GSU_RAM_SIZE_VAL)' >> $@
	@echo '.DEFINE COUNTRY_VAL $(COUNTRY_VAL)' >> $@

# Project header (ROM_NAME padded to 21 chars with spaces, then sed into template)
project_hdr.asm: $(HDR_TEMPLATE) project_config.inc .opensnes_config
	@echo "[HDR] Generating project header ($(if $(filter 1,$(USE_HIROM)),HiROM,LoROM))..."
	@padded=$$(printf "%-21.21s" "$(ROM_NAME)") && sed "s/__ROM_NAME__/$$padded/" $(HDR_TEMPLATE) > $@

# SA-1 boot stub: use example-local sa1_boot.asm if present, otherwise template
SA1_BOOT_SRC := $(if $(wildcard sa1_boot.asm),sa1_boot.asm,$(TEMPLATES)/sa1_boot.asm)
project_sa1_boot.asm: $(SA1_BOOT_SRC)
	@cp $< $@

# crt0: has its own MEMORYMAP via project_hdr.asm
crt0.o: $(TEMPLATES)/crt0.asm project_hdr.asm project_config.inc project_sa1_boot.asm .opensnes_config
	@echo "[AS] crt0"
	@$(AS) $(ASFLAGS) -I $(TEMPLATES) -o $@ $<

# Initialized data start marker
data_init_start.o: $(TEMPLATES)/data_init_start.asm $(MEMMAP_DEP) .opensnes_config
	@echo "[AS] data_init_start"
	$(call wrap_asm,$<,$@)

# RAM code window markers (RAM_CODE_SIZE > 0)
ram_code_start.o: $(TEMPLATES)/ram_code_start.asm $(MEMMAP_DEP) .opensnes_config
	@echo "[AS] ram_code_start"
	$(call wrap_asm,$<,$@)
ram_code_end.o: $(TEMPLATES)/ram_code_end.asm $(MEMMAP_DEP) .opensnes_config
	@echo "[AS] ram_code_end"
	$(call wrap_asm,$<,$@)

# User ASM sources (explicit rules to avoid matching library objects)
define ASM_OBJ_RULE
$(patsubst %.asm,%.o,$(1)): $(1) $(INCBIN_DEPS) $(MEMMAP_DEP) .opensnes_config | $(ASSET_STAMPS)
	@echo "[AS] $(1)"
	$$(call wrap_asm,$(1),$$@)
endef
$(foreach src,$(ASMSRC),$(eval $(call ASM_OBJ_RULE,$(src))))

# Soundbank object
ifneq ($(_HAS_SOUNDBANK),)
$(SOUNDBANK_OUT).o: $(SOUNDBANK_OUT).asm $(MEMMAP_DEP) .opensnes_config
	@echo "[AS] $(SOUNDBANK_OUT)"
	$(call wrap_asm,$<,$@)
endif

# End marker (must be linked LAST)
data_init_end.o: $(TEMPLATES)/data_init_end.asm $(MEMMAP_DEP) .opensnes_config
	@echo "[AS] data_init_end"
	$(call wrap_asm,$<,$@)

#------------------------------------------------------------------------------
# Linking
#------------------------------------------------------------------------------

# All objects in link order
LINK_OBJS := crt0.o $(RUNTIME_OBJ) $(RAM_CODE_START_OBJ) data_init_start.o $(ASM_OBJS) $(C_OBJS)
ifeq ($(USE_LIB),1)
LINK_OBJS += $(LIB_OBJS)
endif
LINK_OBJS += $(MUL32_OBJ) $(DIV32_OBJ) $(SOUNDBANK_OBJ) $(RAM_CODE_END_OBJ) data_init_end.o

linkfile: $(LINK_OBJS) .opensnes_config
	@echo "[objects]" > $@
ifeq ($(OS),Windows_NT)
	@$(foreach obj,$(LINK_OBJS),echo "$$(cygpath -m $(obj))" >> $@;)
else
	@$(foreach obj,$(LINK_OBJS),echo "$(obj)" >> $@;)
endif

$(TARGET): linkfile
	@echo "[LD] $@"
	@$(LD) -S linkfile $@
ifeq ($(USE_SA1),1)
	@# SA-1: patch map mode byte at ROM offset $7FD5 from $20 (LoROM) to $23 (SA-1)
	@# or from $30 (FastROM+LoROM) to $33 (FastROM+SA-1). Adds $03 to the byte.
	@# Implementation lives in tools/sa1-patch/ (audit P2.4 #3 — replaces the
	@# inline Python one-liner that used to live here).
	@$(OPENSNES)/bin/sa1_patch $@ && echo "[SA1] Patched $$FFD5 map mode to SA-1"
endif
	@# Post-link checks (2026-10-05, .claude/rules/two_audiences.md): one compiled
	@# tool, opensnes-rom check, runs what five Python scripts used to run here —
	@# the bank $$00 ROM ratchet (BANK0_FAIL_THRESHOLD; .claude/rules/bank0_budget.md),
	@# the C RAM band budget (RAM_FAIL_THRESHOLD / RAM_WARN_THRESHOLD; FAR is the
	@# way above $$2000), the data-init sentinel (an object linked after
	@# data_init_end.o boots uninitialised), the bank-blind read guard (issue
	@# #104: a 16-bit read of bank $$01+ data returns garbage), the NMI / WRAM-port
	@# race lint (KNOWN_LIMITATIONS red) and the asset inventory line. Exit 1
	@# fails the link; warnings do not. The knobs keep their names:
	@# SKIP_BANK0_CHECK=1 and SKIP_RAM_CHECK=1 set the ratchet to 0 (a RAM
	@# section past $$2000 still fails: it is always a bug), SKIP_BANKREAD_CHECK,
	@# SKIP_NMI_RACE_CHECK and SKIP_ASSET_BUDGET=1 skip their check. A game
	@# developer's make needs no interpreter from here (docs/tools/opensnes-rom.md).
	@$(OPENSNES)/bin/opensnes-rom check "$(TARGET)" \
		--bank0-fail $(if $(filter 1,$(SKIP_BANK0_CHECK)),0,$(BANK0_FAIL_THRESHOLD)) \
		--ram-fail $(if $(filter 1,$(SKIP_RAM_CHECK)),0,$(RAM_FAIL_THRESHOLD)) --ram-warn $(RAM_WARN_THRESHOLD) \
		$(if $(filter 1,$(SKIP_BANKREAD_CHECK)),--no-bank-reads,) \
		$(if $(filter 1,$(SKIP_NMI_RACE_CHECK)),--no-nmi-race,) \
		$(if $(filter 1,$(SKIP_ASSET_BUDGET)),--no-assets,)

#------------------------------------------------------------------------------
# Project tests — opt-in by presence of test/*.toml (no flag needed): luna's
# own manifests (`luna test`), one per test. `make test` runs them against
# the built ROM with the pinned luna; `make test-update` rewrites their
# visual baselines (asserts.fbhash). Nothing interpreted runs here (the
# two-audiences rule). See docs/GETTING_STARTED.md ("Test your game").
#------------------------------------------------------------------------------

LUNA ?= $(OPENSNES)/testing/bin/luna

test test-update: $(TARGET)
	@if ! ls test/*.toml >/dev/null 2>&1; then \
		echo "No tests: this project declares none."; \
		echo "Create test/<name>.toml — see docs/GETTING_STARTED.md,"; \
		echo "section 'Test your game' (a luna manifest per test)."; \
		exit 1; \
	fi
	@if [ ! -x "$(LUNA)" ]; then \
		echo "luna not found at $(LUNA): run $(OPENSNES)/scripts/install-luna.sh (or set LUNA=/path/to/luna)."; \
		exit 1; \
	fi
	@$(LUNA) test --jobs 0 $(if $(filter test-update,$@),--update) test/

#------------------------------------------------------------------------------
# Cleanup
#------------------------------------------------------------------------------

clean:
	@echo "Cleaning $(TARGET)..."
	@rm -f crt0.o
	@rm -f data_init_start.o data_init_start.wrap.asm
	@rm -f $(ASM_OBJS) $(ASM_OBJS:.o=.wrap.asm)
	@rm -f $(CSRC:.c=.c.asm) $(CSRC:.c=.c.wrap.asm) $(CSRC:.c=.c.o)
	@rm -f data_init_end.o data_init_end.wrap.asm
	@rm -f ram_code_start.o ram_code_start.wrap.asm ram_code_end.o ram_code_end.wrap.asm
	@rm -f project_hdr.asm project_config.inc project_sa1_boot.asm linkfile *.sym $(TARGET) .opensnes_config .opensnes_config.tmp
	@rm -f $(ASSET_STAMPS) $(if $(ASSET_TOML),assets_gen.asm) \
		$(foreach t,$(ASSET_TOML),$(addprefix $(basename $(basename $(t))),.pic .pal .map .pc7 .mp7 .inc _data.as _meta.inc _anim.h .brr .h .b16 .t16 .o16 _entities.inc))
	@rm -f $(GFX_HEADERS)
	@rm -f $(SOUNDBANK_OUT).asm $(SOUNDBANK_OUT).h $(SOUNDBANK_OUT).o $(SOUNDBANK_OUT).wrap.asm $(SOUNDBANK_OUT).bnk
	@rm -f $(GSU_BINS) $(GSU_HEADERS) $(GSUSRC:.sfx=.sfx.o) $(GSUSRC:.sfx=.sfx.link) $(GSUSRC:.sfx=.sfx.sym)
	@rm -f $(SPC_BINS) $(SPCSRC:.spc700.asm=.spc700.o) $(SPCSRC:.spc700.asm=.spc700.link)
