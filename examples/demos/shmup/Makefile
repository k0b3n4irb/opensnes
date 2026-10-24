#==============================================================================
# shmup — a vertical shoot 'em up
#
# Kit's delivery glider over the city at night, against the security's
# hunters. The city scrolls down on BG1, the glider, the hunters and Kit's
# shots are sprites; every picture is converted by the build from its
# settings file (the pictures come from devtools/art_assets/build.py).
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := shmup.sfc
ROM_NAME   := SHMUP

USE_LIB    := 1
LIB_MODULES := console dma background asset sprite input

CSRC       := main.c
include $(OPENSNES)/make/common.mk
