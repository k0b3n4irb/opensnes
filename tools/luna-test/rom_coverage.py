#!/usr/bin/env python3
"""Measured ROM coverage of the public lib API — which functions the corpus executes.

Thin orchestrator over `luna profile --pc-set` (luna v1.21.0): for every
example ROM luna writes the set of distinct 24-bit PCs that executed up to
the example's first manifest capture frame; this script folds each PC onto
the WLA-DX `.sym` label that contains it (FastROM / HiROM mirrors folded to
the linker bank, as symmap.py does) and unions the hit labels over the
corpus. The public function names come from `lib/include/snes/*.h`.
luna measures; this file only folds and diffs (luna_tooling.md).

What it answers: "which public lib functions does NOTHING ever run?" —
the measured version of the gaps review's static "96 declarations called
by zero examples" (2026-09-11). Each example is profiled twice over: the
input-free boot/idle path to its first capture frame (the visual pillar),
and once per `luna test` manifest that names its ROM, replaying the
manifest's joypad-1 script to its last checkpoint (since 2026-09-19 —
before that, input-driven code was under-counted: 167 "never" of which 53
were only ever reached by a button). The library fixture
(`devtools/libtests/libtest.sfc`) counts too: a function it asserts on is
tested, and a ratchet that said otherwise was asking for an example nobody
needs. Since luna v1.26.0 `luna profile` takes the peripherals too, so a
manifest's mouse (port 1), Super Scope (port 2) and joypad-2 scripts are
replayed on its leg — the under-count that remained until 2026-09-26.

Every leg also runs the **stack-floor gate** (`--stack-floor`, luna
v1.26.0): the stack starts at $1FFF and grows down toward the C variables
of the plain RAM band, whose top (`used_top`) comes from the ROM's `.sym`.
A leg whose deepest stack pointer went below `used_top` has written over a
global — the silent corruption the link-time `RAM_FAIL_THRESHOLD` (a
guess) could only approximate. The margin is measured on every leg under
real input, and the smallest one is reported. It runs in the same
`profile` invocation as `--pc-set` because no `--budget` is passed there:
exit 1 can only mean the stack gate (luna's 2026-09-22 note).

Usage
-----
    python3 tools/luna-test/rom_coverage.py            # report + check vs the committed list
    python3 tools/luna-test/rom_coverage.py --update   # rewrite baselines/never_executed.txt
    python3 tools/luna-test/rom_coverage.py --only games/

The committed list is a ratchet: a public function that is not executed
by any example and is NOT already in the list fails the check (a new
function shipped without an example or a libtest); a listed function that
became executed is reported so the list can shrink (`--update`).

Exit 0 = no new never-executed function, 1 = the ratchet grew, 2 = usage.
"""
from __future__ import annotations

import argparse
import bisect
import re
import struct
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent / "devtools" / "symmap"))
from luna_runner import (  # noqa: E402
    HERE, REPO_ROOT, LUNA_VERSION, capture_frames, discover_example_roms, example_key,
    find_luna, firmware_dir, load_manifest, missing_firmware,
)
from symmap import SymbolTable, rom_bank  # noqa: E402  (mirror folding, one source of truth)

HEADERS = REPO_ROOT / "lib" / "include" / "snes"
RATCHET = HERE / "baselines" / "never_executed.txt"
# Functions executed ONLY by firmware-gated examples (dsp1b.rom): CI has no
# firmware, skips those examples, and must not count these as newly never-
# executed. Written by --update on a machine that has the firmware.
FIRMWARE_ONLY = HERE / "baselines" / "executed_only_with_firmware.txt"
REPORT = HERE / "ROM_COVERAGE.md"
MANIFESTS = HERE / "manifests"
# Fixture ROMs profiled on top of the examples. The library fixture runs its
# whole assertion path in ~90 frames (test_libtest.py's 3 M instructions end
# at frame 90, the rest is WAI); 120 covers it with margin.
# The compiler's runtime ROMs count too: debug_channel is the only executor
# of the WDM / nocash channel (consoleNocashMessage, consoleMesenBreakpoint)
# and asserts on it; the others run in ~1 M instructions (their tests' STEPS).
DSP1_FIXTURE = (REPO_ROOT / "devtools" / "libtests_dsp1" / "libtest_dsp1.sfc", "libtest_dsp1", 60)
_RT = REPO_ROOT / "devtools" / "compiler-tests" / "runtime"
FIXTURES = [(REPO_ROOT / "devtools" / "libtests" / "libtest.sfc", "libtest", 120),
            # the second fixture reaches r_done around frame 170 (SNESMOD upload first)
            (REPO_ROOT / "devtools" / "libtests_fx" / "libtest_fx.sfc", "libtest_fx", 240),
            (REPO_ROOT / "devtools" / "libtests_hirom" / "libtest_hirom.sfc", "libtest_hirom", 60)] + [
    (_RT / name / f"{name}.sfc", f"runtime/{name}", 70)
    for name in ("a6_farptr", "a7_32bit", "b2_far_ram", "c_features", "debug_channel")
]


