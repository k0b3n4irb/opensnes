# A 4 MB LoROM image: 128 banks of 32 KB (ROM_BANKS_MAX for plain LoROM).
# The linker places C const data in the highest bank first, so the table of
# main.c lands in bank 127, which is addressed $FF:8000. Asserted by
# testing/manifests/libtest_bigrom.toml.
OPENSNES := $(shell cd ../../.. && pwd)
TARGET      := libtest_bigrom.sfc
ROM_NAME    := LIB TEST BIG ROM
ROM_BANKS   := 128
CSRC        := main.c
USE_LIB     := 1
LIB_MODULES := console
include $(OPENSNES)/make/common.mk
