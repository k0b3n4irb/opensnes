#!/usr/bin/env python3
"""
check_asm_abi.py — verify lib/source/*.asm stack-relative reads match the
calling convention encoded by lib/include/snes/*.h C signatures.

Background
==========
Hand-written ASM in lib/source/ implements functions whose C signatures
are declared in lib/include/snes/. The 65816 calling convention (cc65816,
LEFT-TO-RIGHT push) lays each argument out at a known stack offset. If
the C signature changes (e.g. chantier A6 widened pointers from 2 bytes
to 4 bytes), every hand-written ASM that reads those args via `lda N,s`
must update its offsets, or the function silently reads garbage.

That class of bug bit us once (chantier A6+A7 hdmaSetupBank /
hdmaSetTable). Mesen2 visual validation caught it. This script catches
it at build time.

What it checks
==============
For each function in any lib/source/*.asm:
  1. Look up its C signature in lib/include/snes/*.h
  2. Compute the expected stack offset of each argument per the cc65816
     calling convention (post-A6 ABI: ptr/u32 = 4 bytes, others = 2)
  3. Walk the ASM body. For every `lda N,s` / `sta N,s` / `ldx N,s` /
     `stx N,s` / `ldy N,s` / `sty N,s` instruction whose INLINE COMMENT
     names a parameter, verify the named param actually lives at offset N

Functions without a discoverable C signature are SKIPPED (no false
positives on internal helpers). Functions whose body reads have no inline
comment naming a param are also skipped — there's nothing structural to
check.

Exit status
===========
  0 — all annotated reads match expected layout, or no annotations to check
  1 — at least one read names a param that the C signature says lives elsewhere

CI: wired into `make lint` via the doc-drift / ASM marker pipeline.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path


# ----------------------------------------------------------------------------
# Calling convention
# ----------------------------------------------------------------------------
#
# cc65816 pushes args left-to-right. Each arg occupies an aligned slot:
#   u8, u16, s8, s16:       2 bytes (8-bit args pad to 16-bit slot)
#   u32, s32, pointer:      4 bytes  (post-A6 chantier — was 2 pre-A6)
#
# Stack layout after function prologue `php; jsl ... ; <inside callee>`:
#   1,s            saved P (from PHP)
#   2-4,s          24-bit return address (from JSL)
#   5,s onward     arguments, LAST pushed (= rightmost in C decl) is closest
#                  to SP. So offsets grow as we walk the C signature from
#                  right to left.

ABI_SIZE = {
    "u8":  2, "s8":  2, "char":  2, "bool": 2,
    "u16": 2, "s16": 2, "int":   2, "short": 2,
    "u32": 4, "s32": 4, "long":  4,
    # all pointer types
    "_ptr": 4,
}

# Base offset of the FIRST (rightmost / last-pushed) argument from PHP frame.
# 1,s = P, 2..4 = return addr → 5 = first byte of last-pushed arg.
ABI_BASE = 5


def slot_size(c_type: str) -> int:
    """Map a C type to its stack slot size in bytes."""
    t = c_type.strip()
    if t.endswith("*") or "void *" in t:
        return ABI_SIZE["_ptr"]
    # strip qualifiers like `const`, `volatile`
    for q in ("const", "volatile", "restrict"):
        t = t.replace(q, "").strip()
    # collapse multi-space → single
    t = re.sub(r"\s+", " ", t)
    # unsigned / signed prefix → ignore (size is determined by the noun)
    base = t.replace("unsigned ", "").replace("signed ", "").strip()
    if base in ABI_SIZE:
        return ABI_SIZE[base]
    # Pointer with whitespace before *
    if "*" in t:
        return ABI_SIZE["_ptr"]
    return -1  # unknown → caller decides what to do


# ----------------------------------------------------------------------------
# Header parsing
# ----------------------------------------------------------------------------

# Matches `void funcName(arg1, arg2, ...);` across one or multiple lines.
# Restricted to common return types; the SDK uses void / u8 / u16 / u32 / s* / pointers.
SIG_RX = re.compile(
    r"^\s*"                                                  # leading ws
    r"(?:(?:extern|static|inline)\s+)*"                      # qualifiers (ignored)
    r"(?P<ret>(?:const\s+)?(?:unsigned\s+|signed\s+)?"
    r"(?:void|u8|s8|u16|s16|u32|s32|int|char|short|long|bool)"
    r"(?:\s*\*)*)"                                           # return type
    r"\s+(?P<name>[A-Za-z_][A-Za-z0-9_]*)"                   # function name
    r"\s*\((?P<args>[^;{]*?)\)"                              # arg list
    r"\s*;",
    re.MULTILINE,
)


@dataclass
class CParam:
    name: str
    c_type: str
    size: int    # bytes on stack

    def __repr__(self) -> str:
        return f"{self.c_type} {self.name} (size={self.size})"


@dataclass
class CSig:
    name: str
    params: list[CParam]
    header_path: Path
    line: int

    def offsets(self, has_php: bool, prologue_shift: int = 0) -> dict[str, int]:
        """Map each param name to its stack offset (from the callee's view).

        has_php controls the base: with PHP, args start at 5,s; without, 4,s.
        prologue_shift adds the bytes pushed by phb/phx/phy on top of that.
        """
        # Args are pushed left-to-right; last pushed (rightmost) sits at base.
        # Walk RIGHT to LEFT, accumulating offsets.
        result: dict[str, int] = {}
        off = (5 if has_php else 4) + prologue_shift
        for p in reversed(self.params):
            if p.size <= 0:
                return {}
            result[p.name] = off
            off += p.size
        return result


def parse_params(s: str) -> list[CParam]:
    """Parse a comma-separated arg list from a C function declaration."""
    s = s.strip()
    if not s or s == "void":
        return []
    out: list[CParam] = []
    # naive split on commas — fine for simple SDK signatures (no function ptrs
    # with comma-separated args inside)
    for raw in s.split(","):
        raw = raw.strip()
        # last identifier is the name; everything before is the type
        m = re.match(r"(.+?)\s+(\*\s*)?([A-Za-z_][A-Za-z0-9_]*)\s*$", raw)
        if not m:
            # unparseable arg → skip the whole sig
            return []
        type_part, ptr_part, name = m.group(1), m.group(2) or "", m.group(3)
        full_type = (type_part + " " + ptr_part).strip()
        out.append(CParam(name=name, c_type=full_type, size=slot_size(full_type)))
    return out


# File-level marker: when present in an ASM source's first 30 lines,
# the entire file is skipped by the lint. Use this for legacy code that
# uses a non-cc65816 calling convention (e.g. PVSnesLib R-to-L + 1-byte
# packed u8). The detail is captured in a chantier note; the marker
# turns lint passes back to green without papering over the issue.
#
#   ; lint-asm-abi: skip-file pvsneslib-legacy-cc
#
ASM_FILE_SKIP_RX = re.compile(
    r";\s*lint-asm-abi:\s*skip-file\b",
    re.IGNORECASE,
)


def file_is_skipped(asm_path: Path) -> bool:
    """True if the file's first 30 lines carry the skip-file marker."""
    try:
        with asm_path.open() as f:
            for i, line in enumerate(f):
                if i >= 30:
                    return False
                if ASM_FILE_SKIP_RX.search(line):
                    return True
    except OSError:
        return False
    return False


def load_signatures(include_dir: Path) -> dict[str, CSig]:
    """Walk include_dir for *.h, return name → CSig for every function decl."""
    sigs: dict[str, CSig] = {}
    for hdr in sorted(include_dir.rglob("*.h")):
        text = hdr.read_text(encoding="utf-8", errors="replace")
        # strip block comments to avoid matching example signatures in docs
        text_nocomments = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
        for m in SIG_RX.finditer(text_nocomments):
            name = m.group("name")
            params = parse_params(m.group("args"))
            line = text[:m.start()].count("\n") + 1
            # If we see the same name twice (e.g. overloaded across HiROM/LoROM),
            # the first wins. Real conflicts would need disambiguation but the
            # SDK doesn't have function overloading.
            if name in sigs:
                continue
            sigs[name] = CSig(name=name, params=params, header_path=hdr, line=line)
    return sigs


# ----------------------------------------------------------------------------
# ASM parsing
# ----------------------------------------------------------------------------

# Match a function entry label at column 0, alphanumeric + underscore.
ASM_LABEL_RX = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*):\s*$")
# Prologue pushes that shift the arg base by a known number of bytes:
#   phb  → +1   (data bank reg, 1 byte)
#   phd  → +2   (direct page reg, 2 bytes) — and a strong PVSnesLib-legacy marker
#   phx  → +2   (assumed 16-bit X — the OpenSNES lib convention)
#   phy  → +2   (assumed 16-bit Y)
ASM_PROLOGUE_PUSH_RX = re.compile(r"^\s*(phb|phd|phx|phy)\b", re.IGNORECASE)
PROLOGUE_PUSH_SHIFT = {"phb": 1, "phd": 2, "phx": 2, "phy": 2}
# `phd` is the canonical marker for legacy PVSnesLib code: it indicates the
# function saves the direct-page register and uses the old R-to-L 1-byte-packed
# convention. Only audio.asm uses phd in the OpenSNES lib today. Functions
# matching this stay skipped (the calling convention itself is non-cc65816).
ASM_PVSNESLIB_MARKER_RX = re.compile(r"^\s*phd\b", re.IGNORECASE)
# PHP push — shifts arg base from 4,s to 5,s.
ASM_PHP_RX = re.compile(r"^\s*php\b", re.IGNORECASE)
# Functions that allocate a local frame (`tsa; sec; sbc #N; tas` or similar)
# read slots in [1, framesize] as locals. Detect the prologue allocation.
ASM_FRAME_RX = re.compile(r"sbc(?:\.w)?\s+#\$?[0-9A-Fa-f]+")
# Match a stack-relative load/store with an inline comment naming a param.
ASM_STACK_RX = re.compile(
    r"^\s*(?P<op>(?:lda|sta|ldx|stx|ldy|sty|adc|sbc|cmp|cpx|cpy|and|ora|eor)"
    r"(?:\.[wb])?)"
    r"\s+(?P<off>[0-9]+)\s*,\s*s"
    r"\s*;\s*(?P<comment>.+?)\s*$",
    re.IGNORECASE,
)
# Function-end markers used by lib/source/*.asm.
ASM_END_RX = re.compile(r"^\s*(rtl|rts|\.ENDS\b)", re.IGNORECASE)


