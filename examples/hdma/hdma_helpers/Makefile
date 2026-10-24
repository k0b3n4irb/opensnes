#==============================================================================
# HDMA Helpers Demo
# Demonstrates brightness gradient, color gradient, iris wipe, water ripple
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := hdma_helpers.sfc
ROM_NAME   := HDMA HELPERS DEMO

USE_LIB    := 1
LIB_MODULES := console sprite dma input background hdma

CSRC       := main.c

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (the OpenSNES logo, devtools/art_assets)
#------------------------------------------------------------------------------

clean-hdma-helpers:
	@rm -f res/logo.pic res/logo.pal res/logo.map res/logo.inc res/logo_data.as