# Manifest keys replayed on a profile leg, with the port each device sits on
# in `luna test` (measured 2026-09-26: the mouse example reads port 1, the
# Super Scope example port 2) and the entry separator of its grammar.
PERIPHERALS = [("input2", ["--input2"], ","),
               ("mouse", ["--port1", "mouse", "--mouse"], ";"),
               ("superscope", ["--port2", "superscope", "--superscope"], ";")]


def _merge(scripts: list[str], sep: str) -> str | None:
    events = []
    for sc in scripts:
        for ev in filter(None, (e.strip() for e in sc.split(sep))):
            frame, rest = ev.split(":", 1)
            events.append((int(frame), rest))
    events.sort(key=lambda e: e[0])
    return sep.join(f"{f}:{r}" for f, r in events) or None


def manifest_runs(rom: Path) -> list[tuple[str, list[str], str | None]]:
    """(manifest name, run flags, merged joypad-1 script or None) for every
    `luna test` manifest whose `rom` is this ROM. Checkpoint scripts merge into
    one timeline exactly as `luna test` merges them (README: "input scripts from
    all checkpoints are merged"). The run flags carry the bound and, since
    2026-09-26, the joypad-2 / mouse / Super Scope scripts."""
    import tomllib
    runs = []
    for toml in sorted(MANIFESTS.glob("*.toml")):
        try:
            m = tomllib.loads(toml.read_text())
        except tomllib.TOMLDecodeError:
            continue
        target = (toml.parent / m.get("rom", "")).resolve()
        if target != rom.resolve():
            continue
        cps = m.get("checkpoint", [])
        # The run bound, in `luna test` precedence: frames, else the last
        # checkpoint, else an instruction count (`steps`, the R7 DMA-safety
        # manifests). Encoded as the luna profile flag it maps to.
        until = m.get("frames") or max((c.get("at_frame", 0) for c in cps), default=0)
        if until:
            bound = ["--until-frame", str(until)]
        elif m.get("steps"):
            bound = ["-n", str(m["steps"])]
        else:
            continue
        script = _merge([m.get("input", "")] + [c.get("input", "") for c in cps], ",")
        for key, flags, sep in PERIPHERALS:
            merged = _merge([m.get(key, "")] + [c.get(key, "") for c in cps], sep)
            if merged:
                bound = bound + flags + [merged]
        runs.append((toml.stem, bound, script))
    return runs


def ram_band_top(sym: Path) -> int | None:
    """Top of the plain C RAM band ($00:0000-$1FFF) in this ROM: the first byte
    above the highest bank-$00 RAMSECTION — what symmap --check-ram-budget
    reports as `top at $XXXX`. The stack must never reach below it."""
    table = SymbolTable()
    table.parse(sym)
    band = [s for s in table.ramsections if s.bank == 0x00 and s.address < 0x2000]
    return max(s.address + s.size for s in band) if band else None


_STACK_RE = re.compile(r"^stack: deepest S \$([0-9A-Fa-f]+)(.*)$", re.MULTILINE)


def profile_pcs(luna: str, rom: Path, sym: Path, bound: list[str], script: str | None,
                pcs: Path, floor: int | None) -> tuple[str | None, int | None, str]:
    """One `luna profile --pc-set [--stack-floor]` run.

    Returns (error or None, deepest stack pointer or None, luna's stack line).
    Exit 1 with a `stack:` line ending in UNDER is the stack gate, not an error:
    no --budget is passed here, so it is the only gate that can fail."""
    cmd = [luna, "profile", str(rom), *bound, "--sym", str(sym),
           "--pc-set", str(pcs), "--out", "/dev/null", "--top", "0"]
    if script:
        cmd += ["--input", script]
    if floor is not None:
        cmd += ["--stack-floor", f"0x{floor:04X}"]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    out = proc.stdout + proc.stderr
    m = _STACK_RE.search(out)
    deepest = int(m.group(1), 16) if m else None
    line = m.group(0).strip() if m else ""
    gate_failed = proc.returncode == 1 and "UNDER" in line
    if (proc.returncode != 0 and not gate_failed) or not pcs.is_file():
        return proc.stderr.strip()[:200], deepest, line
    return None, deepest, line


