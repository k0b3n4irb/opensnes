# opensnes-tileset — background images → tileset, tilemap and palette (the 1.x successor of gfx4snes -m)
TOOL        := opensnes-tileset
VERSION     := 1.0.0
GFX         := ../gfx4snes/src
SRCS        := $(wildcard src/*.c) ../common/cli.c ../common/incfile.c \
               $(GFX)/images.c $(GFX)/palettes.c $(GFX)/tiles.c $(GFX)/maps.c $(GFX)/metasprites.c \
               $(GFX)/lz77.c $(GFX)/loadbmp.c ../third_party/lodepng.c
HDRS        := $(wildcard src/*.h) ../common/cli.h ../common/incfile.h $(wildcard $(GFX)/*.h) ../third_party/lodepng.h
INCLUDES    := -Isrc -I../common -I$(GFX) -I../third_party
TOOL_CFLAGS := -Wno-missing-field-initializers -Wno-unused-parameter -Wno-sign-compare
include ../tool.mk
