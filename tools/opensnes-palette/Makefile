# opensnes-palette — a project's palettes: the CGRAM plan (the 1.x successor
# of palplan) and the quantization of RGB art (img2snes's quantizer, linked here).
TOOL        := opensnes-palette
VERSION     := 1.0.0
IMG         := ../img2snes/src
SRCS        := $(wildcard src/*.c) ../common/cli.c ../common/incfile.c $(IMG)/quantize.c $(IMG)/scale.c ../third_party/lodepng.c
HDRS        := $(wildcard src/*.h) ../common/cli.h ../common/incfile.h $(IMG)/quantize.h $(IMG)/scale.h ../third_party/lodepng.h
INCLUDES    := -Isrc -I../common -I$(IMG) -I../third_party
LDLIBS      := -lm
include ../tool.mk
