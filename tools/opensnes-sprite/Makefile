# opensnes-sprite — sprite sheets and Aseprite exports → tiles, palette, metasprites, animation clips
# (the 1.x successor of gfx4snes -s/-T and aseprite2snes; their converters, linked as libraries)
TOOL        := opensnes-sprite
VERSION     := 1.0.0
GFX         := ../gfx4snes/src
ASE         := ../aseprite2snes/src
SRCS        := $(wildcard src/*.c) ../common/cli.c \
               $(GFX)/images.c $(GFX)/palettes.c $(GFX)/tiles.c $(GFX)/maps.c $(GFX)/metasprites.c \
               $(GFX)/lz77.c $(GFX)/loadbmp.c $(GFX)/incgener.c ../third_party/lodepng.c \
               $(ASE)/anim.c $(ASE)/json.c
HDRS        := $(wildcard src/*.h) ../common/cli.h $(wildcard $(GFX)/*.h) ../third_party/lodepng.h $(ASE)/anim.h $(ASE)/json.h
INCLUDES    := -Isrc -I../common -I$(GFX) -I../third_party -I$(ASE)
# gfx4snes's inherited code: the same -Wextra classes its own Makefile silences
TOOL_CFLAGS := -Wno-missing-field-initializers -Wno-unused-parameter -Wno-sign-compare
include ../tool.mk
