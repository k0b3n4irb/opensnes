# The region fixture, PAL build: the same main.c as libtests_region, header
# country byte $02. Asserted by testing/manifests/libtest_region_pal.toml.
OPENSNES := $(shell cd ../../.. && pwd)
TARGET      := libtest_region_pal.sfc
ROM_NAME    := LIB TEST REGION PAL
ROM_REGION  := pal
CSRC        := main.c
USE_LIB     := 1
LIB_MODULES := console
include $(OPENSNES)/make/common.mk
