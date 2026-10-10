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
# Graphics conversion (reuse opensnes logo)
#------------------------------------------------------------------------------

clean-hdma-helpers:
	@rm -f res/bg.pic res/bg.pal res/bg.map res/bg.inc res/bg_data.as
