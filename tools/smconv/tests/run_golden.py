#!/usr/bin/env python3
"""Golden-output tests for smconv (SNESMOD soundbank converter).

Copies the fixture .it module into a temp dir, runs smconv with the
canonical soundbank flags from make/common.mk, and byte-compares the
produced soundbank files against the committed goldens. smconv output
is deterministic, so any diff is a real behaviour change: either a
regression, or an intentional change that must be re-goldened.

Fixture provenance (see ATTRIBUTION.md):
  fixtures/pollen8.it    = examples/audio/snesmod_music/music/pollen8.it
                           (PVSnesLib-derived asset already in the repo)
  fixtures/reflection.it = modlib itmod/test/reflection.it (MIT, Mukunda
                           Johnson): one 8-bit IT 2.14 compressed sample,
                           written by a real tracker

Compressed samples (2026-10-03). reflection.it is the one file not made
here; its decoded PCM was checked byte for byte against modlib's
doodle.raw before its golden was committed. Every other compressed input
is generated in the temp dir by itcompress.py from pollen8.it, and the
soundbank must be the one its uncompressed twin gives: the committed
golden for the plain module, smconv's own output in the same run for the
16-bit and multi-block twins. Encoder and decoder being by the same hand,
those prove consistency, not conformance — reflection.it is the outside
witness.

Run:  python3 tools/smconv/tests/run_golden.py
"""
from __future__ import annotations

import filecmp
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
TOOL = REPO / "bin" / "smconv"

sys.path.insert(0, str(HERE))
import itcompress  # noqa: E402

FIXTURE = "pollen8.it"
COMPRESSED_FIXTURE = "reflection.it"
# Canonical invocation from make/common.mk's soundbank rule.
FLAGS = ["-s", "-o", "soundbank", "-b", "1", "-n", "-p", "soundbank"]
OUTPUTS = ["soundbank.asm", "soundbank.h", "soundbank.bnk"]
# Long enough for the longest pollen8 sample (2122 frames) to span two
# 8-bit blocks (0x8000 frames) and three 16-bit ones (0x4000).
STRETCH = 16


def convert(work: Path, name: str, data: bytes) -> subprocess.CompletedProcess:
    """Write `data` as work/name and run smconv on it there."""
    work.mkdir(parents=True, exist_ok=True)
    (work / name).write_bytes(data)
    return subprocess.run([str(TOOL), *FLAGS, name],
                          cwd=work, capture_output=True, text=True, timeout=60)


def compare(work: Path, proc: subprocess.CompletedProcess, want_dir: Path) -> list[str]:
    """The run succeeded and its outputs are byte-identical to want_dir's."""
    if proc.returncode != 0:
        return [f"exit {proc.returncode}: {(proc.stderr or proc.stdout).strip()[:200]}"]
    errs = []
    for out in OUTPUTS:
        got, want = work / out, want_dir / out
        if not got.is_file():
            errs.append(f"{out}: not produced")
        elif not filecmp.cmp(got, want, shallow=False):
            errs.append(f"{out}: differs ({got.stat().st_size} "
                        f"vs {want.stat().st_size} bytes)")
    return errs


def refused(work: Path, proc: subprocess.CompletedProcess, *needles: str) -> list[str]:
    """The run failed, said why, and wrote nothing."""
    errs = []
    if proc.returncode == 0:
        errs.append("exit 0")
    said = proc.stdout + proc.stderr
    errs += [f"no message saying {n!r}" for n in needles if n not in said]
    left = [out for out in OUTPUTS if (work / out).exists()]
    if left:
        errs.append(f"wrote {', '.join(left)}")
    return errs


def compressed(src: bytes, how: str, **kw) -> tuple[bytes, list[str]]:
    """A compressed twin of `src`, and what the encoder failed to exercise."""
    data, stats = itcompress.rewrite(src, compress_as=how, **kw)
    return data, [f"encoder never used {k}" for k, v in stats.items() if v == 0]


