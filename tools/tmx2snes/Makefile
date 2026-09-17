VERSION    := 1.0.1
DATESTRING := $(shell date +%Y%m%d)

CC     := clang
CFLAGS := -g -O2 -Wall -Wextra -pedantic \
          -D__BUILD_DATE="\"$(DATESTRING)\"" \
          -D__BUILD_VERSION="\"$(VERSION)\""

SOURCES := tmx2snes.c
OBJS    := $(SOURCES:%.c=%.o)
EXE     := tmx2snes

# Static linking (except macOS which doesn't support -static)
UNAME := $(shell uname -s)
ifneq ($(UNAME), Darwin)
	ifneq ($(SANITIZE),1)
	CFLAGS += -static
	endif
endif

all: $(EXE)
	@cp $(EXE) ../../bin/$(EXE)

$(EXE): $(OBJS)
	@echo "Linking $@"
	$(CC) $(CFLAGS) $(OBJS) -o $@ -lm

%.o: %.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $<

install:
	@true

clean:
	rm -f $(EXE) $(OBJS)

.PHONY: all clean install

# SANITIZE=1: ASan + UBSan build for the host-side sanitizer job
# (make test-sanitizers). Not static — the sanitizer runtimes do not link
# statically — and -O1 for readable reports.
ifeq ($(SANITIZE),1)
CFLAGS += -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
endif
