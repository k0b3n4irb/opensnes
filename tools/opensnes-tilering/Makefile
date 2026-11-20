# opensnes-tilering — map pictures with more 8x8 characters than VRAM holds -> the
# deduplicated characters (mirrors merged) and the 16-bit map the tilering module streams.
TOOL        := opensnes-tilering
VERSION     := 1.0.0
SRCS        := $(wildcard src/*.c) ../common/cli.c ../common/incfile.c ../third_party/lodepng.c
HDRS        := $(wildcard src/*.h) ../common/cli.h ../common/incfile.h ../common/xalloc.h ../third_party/lodepng.h
INCLUDES    := -Isrc -I../common -I../third_party
include ../tool.mk
