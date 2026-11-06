# The region fixture, NTSC build (the default ROM_REGION). Its twin
# libtests_region_pal builds the same main.c with ROM_REGION=pal.
# Asserted by testing/manifests/libtest_region_*.toml.
OPENSNES := $(shell cd ../../.. && pwd)
TARGET      := libtest_region.sfc
ROM_NAME    := LIB TEST REGION
CSRC        := main.c
USE_LIB     := 1
LIB_MODULES := console
include $(OPENSNES)/make/common.mk
