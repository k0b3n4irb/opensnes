# img2snes — RGB/RGBA PNG → indexed PNG (quantize, BGR555, scale)
TOOL     := img2snes
VERSION  := 1.0.0
SRCS     := $(wildcard src/*.c) ../third_party/lodepng.c ../third_party/cmdparser.c
HDRS     := $(wildcard src/*.h) ../third_party/lodepng.h ../third_party/cmdparser.h
INCLUDES := -Isrc -I../third_party
# inherited option tables and callbacks: not worth a rewrite for two -Wextra classes
TOOL_CFLAGS := -Wno-missing-field-initializers -Wno-unused-parameter
LDLIBS   := -lm
include ../tool.mk
