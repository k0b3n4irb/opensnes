# Custom Font Example
# Uses shared OpenSNES templates

OPENSNES := $(shell cd ../../.. && pwd)

# ROM Configuration
TARGET   := custom_font.sfc
ROM_NAME := CUSTOM FONT DEMO

# Font conversion (uses font2snes, not gfx2snes)
FONT2SNES := $(OPENSNES)/bin/font2snes
FONT_PNG  := assets/myfont.png
FONT_H    := myfont.h

# Include shared build rules
include $(OPENSNES)/make/common.mk

# Override: font conversion rule
$(FONT_H): $(FONT_PNG)
	@echo "[FONT] $< -> $@"
	@$(FONT2SNES) -c $< $@

# Add font header as prerequisite for compilation
main.c.asm: $(FONT_H)

# Extend clean to remove font header
clean: clean-font
.PHONY: clean-font
clean-font:
	@rm -f $(FONT_H)
