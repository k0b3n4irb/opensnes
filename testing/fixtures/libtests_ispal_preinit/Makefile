# isPAL() before any consoleInit() (2026-11-25): it reads $213F itself on its
# first call. Asserted by testing/manifests/libtest_ispal_preinit.toml.
OPENSNES := $(shell cd ../../.. && pwd)
TARGET      := libtest_ispal_preinit.sfc
ROM_NAME    := LIB TEST ISPAL PRE
CSRC        := main.c
USE_LIB     := 1
LIB_MODULES := console
include $(OPENSNES)/make/common.mk
