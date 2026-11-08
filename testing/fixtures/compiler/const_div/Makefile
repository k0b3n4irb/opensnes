OPENSNES := $(shell cd ../../../.. && pwd)
TARGET   := const_div.sfc
ROM_NAME := CONST DIV TEST
USE_LIB  := 1
LIB_MODULES := console
CSRC   := main.c
include $(OPENSNES)/make/common.mk
# edges() divides by a constant zero on purpose: the folder must leave it to
# run time (main.c)
CLANG_LINT_FLAGS += -Wno-division-by-zero
