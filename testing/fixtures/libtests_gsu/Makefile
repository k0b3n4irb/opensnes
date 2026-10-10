# Super FX library fixture (2026-09-27): the cache-resident job API
# (gsuCacheLoad / gsuStartCached / gsuBusy / gsuWait) and a ROM job waited on
# from the RAM code window (RAM_CODE_SIZE, ram_job.asm), asserted by the luna
# test manifest testing/manifests/libtest_gsu_cached.toml.
OPENSNES := $(shell cd ../../.. && pwd)
TARGET      := libtest_gsu.sfc
ROM_NAME    := LIB RUNTIME TEST GSU
CSRC        := main.c
ASMSRC      := gsu_loader.asm ram_job.asm
GSUSRC      := gsu_job.sfx
# The program is linked at its ROM address: rom_job jumps to an absolute
# label and reads a table of the program (GSU_BANK, make/common.mk).
GSU_BANK    := 1
USE_LIB     := 1
USE_SUPERFX := 1
# A Super FX save: the battery keeps Game Pak RAM (header $15), and the sram
# module reaches it while a job runs (2026-10-03).
USE_SRAM    := 1
LIB_MODULES := console
RAM_CODE_SIZE := 512
include $(OPENSNES)/make/common.mk
