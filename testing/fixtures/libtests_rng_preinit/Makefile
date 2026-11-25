# rngNext() before any consoleInit()/rngSeed() (2026-11-25): the generator
# starts from a non-zero seed, so it does not return 0 forever.
# Asserted by testing/manifests/libtest_rng_preinit.toml.
OPENSNES := $(shell cd ../../.. && pwd)
TARGET      := libtest_rng_preinit.sfc
ROM_NAME    := LIB TEST RNG PREINIT
CSRC        := main.c
USE_LIB     := 1
LIB_MODULES := console
include $(OPENSNES)/make/common.mk
