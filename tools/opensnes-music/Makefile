# opensnes-music — Impulse Tracker → SNESMOD soundbank (the 1.x successor of smconv; same converter)
TOOL        := opensnes-music
VERSION     := 1.0.0
SMC         := ../lib/smconv/src
SRCS        := $(wildcard src/*.c) ../common/cli.c $(SMC)/brr.c $(SMC)/io.c $(SMC)/itloader.c $(SMC)/it2spc.c $(SMC)/report.c
# the SNESMOD driver the .spc embeds is the library's, byte for byte: its
# one copy is lib/source/sm_spc.asm (`.byte` lines), turned into a C array here
SM_SPC      := ../../lib/source/sm_spc.asm
GEN         := build/gen
HDRS        := $(wildcard src/*.h) ../common/cli.h $(wildcard $(SMC)/*.h) $(GEN)/spc_program.h
INCLUDES    := -Isrc -I../common -I$(GEN) -I$(SMC)
TOOL_CFLAGS := -std=c11
LDLIBS      := -lm
include ../tool.mk

$(GEN)/spc_program.h: $(SM_SPC)
	@mkdir -p $(GEN)
	@{ echo '/* generated from lib/source/sm_spc.asm by tools/opensnes-music/Makefile: do not edit */'; \
	   echo 'static const unsigned char spc_program[] = {'; \
	   sed -n 's/^[[:space:]]*\.byte[[:space:]]*//p' $< | sed 's/\$$/0x/g; s/[[:space:]]*$$/,/'; \
	   echo '};'; } > $@.tmp && mv $@.tmp $@