@dataclass
class AsmRead:
    """A single `op N,s ; comment` from inside an ASM function body."""
    func: str
    file: Path
    line: int
    op: str
    offset: int
    comment: str


@dataclass
class AsmFunction:
    name: str
    reads: list[AsmRead]
    # True if the prologue pushes phd (PVSnesLib R-to-L marker).
    # Lint skips these — the calling convention itself is non-cc65816.
    pvsneslib_cc: bool
    # True if the function pushes PHP in its prologue. Affects the base offset
    # of arguments: with PHP, args start at 5,s; without, at 4,s.
    has_php: bool
    # Bytes pushed in the prologue ON TOP of PHP+JSL return (i.e. phb/phx/phy).
    # Added to the base offset before consulting param positions.
    prologue_shift: int

    # Backwards-compat alias: the lint historically used `custom_cc` to mean
    # "skip this function entirely". Keep the same surface but route it
    # through the new pvsneslib_cc field.
    @property
    def custom_cc(self) -> bool:
        return self.pvsneslib_cc


def parse_asm(asm_path: Path) -> list[AsmFunction]:
    """Walk asm_path, return parsed function info."""
    funcs: list[AsmFunction] = []
    current: AsmFunction | None = None
    seen_label_recently = False  # tracks "we just hit a label, scanning prologue"
    with asm_path.open() as f:
        for lineno, line in enumerate(f, start=1):
            stripped = line.rstrip("\n")

            label_m = ASM_LABEL_RX.match(stripped)
            if label_m:
                if current is not None:
                    funcs.append(current)
                current = AsmFunction(
                    name=label_m.group(1), reads=[], pvsneslib_cc=False,
                    has_php=False, prologue_shift=0,
                )
                seen_label_recently = True
                continue

            if current is None:
                continue

            # Detect prologue features in the first few instructions:
            # - phd → PVSnesLib R-to-L marker, skip the function
            # - phb/phx/phy → shift arg base by a known amount
            # - php → arg base shifts from 4,s to 5,s
            # Once we hit a stack-relative read, we're past the prologue.
            if seen_label_recently:
                if ASM_PVSNESLIB_MARKER_RX.match(stripped):
                    current.pvsneslib_cc = True
                push_m = ASM_PROLOGUE_PUSH_RX.match(stripped)
                if push_m:
                    current.prologue_shift += PROLOGUE_PUSH_SHIFT[
                        push_m.group(1).lower()]
                if ASM_PHP_RX.match(stripped):
                    current.has_php = True
                if ASM_STACK_RX.match(stripped):
                    seen_label_recently = False

            if current and ASM_END_RX.match(stripped):
                if ".ENDS" in stripped:
                    funcs.append(current)
                    current = None
                    seen_label_recently = False
                continue

            m = ASM_STACK_RX.match(stripped)
            if not m:
                continue
            current.reads.append(AsmRead(
                func=current.name,
                file=asm_path,
                line=lineno,
                op=m.group("op").lower(),
                offset=int(m.group("off")),
                comment=m.group("comment").strip(),
            ))

    if current is not None:
        funcs.append(current)
    return funcs