def public_functions() -> dict[str, str]:
    """name -> header, for every function declared in lib/include/snes/*.h."""
    out: dict[str, str] = {}
    for h in sorted(HEADERS.glob("*.h")):
        s = h.read_text()
        s = re.sub(r"/\*.*?\*/", "", s, flags=re.S)
        s = re.sub(r"//.*", "", s)
        for m in re.finditer(r"^\s*(?:extern\s+)?[A-Za-z_][\w\s\*]*?\b([a-zA-Z_]\w*)\s*\([^;{]*\)\s*;",
                             s, flags=re.M):
            name = m.group(1)
            if name in ("if", "while", "for", "switch", "return", "sizeof", "void",
                        "int", "char", "unsigned", "signed", "static", "inline"):
                continue
            out.setdefault(name, h.name)
    return out


def load_labels(sym: Path) -> dict[int, tuple[list[int], list[str]]]:
    """bank -> (sorted addresses, names) from the .sym [labels] section."""
    per: dict[int, list[tuple[int, str]]] = {}
    in_labels = False
    for line in sym.read_text(errors="replace").splitlines():
        if line.startswith("["):
            in_labels = line.strip() == "[labels]"
            continue
        if not in_labels:
            continue
        m = re.match(r"^([0-9a-f]{2}):([0-9a-f]{4}) (\S+)$", line.strip(), flags=re.I)
        if not m:
            continue
        per.setdefault(rom_bank(int(m.group(1), 16)), []).append((int(m.group(2), 16), m.group(3)))
    folded: dict[int, tuple[list[int], list[str]]] = {}
    for bank, items in per.items():
        items.sort()
        folded[bank] = ([a for a, _ in items], [n for _, n in items])
    return folded


def executed_labels(pc_set: Path, labels: dict) -> set[str]:
    data = pc_set.read_bytes()
    hit: set[str] = set()
    for (pc,) in struct.iter_unpack("<I", data):
        bank, addr = rom_bank((pc >> 16) & 0xFF), pc & 0xFFFF
        table = labels.get(bank)
        if not table:
            continue
        addrs, names = table
        i = bisect.bisect_right(addrs, addr) - 1
        # Several labels can share an address (a data label ending right
        # where a function starts, e.g. `brr_pop_end` at `audioInit`), and a
        # PC inside a QBE block label `fn@start.12` belongs to `fn`: credit
        # every label at that address and the `@`-stripped base name.
        j = i
        while j >= 0 and addrs[j] == addrs[i]:
            hit.add(names[j]); hit.add(names[j].split("@", 1)[0]); j -= 1
    return hit


