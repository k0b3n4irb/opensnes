# opensnes-image — full-screen pictures (HiColor) and the Mode 7 perspective
# tables; img2snes's quantizer and font2snes's tile packer are linked here.
TOOL        := opensnes-image
VERSION     := 1.0.0
IMG         := ../img2snes/src
FNT         := ../font2snes/src
SRCS        := $(wildcard src/*.c) ../common/cli.c ../common/incfile.c $(IMG)/quantize.c $(FNT)/tiles.c ../third_party/lodepng.c
HDRS        := $(wildcard src/*.h) ../common/cli.h ../common/incfile.h $(IMG)/quantize.h $(FNT)/tiles.h ../third_party/lodepng.h
INCLUDES    := -Isrc -I../common -I$(IMG) -I$(FNT) -I../third_party
LDLIBS      := -lm
include ../tool.mk