# ----------------------------------------------------------------------------
# Cross-check
# ----------------------------------------------------------------------------

# Hints in inline comments that ARE NOT param references (skip these).
NON_PARAM_HINTS = {
    "saved", "pushed", "stash", "temp", "scratch", "local",
    "spill", "return addr", "frame", "padding",
}


def comment_names_param(comment: str, params: list[CParam]) -> str | None:
    """If the inline comment mentions a param by name, return that name.
    Heuristic: first whole-word match against the param name list."""
    low = comment.lower()
    for hint in NON_PARAM_HINTS:
        if hint in low:
            return None
    for p in params:
        # whole-word match (avoid `mode` matching `modeline`, etc.)
        if re.search(rf"\b{re.escape(p.name.lower())}\b", low):
            return p.name
    return None


def valid_offsets_for_param(param: CParam, base: int) -> set[int]:
    """Return all stack offsets that legitimately reference this param.

    A u8/u16 param at offset N occupies bytes N..N+1 (16-bit slot).
    A pointer/u32 param at offset N occupies bytes N..N+3 — code may read:
      N+0  low byte / low word
      N+2  high byte / bank byte (the part that distinguishes from pre-A6)
      N+3  high-byte's pad byte (rarely)
    Comments naming the param are legitimate at any of those offsets.
    """
    return {base + i for i in range(param.size)}


