# Fuzzing the asset parsers with libFuzzer (gaps review H5, 2026-09-14).
#
#   make -C tools/fuzz            build both harnesses (clang, ASan + UBSan + fuzzer)
#   make -C tools/fuzz run        fuzz each target for FUZZ_SECONDS (default 60) from the
#                                 seed fixtures; a crash leaves crash-* in the corpus dir
#                                 and fails the target
#   make -C tools/fuzz replay     run every committed input under crashes/<target>/ once —
#                                 the regression corpus, cheap enough for every push
#
# Targets: lodepng (the PNG decoder gfx4snes and img2snes vendor) and
# itloader (smconv's Impulse Tracker loader, which trusts its header).
# Seeds are the goldens' fixtures. The evolving corpus (corpus/) is local
# and ignored; only crashes/ is committed, one file per fixed bug.

CLANG        ?= clang
FUZZ_SECONDS ?= 60
FUZZ_CFLAGS  := -g -O1 -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined
COMMON       := ../common
SMC          := ../smconv/src

TARGETS := fuzz_lodepng fuzz_itloader

all: $(TARGETS)

fuzz_lodepng: fuzz_lodepng.c $(COMMON)/lodepng.c
	$(CLANG) $(FUZZ_CFLAGS) -I$(COMMON) -DLODEPNG_NO_COMPILE_ENCODER -DLODEPNG_NO_COMPILE_DISK $^ -o $@

fuzz_itloader: fuzz_itloader.c $(SMC)/itloader.c $(SMC)/io.c
	$(CLANG) $(FUZZ_CFLAGS) -I$(SMC) $^ -o $@

# Seed corpora: the golden fixtures (real files the tools accept).
corpus/lodepng:
	@mkdir -p $@ && cp ../gfx4snes/tests/fixtures/*.png ../img2snes/tests/fixtures/*.png $@/
corpus/itloader:
	@mkdir -p $@ && cp ../smconv/tests/fixtures/*.it $@/

run: all corpus/lodepng corpus/itloader
	@for t in lodepng itloader; do \
		echo "== fuzz $$t for $(FUZZ_SECONDS)s"; \
		ASAN_OPTIONS=detect_leaks=0 ./fuzz_$$t -max_total_time=$(FUZZ_SECONDS) -max_len=65536 \
			-artifact_prefix=corpus/$$t/crash- -print_final_stats=1 corpus/$$t > corpus/$$t.log 2>&1; \
		st=$$?; grep -E "stat::number_of_executed_units|SUMMARY" corpus/$$t.log; \
		if [ $$st -ne 0 ]; then echo "FUZZ FAIL: $$t (exit $$st) — corpus/$$t.log, corpus/$$t/crash-*"; \
			grep -E -A12 "SUMMARY|ERROR" corpus/$$t.log | head -40; exit 1; fi; \
	done
	@echo "fuzz: OK (no crash in $(FUZZ_SECONDS)s per target)"

replay: all
	@fail=0; for t in lodepng itloader; do \
		n=$$(ls crashes/$$t 2>/dev/null | wc -l); \
		if [ $$n -gt 0 ]; then \
			ASAN_OPTIONS=detect_leaks=0 ./fuzz_$$t -runs=0 crashes/$$t >/dev/null 2>&1 \
				|| { echo "REPLAY FAIL: $$t"; fail=1; }; \
		fi; echo "replay $$t: $$n regression input(s)"; \
	done; [ $$fail -eq 0 ] && echo "fuzz-replay: OK"

clean:
	rm -f $(TARGETS)
	rm -rf corpus

.PHONY: all run replay clean
