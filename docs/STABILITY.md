# What 1.0 freezes {#stability}

This page is the promise behind the version number. It says what a project
built on OpenSNES 1.0 can count on until 2.0, what is not part of that
promise, and how a change reaches you when one is needed.

## The promise

From 1.0 to the next major version:

- **Every public function, type, macro and constant** declared in
  `lib/include/snes.h` and `lib/include/snes/*.h` keeps its name, its
  signature, its meaning and its return values. A function may do more
  (a new bound checked, a new error code documented), never less.
- **Every documented variable** a header declares `extern` keeps its name
  and type (`oamMemory`, `frame_count`, `gsu_cfgr`, …). Variables the
  headers do not declare are internal: a project that declares them
  itself may break at any release.
- **The build knobs** of `make/common.mk` listed on `docs/tools/build.md`
  (every `?=` variable: `USE_*`, `ROM_BANKS`, `LIB_MODULES`, …) keep their
  name and meaning; refusals may be added (a combination that cannot
  work), never silently changed.
- **The assembly interface** documented in `compiler/ABI.md` (argument
  order and slots, return registers, the `tcc__r*` scratch) and the
  linker sections a project may use (`ASSET_SECTION`, `RAM_CODE_SECTION`,
  `GSU_SECTION`) keep their shape.
- **The asset formats** the tools produce (`.pic`, `.pal`, `.map`, `.brr`,
  the soundbank) stay readable by the library of the same major version.

New functions, modules, knobs and sections may appear in any minor
version. A new module costs nothing to a project that does not link it.

## What is not frozen

- **Performance figures** (`docs/PERF.md`, `docs/BENCHMARK.md`) are
  measurements, not contracts; a release may make a call cheaper or, with
  a stated reason, slightly dearer.
- **The emulator pin** (`testing/luna.version`) and the test
  harness under `testing/` are the SDK's own test infrastructure;
  they move with luna.
- **The examples** are teaching material and may be rewritten, merged or
  removed; `docs/HARDWARE_VERIFICATION.md` names the ones a release is
  checked against.
- **The compiler's accepted C** grows; what it refuses today (struct
  parameters and returns by value, variadic functions, inline assembly)
  may be accepted later, never the other way.
- **Internal notes** under `.claude/` are the maintainers' own.

## How a change reaches you

- A name that has to go is **deprecated first**: it keeps working, the
  clang pre-pass of every build prints a warning naming its replacement,
  and `docs/UPGRADING.md` lists it. It is removed at the next major
  version only. A build without clang runs the same scan in Python on each
  source it compiles and prints the names it finds (since 2026-10-04);
  `make check-upgrade SRC=<folder>` reads a whole project against the list.
- A **change of meaning** (the same name, a different effect) happens only
  at a major version, is announced one minor version ahead, and gets an
  entry in `docs/UPGRADING.md` and a check in `check-upgrade`. The 1.0
  release carries one: `hdmaEnable()` / `hdmaDisable()`, mask-taking and
  deprecated in 0.48, take a channel number (and refuse a value above 7).
- A **bug fix that changes behaviour** (the hardware did not do what the
  function promised, or the function did not do what its header said) is
  not a break: the header is the contract, and the fix makes the code
  meet it. `CHANGELOG.md` says what moved and `KNOWN_LIMITATIONS.md` keeps
  the history of the hazard.

## Where the line is drawn

A project that uses only what the headers declare and the pages under
`docs/` describe is inside the promise. A project that reads a RAM
section by address, calls an assembly label the headers do not name, or
depends on a figure in `PERF.md`, is outside it — it may work for years,
and it may break at a minor version without notice.
