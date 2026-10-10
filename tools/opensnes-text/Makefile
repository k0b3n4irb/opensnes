# opensnes-text — bitmap fonts for the text module (the 1.x successor of
# font2snes; its tile packer, tools/font2snes/src/tiles.c, is linked here).
TOOL        := opensnes-text
VERSION     := 1.0.0
FNT         := ../font2snes/src
SRCS        := $(wildcard src/*.c) ../common/cli.c ../common/incfile.c $(FNT)/tiles.c ../third_party/lodepng.c
HDRS        := $(wildcard src/*.h) ../common/cli.h ../common/incfile.h $(FNT)/tiles.h ../third_party/lodepng.h
INCLUDES    := -Isrc -I../common -I$(FNT) -I../third_party
include ../tool.mk
