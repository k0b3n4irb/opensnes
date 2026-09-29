# Super FX library fixture (2026-09-27): the cache-resident job API
# (gsuCacheLoad / gsuStartCached / gsuBusy / gsuWait) and a ROM job waited on
# from the RAM code window (RAM_CODE_SIZE, ram_job.asm), asserted by the luna
# test manifest tools/luna-test/manifests/libtest_gsu_cached.toml.
OPENSNES := $(shell cd ../.. && pwd)
TARGET      := libtest_gsu.sfc
ROM_NAME    := LIB RUNTIME TEST GSU
CSRC        := main.c
ASMSRC      := gsu_loader.asm ram_job.asm
GSUSRC      := gsu_job.sfx
USE_LIB     := 1
USE_SUPERFX := 1
LIB_MODULES := console
RAM_CODE_SIZE := 256
include $(OPENSNES)/make/common.mk