def main() -> int:
    ap = argparse.ArgumentParser(description="measured ROM coverage of the public lib API (luna --pc-set)")
    ap.add_argument("--update", action="store_true", help="rewrite the never-executed ratchet list")
    ap.add_argument("--only", metavar="SUBSTR", help="restrict to example keys containing SUBSTR (no ratchet check)")
    args = ap.parse_args()

    luna = find_luna()
    manifest = load_manifest()
    public = public_functions()
    hits: dict[str, set[str]] = {}          # function -> examples that executed it
    tmp = Path("/tmp/luna-pcset"); tmp.mkdir(parents=True, exist_ok=True)
    roms = 0
    legs = 0
    stack_under: list[str] = []                 # legs whose stack crossed the band top
    stack_margins: list[tuple[int, str]] = []    # (bytes between deepest S and band top, leg)
    skipped_fw: list[str] = []
    gated = {k for k, v in manifest.get("examples", {}).items() if v.get("firmware")}
    targets: list[tuple[Path, str, int]] = []
    for rom in discover_example_roms():
        key = example_key(rom)
        if missing_firmware(key, manifest):
            skipped_fw.append(key)
            continue
        targets.append((rom, key, capture_frames(key, manifest)[0]))
    # The DSP-1 fixture is firmware-gated like the DSP-1 examples: without
    # dsp1b.rom it is skipped and what only it executes is exempt (CI).
    gated.add(DSP1_FIXTURE[1])
    if (firmware_dir() / "dsp1b.rom").is_file():
        if DSP1_FIXTURE[0].is_file():
            targets.append(DSP1_FIXTURE)
    else:
        skipped_fw.append(DSP1_FIXTURE[1])
    targets += [(rom, key, frames) for rom, key, frames in FIXTURES if rom.is_file()]
    if not all(rom.is_file() for rom, _, _ in FIXTURES):
        missing = [key for rom, key, _ in FIXTURES if not rom.is_file()]
        print(f"  note: fixture ROM(s) not built, skipped: {', '.join(missing)}", file=sys.stderr)
    for rom, key, frame in targets:
        if args.only and args.only not in key:
            continue
        sym = rom.with_suffix(".sym")
        if not sym.is_file():
            print(f"  SKIP  {key}: no .sym", file=sys.stderr)
            continue
        labels = load_labels(sym)
        floor = ram_band_top(sym)
        runs = [("idle", ["--until-frame", str(frame)], None)] + manifest_runs(rom)
        for name, bound, script in runs:
            pcs = tmp / (key.replace("/", "_") + "." + name + ".bin")
            err, deepest, stack_line = profile_pcs(luna, rom, sym, bound, script, pcs, floor)
            if err is not None:
                print(f"  ERROR {key} [{name}]: {err}", file=sys.stderr)
                return 2
            legs += 1
            if floor is not None and deepest is not None:
                # S points at the next free byte: the deepest byte written is S+1.
                stack_margins.append((deepest + 1 - floor, f"{key} [{name}]"))
                if "UNDER" in stack_line:
                    stack_under.append(f"{key} [{name}]: {stack_line} (band top ${floor:04X})")
            for fn in executed_labels(pcs, labels) & public.keys():
                hits.setdefault(fn, set()).add(key)
        roms += 1

    never = sorted(n for n in public if n not in hits)
    by_header: dict[str, list[str]] = {}
    for n in never:
        by_header.setdefault(public[n], []).append(n)

    lines = [
        "# Measured ROM coverage of the public lib API",
        "",
        f"luna {LUNA_VERSION} · `luna profile --pc-set` per ROM: the input-free idle path to the "
        f"first capture frame, plus every `luna test` manifest's joypad-1 script to its last "
        f"checkpoint · {roms} ROMs (examples + the library fixture), {legs} legs · "
        f"**{len(public) - len(never)} of {len(public)} public functions executed, {len(never)} never**",
        "",
        "> Executed = at least one PC inside the function's `.sym` label range on at least one "
        "leg. Joypad-2, mouse (port 1) and Super Scope (port 2) scripts are replayed like "
        "joypad 1. The never-executed list is the ratchet in `baselines/never_executed.txt`.",
        "",
        "| header | never executed |",
        "|---|---|",
    ]
    lines += [f"| `{h}` | {', '.join(f'`{n}`' for n in ns)} |" for h, ns in sorted(by_header.items())]
    lines += ["", "## Least-covered executed functions (one ROM only)", "",
              "| function | the one ROM |", "|---|---|"]
    lines += [f"| `{n}` | `{next(iter(ex))}` |" for n, ex in sorted(hits.items()) if len(ex) == 1]
    if args.update:
        # The committed report is the full-coverage capture (firmware present);
        # a check run (CI skips firmware-gated examples) leaves it alone.
        REPORT.write_text("\n".join(lines) + "\n")
    print(f"ROM coverage: {len(public) - len(never)}/{len(public)} public functions executed "
          f"by {roms} ROMs; {len(never)} never.")
    if stack_margins:
        stack_margins.sort()
        low = ", ".join(f"{leg} {m} B" for m, leg in stack_margins[:3])
        print(f"Stack floor: {len(stack_margins)} legs measured; smallest margins "
              f"between the deepest stack byte and the C variables: {low}")
    if stack_under:
        print("ERROR: the stack reached into the C variables of the plain RAM band "
              "(silent corruption):", file=sys.stderr)
        for s in stack_under:
            print(f"  {s}", file=sys.stderr)
        return 1

    if args.only:
        return 0
    if args.update:
        if skipped_fw:
            print(f"ERROR: --update needs the coprocessor firmware installed (skipped: "
                  f"{', '.join(skipped_fw)}) — the list must be captured with full coverage",
                  file=sys.stderr)
            return 2
        RATCHET.write_text("\n".join(never) + "\n")
        fw_only = sorted(n for n, ex in hits.items() if ex and ex <= gated)
        FIRMWARE_ONLY.write_text("\n".join(fw_only) + "\n")
        print(f"wrote {RATCHET.relative_to(REPO_ROOT)} ({len(never)} names), "
              f"{FIRMWARE_ONLY.relative_to(REPO_ROOT)} ({len(fw_only)} names) and "
              f"{REPORT.relative_to(REPO_ROOT)}")
        return 0
    known = set(RATCHET.read_text().split()) if RATCHET.is_file() else set()
    new = sorted(set(never) - known)
    if skipped_fw and FIRMWARE_ONLY.is_file():
        exempt = set(FIRMWARE_ONLY.read_text().split())
        dropped = [n for n in new if n in exempt]
        new = [n for n in new if n not in exempt]
        print(f"  note: {len(skipped_fw)} firmware-gated example(s) skipped; "
              f"{len(dropped)} function(s) executed only by them are exempt from the check")
    gone = sorted(known - set(never))
    for n in new:
        print(f"  NEW never-executed public function: {n} ({public[n]}) — add an example or a "
              f"libtest, or --update with a reason")
    for n in gone:
        print(f"  now executed (was listed): {n} — run --update to shrink the ratchet")
    if new:
        print(f"ROM coverage ratchet: {len(new)} new never-executed function(s)")
        return 1
    print(f"ROM coverage ratchet: OK ({len(never)} listed, {len(gone)} could be removed)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