def sample_pointer(mod: bytes, n: int) -> int:
    ordnum, insnum = struct.unpack_from("<HH", mod, 0x20)
    hdr, = struct.unpack_from("<I", mod, 0xC0 + ordnum + 4 * insnum + 4 * n)
    return struct.unpack_from("<I", mod, hdr + 0x48)[0]


def main() -> int:
    if not TOOL.is_file():
        sys.exit(f"ERROR: {TOOL} not found — run `make tools` first")
    results: list[tuple[str, str, list[str]]] = []   # (name, what passed, errors)
    pollen8 = (HERE / "fixtures" / FIXTURE).read_bytes()
    reflection = (HERE / "fixtures" / COMPRESSED_FIXTURE).read_bytes()
    with tempfile.TemporaryDirectory() as td:
        root = Path(td)

        work = root / "plain"
        results.append((FIXTURE, f"{len(OUTPUTS)} outputs match",
                        compare(work, convert(work, FIXTURE, pollen8), HERE / "golden")))

        # Negative case (2026-09-26): a file that is not an IT module must fail
        # the run and write nothing. It used to exit 0 with an empty soundbank.
        work = root / "not_an_it"
        proc = convert(work, "not_an_it.it", b"\x89PNG\r\n\x1a\n" + bytes(64))
        results.append(("not_an_it.it", "refused, nothing written",
                        refused(work, proc, "IMPM")))

        # A real compressed module (IT 2.14, 8-bit).
        work = root / "reflection"
        results.append((COMPRESSED_FIXTURE, "real IT 2.14 file, outputs match",
                        compare(work, convert(work, COMPRESSED_FIXTURE, reflection),
                                HERE / "golden" / "reflection")))

        # pollen8 with every sample compressed: the committed golden, unchanged.
        for how in ("it214", "it215"):
            work = root / how
            data, errs = compressed(pollen8, how)
            errs += compare(work, convert(work, FIXTURE, data), HERE / "golden")
            results.append((f"{FIXTURE} as {how}", "same soundbank as uncompressed", errs))

        # 16-bit and multi-block: no golden, the uncompressed twin is the
        # reference, converted in the same run.
        for bits16 in (False, True):
            label = f"{FIXTURE} x{STRETCH} {'16' if bits16 else '8'}-bit"
            twin, _ = itcompress.rewrite(pollen8, bits16=bits16, factor=STRETCH)
            ref = root / f"twin{int(bits16)}"
            proc = convert(ref, FIXTURE, twin)
            if proc.returncode != 0 or not all((ref / o).is_file() for o in OUTPUTS):
                results.append((label, "", [f"uncompressed twin not converted (exit {proc.returncode})"]))
                continue
            for how in ("it214", "it215"):
                work = root / f"twin{int(bits16)}_{how}"
                data, errs = compressed(pollen8, how, bits16=bits16, factor=STRETCH)
                errs += compare(work, convert(work, FIXTURE, data), ref)
                results.append((f"{label} as {how}", "same soundbank as its uncompressed twin", errs))

        # Negative: a compressed block that is cut short must fail the run,
        # name the sample and write nothing — once with the file ending inside
        # the block, once with the block declaring fewer bytes than its
        # samples need. (Before 2026-10-03 any compressed sample segfaulted.)
        ptr = sample_pointer(reflection, 0)
        short = bytearray(reflection)
        struct.pack_into("<H", short, ptr, 5)
        for name, data in (("reflection_eof.it", reflection[:ptr + 12]),
                           ("reflection_short_block.it", bytes(short))):
            work = root / name
            proc = convert(work, name, data)
            results.append((name, "refused, sample named, nothing written",
                            refused(work, proc, "sample 'doodle'",
                                    "corrupt compressed data (block 1)")))

    for name, what, errs in results:
        if errs:
            print(f"  FAIL {name}: " + "; ".join(errs))
        else:
            print(f"  PASS {name} ({what})")
    ok = sum(1 for _, _, errs in results if not errs)
    print(f"\nsmconv golden: {ok}/{len(results)} ok")
    return 0 if ok == len(results) else 1


if __name__ == "__main__":
    sys.exit(main())
