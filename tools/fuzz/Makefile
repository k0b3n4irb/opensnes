# Fuzzing the asset parsers with libFuzzer (gaps review H5, 2026-09-14).
#
#   make -C tools/fuzz            build both harnesses (clang, ASan + UBSan + fuzzer)
#   make -C tools/fuzz run        fuzz each target for FUZZ_SECONDS (default 60) from the
#                                 seed fixtures; a crash leaves crash-* in the corpus dir
#                                 and fails the target
#   make -C tools/fuzz replay     run every committed input under crashes/<target>/ once —
#                                 the regression corpus, cheap enough for every push
#
# Targets: lodepng (the PNG decoder gfx4snes and img2snes share), itloader
# (smconv's Impulse Tracker loader, which trusts its header), tiled
# (tmx2snes's cute_tiled .tmj loader), json (aseprite2snes's JSON parser)
# and stbimage (font2snes's stb_image PNG decoder) — every parser an asset
# tool feeds with a file (H5 complete, 2026-09-15) — wav (opensnes-sample's
# WAV parser, 2026-10-05) and rearrange (the palette rearrangement of
# opensnes-tileset --rearrange, with its colour and transparency invariants,
# 2026-10-16). Seeds are the goldens'
# fixtures. The evolving corpus (corpus/) is local and ignored; only
# crashes/ is committed, one file per fixed bug.

CLANG        ?= clang
FUZZ_SECONDS ?= 60
FUZZ_CFLAGS  := -g -O1 -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined
COMMON       := ../third_party
SMC          := ../lib/smconv/src
TMX          := ../third_party
ASE          := ../lib/aseprite2snes/src
FNT          := ../third_party

FUZZERS := lodepng itloader tiled json stbimage wav rearrange
TARGETS := $(addprefix fuzz_,$(FUZZERS))

all: $(TARGETS)

fuzz_lodepng: fuzz_lodepng.c $(COMMON)/lodepng.c
	$(CLANG) $(FUZZ_CFLAGS) -I$(COMMON) -DLODEPNG_NO_COMPILE_ENCODER -DLODEPNG_NO_COMPILE_DISK $^ -o $@

fuzz_itloader: fuzz_itloader.c $(SMC)/itloader.c $(SMC)/io.c $(SMC)/report.c
	$(CLANG) $(FUZZ_CFLAGS) -I$(SMC) $^ -o $@

fuzz_tiled: fuzz_tiled.c $(TMX)/cute_tiled.h
	$(CLANG) $(FUZZ_CFLAGS) -I$(TMX) $< -o $@

fuzz_json: fuzz_json.c $(ASE)/json.c
	$(CLANG) $(FUZZ_CFLAGS) -I$(ASE) $^ -o $@

fuzz_stbimage: fuzz_stbimage.c $(FNT)/stb_image.h
	$(CLANG) $(FUZZ_CFLAGS) -I$(FNT) $< -o $@

fuzz_rearrange: fuzz_rearrange.c ../lib/gfx4snes/src/palettes.c
	$(CLANG) $(FUZZ_CFLAGS) -I../lib/gfx4snes/src -I$(COMMON) $^ -o $@

fuzz_wav: fuzz_wav.c ../opensnes-sample/src/wav.c
	$(CLANG) $(FUZZ_CFLAGS) -I../opensnes-sample/src -I$(SMC) $^ -o $@

# Seed corpora: the golden fixtures (real files the tools accept).
corpus/lodepng:
	@mkdir -p $@ && cp ../lib/gfx4snes/tests/fixtures/*.png ../lib/img2snes/tests/fixtures/*.png $@/
corpus/itloader:
	@mkdir -p $@ && cp ../lib/smconv/tests/fixtures/*.it $@/
corpus/tiled:
	@mkdir -p $@ && cp ../lib/tmx2snes/tests/fixtures/*.tmj $@/
corpus/json:
	@mkdir -p $@ && cp ../lib/aseprite2snes/tests/fixtures/*.json $@/
corpus/wav:
	@mkdir -p $@ && cp ../opensnes-sample/tests/fixtures/*.wav $@/
corpus/rearrange:
	@mkdir -p $@ && cp crashes/rearrange/* $@/
corpus/stbimage:
	@mkdir -p $@ && cp ../lib/font2snes/tests/fixtures/*.png ../lib/gfx4snes/tests/fixtures/*.png $@/

# -close_fd_mask=3 closes the target's stdout and stderr: the parsers print a
# warning per rejected input (smconv's IT loader wrote a million lines, 91 MB,
# in one 600 s run). libFuzzer's own lines and the ASan / UBSan reports go to
# a copy of stderr and still reach the log (checked on a crashing harness,
# 2026-10-25). After a clean run, -merge=1 keeps only the inputs that add
# coverage, so the corpus does not grow run after run.
run: all $(addprefix corpus/,$(FUZZERS))
	@for t in $(FUZZERS); do \
		echo "== fuzz $$t for $(FUZZ_SECONDS)s"; \
		ASAN_OPTIONS=detect_leaks=0 ./fuzz_$$t -max_total_time=$(FUZZ_SECONDS) -max_len=65536 -close_fd_mask=3 \
			-artifact_prefix=corpus/$$t/crash- -print_final_stats=1 corpus/$$t > corpus/$$t.log 2>&1; \
		st=$$?; grep -E "stat::number_of_executed_units|SUMMARY" corpus/$$t.log; \
		if [ $$st -ne 0 ]; then echo "FUZZ FAIL: $$t (exit $$st) — corpus/$$t.log, corpus/$$t/crash-*"; \
			grep -E -A12 "SUMMARY|ERROR" corpus/$$t.log | head -40; exit 1; fi; \
		rm -rf corpus/$$t.min && mkdir corpus/$$t.min && \
		ASAN_OPTIONS=detect_leaks=0 ./fuzz_$$t -merge=1 -close_fd_mask=3 -max_len=65536 corpus/$$t.min corpus/$$t \
			> corpus/$$t.merge.log 2>&1 && rm -rf corpus/$$t && mv corpus/$$t.min corpus/$$t \
			&& rm -f corpus/$$t.merge.log && echo "   corpus kept: $$(ls corpus/$$t | wc -l) input(s)"; \
	done
	@echo "fuzz: OK (no crash in $(FUZZ_SECONDS)s per target)"

replay: all
	@mkdir -p corpus
	@fail=0; for t in $(FUZZERS); do \
		n=$$(ls crashes/$$t 2>/dev/null | wc -l); \
		if [ $$n -gt 0 ]; then \
			ASAN_OPTIONS=detect_leaks=0 ./fuzz_$$t -runs=0 crashes/$$t > corpus/replay_$$t.log 2>&1 \
				|| { echo "REPLAY FAIL: $$t"; \
				     grep -E "runtime error|ERROR:|SUMMARY|READ of|WRITE of" corpus/replay_$$t.log | head -6; \
				     fail=1; }; \
		fi; echo "replay $$t: $$n regression input(s)"; \
	done; [ $$fail -eq 0 ] && echo "fuzz-replay: OK"

clean:
	rm -f $(TARGETS)
	rm -rf corpus

.PHONY: all run replay clean