def check(asm_files: list[Path], sigs: dict[str, CSig]) -> list[str]:
    """Return list of human-readable error messages (empty = all good)."""
    errors: list[str] = []
    for asm in asm_files:
        if file_is_skipped(asm):
            continue
        funcs = parse_asm(asm)
        for fn in funcs:
            sig = sigs.get(fn.name)
            if sig is None:
                continue  # internal helper, no declared C signature
            if fn.pvsneslib_cc:
                continue  # phd marker → PVSnesLib R-to-L CC, skip
            offsets = sig.offsets(has_php=fn.has_php,
                                  prologue_shift=fn.prologue_shift)
            if not offsets:
                continue  # unparseable signature

            # Build {param_name: set_of_valid_offsets}
            valid: dict[str, set[int]] = {}
            for p in sig.params:
                if p.name in offsets:
                    valid[p.name] = valid_offsets_for_param(p, offsets[p.name])

            for r in fn.reads:
                named = comment_names_param(r.comment, sig.params)
                if named is None:
                    continue
                if named not in valid:
                    continue
                if r.offset in valid[named]:
                    continue  # legitimate read of this param (low/high/bank)

                # Mismatch — describe which slot the offset actually hits.
                at_offset: str
                hit = [n for n, ofs in valid.items() if r.offset in ofs]
                if hit:
                    at_offset = "/".join(hit)
                else:
                    at_offset = "(no param slot)"
                errors.append(
                    f"{r.file}:{r.line}: in {fn.name}(): "
                    f"`{r.op} {r.offset},s ; {r.comment}` references param "
                    f"`{named}` but offset {r.offset},s is slot for "
                    f"`{at_offset}`. Expected `{named}` at offsets "
                    f"{sorted(valid[named])} (sig: {sig.header_path}:{sig.line})"
                )
    return errors


# ----------------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[1])
    parser.add_argument(
        "--include",
        type=Path,
        default=Path("lib/include/snes"),
        help="C header root (default: lib/include/snes)",
    )
    parser.add_argument(
        "--source",
        type=Path,
        default=Path("lib/source"),
        help="ASM source root (default: lib/source)",
    )
    parser.add_argument(
        "--quiet", "-q",
        action="store_true",
        help="Suppress the per-file scan summary",
    )
    args = parser.parse_args()

    if not args.include.is_dir():
        print(f"check_asm_abi: include dir not found: {args.include}", file=sys.stderr)
        return 2
    if not args.source.is_dir():
        print(f"check_asm_abi: source dir not found: {args.source}", file=sys.stderr)
        return 2

    sigs = load_signatures(args.include)
    asm_files = sorted(args.source.glob("*.asm"))
    if not args.quiet:
        print(f"check_asm_abi: {len(sigs)} signatures from {args.include}, "
              f"{len(asm_files)} ASM files in {args.source}", file=sys.stderr)

    errors = check(asm_files, sigs)
    if errors:
        for e in errors:
            print(e, file=sys.stderr)
        print(f"\ncheck_asm_abi: {len(errors)} ABI mismatch(es) found.", file=sys.stderr)
        return 1
    if not args.quiet:
        print("check_asm_abi: OK (no ABI mismatches)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
