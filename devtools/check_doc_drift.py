#!/usr/bin/env python3
"""Doc-drift sentinel for OpenSNES.

Compares anchored claims across multiple docs against the canonical sources of
truth, and exits non-zero if any drift is detected.

Anchored claims currently watched:

  1. ``lib/include/snes.h`` ``OPENSNES_VERSION_{MAJOR,MINOR,PATCH,STRING}``
     must match the head version in ``CHANGELOG.md`` (the topmost
     ``## [X.Y.Z]`` heading after ``## [Unreleased]``).

  2. ``ROADMAP.md`` "Current Status: post-vX.Y.Z" line must match
     ``CHANGELOG.md`` head — i.e. the in-development line cannot be more
     than one minor version behind the latest release.

  3. ``find examples -name 'main.c' | wc -l`` must match every "N example(s)"
     claim in active rule files (``.claude/rules/*.md``) and ``ROADMAP.md``.
     Historical entries in ``CHANGELOG.md`` are exempt — they freeze a
     past state.

  4. Public C function prototypes quoted in ``compiler/ABI.md`` must match
     the canonical declaration in ``lib/include/snes/*.h``. ABI.md is the
     canonical ABI reference; a stale prototype there makes its whole
     stack-offset table wrong. Caught historically as the ``oamSet`` worked
     example pinning a 6-arg ``(u8 id, u16 x, u16 y, u16 attr, u8 size,
     u16 tile)`` signature while the real API is the 7-arg ``(u16 id,
     u16 x, u16 y, u16 tile, u16 palette, u16 priority, u16 flags)`` —
     the entire offset table was off.

Why this exists: the v0.1.0-dev macro stuck at v0.16.0, the post-v0.13.0
ROADMAP stale by three minor versions, and the "53 examples" / "54 examples"
count drift across rules — all of these were caught by a 1-day external
review and would have been caught earlier by this script. See the related
``.claude/rules/doc_consistency.md`` and the lint job in ``.github/workflows/lint.yml``.

Usage:

    python3 devtools/check_doc_drift.py            # run all checks
    python3 devtools/check_doc_drift.py --fix      # autofix where safe (TBD)
    python3 devtools/check_doc_drift.py --quiet    # only print drift / errors

Exit codes:
    0   no drift
    1   drift detected (or invocation error)
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


# --------------------------------------------------------------------------
# Helpers
# --------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parent.parent


def repo_path(*parts: str) -> Path:
    return REPO_ROOT.joinpath(*parts)


class Drift(Exception):
    """A specific drift finding. Aggregated and reported at the end."""


# --------------------------------------------------------------------------
# Canonical truth: head version + examples count
# --------------------------------------------------------------------------

CHANGELOG_HEAD_RE = re.compile(
    r"^##\s*\[(?P<ver>\d+\.\d+\.\d+)\]\s*[—-]\s*(?P<date>\d{4}-\d{2}-\d{2})",
    re.MULTILINE,
)


def canonical_version() -> tuple[str, str]:
    """Return (version, date) of the head release in CHANGELOG.md.

    Skips ``## [Unreleased]`` if present.
    """
    text = repo_path("CHANGELOG.md").read_text(encoding="utf-8")
    for match in CHANGELOG_HEAD_RE.finditer(text):
        return match.group("ver"), match.group("date")
    raise SystemExit("CHANGELOG.md: no '## [X.Y.Z] - YYYY-MM-DD' heading found")


def canonical_examples_count() -> int:
    """Number of example ROMs (one ``main.c`` per example)."""
    examples = list(repo_path("examples").rglob("main.c"))
    return len(examples)


# --------------------------------------------------------------------------
# Check 1: snes.h version macros
# --------------------------------------------------------------------------

VERSION_RE = {
    "MAJOR": re.compile(r"#define\s+OPENSNES_VERSION_MAJOR\s+(\d+)"),
    "MINOR": re.compile(r"#define\s+OPENSNES_VERSION_MINOR\s+(\d+)"),
    "PATCH": re.compile(r"#define\s+OPENSNES_VERSION_PATCH\s+(\d+)"),
    "STRING": re.compile(r'#define\s+OPENSNES_VERSION_STRING\s+"([^"]+)"'),
}


def check_snes_h_version(canonical: str) -> list[str]:
    """Return drift messages for ``lib/include/snes.h`` version macros."""
    drifts: list[str] = []
    header = repo_path("lib/include/snes.h").read_text(encoding="utf-8")
    major, minor, patch = canonical.split(".")

    expected = {
        "MAJOR": major,
        "MINOR": minor,
        "PATCH": patch,
        "STRING": canonical,
    }

    for key, rx in VERSION_RE.items():
        match = rx.search(header)
        if not match:
            drifts.append(f"lib/include/snes.h: OPENSNES_VERSION_{key} macro missing")
            continue
        actual = match.group(1)
        if actual != expected[key]:
            drifts.append(
                f"lib/include/snes.h: OPENSNES_VERSION_{key} = {actual!r} "
                f"but CHANGELOG.md head is {expected[key]!r}"
            )
    return drifts


# --------------------------------------------------------------------------
# Check 2: ROADMAP.md "post-vX.Y.Z" line
# --------------------------------------------------------------------------

ROADMAP_STATUS_RE = re.compile(
    r"^## Current Status:\s*post-v(?P<ver>\d+\.\d+\.\d+)",
    re.MULTILINE,
)


def check_roadmap_status(canonical: str) -> list[str]:
    text = repo_path("ROADMAP.md").read_text(encoding="utf-8")
    match = ROADMAP_STATUS_RE.search(text)
    if not match:
        return ["ROADMAP.md: '## Current Status: post-vX.Y.Z' line not found"]
    actual = match.group("ver")
    if actual != canonical:
        return [
            f"ROADMAP.md: 'Current Status: post-v{actual}' but CHANGELOG.md "
            f"head is {canonical} — bump the line in the same release commit"
        ]
    return []


# --------------------------------------------------------------------------
# Check 3: examples count consistency across active docs
# --------------------------------------------------------------------------

# Look for "<N> example(s)" or "All <N> examples" in active docs.
# Active = ROADMAP.md + .claude/rules/*.md.
# Excluded = CHANGELOG.md (historical) and any docs/ tutorial that pins a
# past state intentionally.
COUNT_PATTERNS = [
    re.compile(r"\b(\d{2,3})\s+working\s+examples?\b", re.IGNORECASE),
    re.compile(r"\b(\d{2,3})\s+examples?\s+(?:cover|organized|across)", re.IGNORECASE),
    re.compile(r"\bAll\s+(\d{2,3})\s+examples?\b", re.IGNORECASE),
    re.compile(r"\ball\s+(\d{2,3})\s+examples\s+compile\s+cleanly\b", re.IGNORECASE),
]

# Files where an example count is allowed to be stale (historical record).
COUNT_STALE_OK_GLOBS = ["CHANGELOG.md"]


def _gather_active_doc_paths() -> list[Path]:
    paths: list[Path] = [repo_path("ROADMAP.md"), repo_path("README.md")]
    rules = repo_path(".claude/rules")
    if rules.is_dir():
        paths.extend(sorted(rules.glob("*.md")))
    return paths


def check_examples_count(canonical: int) -> list[str]:
    seen: set[tuple[str, int, int]] = set()
    drifts: list[str] = []
    for path in _gather_active_doc_paths():
        if path.name in COUNT_STALE_OK_GLOBS:
            continue
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8")
        for lineno, line in enumerate(text.splitlines(), 1):
            for rx in COUNT_PATTERNS:
                m = rx.search(line)
                if not m:
                    continue
                count = int(m.group(1))
                # Skip obviously-irrelevant numbers (e.g. "all 256 colours").
                if count < 10 or count > 999:
                    continue
                if count == canonical:
                    continue
                rel = str(path.relative_to(REPO_ROOT))
                key = (rel, lineno, count)
                if key in seen:
                    continue
                seen.add(key)
                drifts.append(
                    f"{rel}:{lineno}: claims {count} example(s) but "
                    f"`find examples -name main.c | wc -l = {canonical}` — "
                    f"update the line or, if it's an off-by-one drift, "
                    f"replace the number with `<run --list>`-style language"
                )
    return drifts


# --------------------------------------------------------------------------
# Check 4: ABI.md C prototypes vs canonical headers
# --------------------------------------------------------------------------

HEADER_DIR = "lib/include/snes"

# A C function prototype: <return type> <name>(<params>);
#
# Deliberately strict to avoid matching prose with parentheses:
#   * the return type and name sit on a single line (no newline in `ret`,
#     none between `name` and `(`) — a real declaration never wraps there;
#   * the parameter list admits only the characters of an actual param list
#     (identifiers, whitespace, `*`, `,`, `[]`) — backticks, prose
#     punctuation and nested `(` are excluded, so a sentence like
#     ``handshakes (`vblank_flag`, ...)`` can never be mistaken for one.
# Function-pointer params are not modelled (the SDK has none); such a
# prototype simply won't match and is skipped rather than mis-parsed.
_C_PROTO_RE = re.compile(
    r"(?P<ret>[A-Za-z_][\w \t\*]*?)\b(?P<name>[A-Za-z_]\w*)[ \t]*"
    r"\((?P<params>[\w\s\*,\[\]]*)\)[ \t]*;",
)

# C block/line comments stripped before prototype scanning.
_C_BLOCK_COMMENT_RE = re.compile(r"/\*.*?\*/", re.DOTALL)
_C_LINE_COMMENT_RE = re.compile(r"//[^\n]*")


def _strip_c_comments(text: str) -> str:
    """Blank out C comments while preserving offsets and line numbering.

    Each comment is replaced by spaces and its own newlines, so a match's
    ``start()`` still maps to the right line in the original text.
    """
    def _blank(m: re.Match) -> str:
        return "".join("\n" if ch == "\n" else " " for ch in m.group(0))

    text = _C_BLOCK_COMMENT_RE.sub(_blank, text)
    text = _C_LINE_COMMENT_RE.sub(_blank, text)
    return text


def _is_typed_param_list(params: str) -> bool:
    """True if every parameter looks like a typed declaration (``type name``).

    This is what separates a real prototype (``oamSet(u16 id, u16 x, ...)``)
    from a *call* in a code block (``oamSet(0, 100, ...)``) or a fenced-code
    language tag bleeding into the match. Function-call arguments are bare
    literals / dotted expressions with no ``type name`` shape.
    """
    s = params.strip()
    if s == "" or s == "void":
        return True
    for p in s.split(","):
        toks = re.sub(r"\*", " * ", p).split()
        idents = [t for t in toks if t != "*"]
        # Need at least a type token and a name token, both plain C identifiers.
        if len(idents) < 2:
            return False
        if not re.fullmatch(r"[A-Za-z_]\w*", idents[0]):
            return False
        if not re.fullmatch(r"[A-Za-z_]\w*", idents[-1]):
            return False
    return True


def _normalize_param(decl: str) -> str:
    """Normalize one parameter declaration to a canonical 'type name' string."""
    decl = decl.strip()
    if not decl or decl == "void":
        return ""
    # Trailing identifier is the parameter name; the rest is the type.
    m = re.search(r"([A-Za-z_]\w*)\s*$", decl)
    if not m:
        return re.sub(r"\s+", " ", decl)
    name = m.group(1)
    typ = decl[: m.start()].strip()
    typ = re.sub(r"\s*\*\s*", " *", typ)  # canonical pointer spacing
    typ = re.sub(r"\s+", " ", typ).strip()
    if not typ:  # bare type with no name (e.g. unnamed); keep as type only
        return name
    return f"{typ} {name}"


def _normalize_signature(ret: str, name: str, params: str) -> str:
    ret = re.sub(r"\s*\*\s*", " *", ret)
    ret = re.sub(r"\s+", " ", ret).strip()
    parts = [p for p in (_normalize_param(p) for p in params.split(",")) if p]
    return f"{ret} {name}({', '.join(parts)})"


def _header_signatures() -> dict[str, tuple[str, str]]:
    """Map public function name -> (normalized signature, source 'file').

    If a name is declared in more than one header, the first wins and the
    duplicate is ignored (the ABI doc references a single canonical decl).
    """
    sigs: dict[str, tuple[str, str]] = {}
    hdr_dir = repo_path(HEADER_DIR)
    if not hdr_dir.is_dir():
        return sigs
    for hdr in sorted(hdr_dir.glob("*.h")):
        text = _strip_c_comments(hdr.read_text(encoding="utf-8"))
        for m in _C_PROTO_RE.finditer(text):
            name = m.group("name")
            ret = m.group("ret").strip()
            params = m.group("params")
            # Skip control-flow keywords masquerading as return types and
            # anything that doesn't look like a declaration (e.g. 'if (...)').
            if ret in ("", "return", "if", "while", "for", "switch", "sizeof"):
                continue
            if not _is_typed_param_list(params):
                continue
            norm = _normalize_signature(ret, name, params)
            sigs.setdefault(name, (norm, hdr.name))
    return sigs


def check_abi_signatures() -> list[str]:
    """ABI.md prototypes for public SDK functions must match the headers."""
    abi = repo_path("compiler/ABI.md")
    if not abi.is_file():
        return []
    headers = _header_signatures()
    if not headers:
        return []

    drifts: list[str] = []
    text = abi.read_text(encoding="utf-8")
    for m in _C_PROTO_RE.finditer(_strip_c_comments(text)):
        name = m.group("name")
        if name not in headers:
            continue  # illustrative prototype, not a real SDK function
        ret = m.group("ret").strip()
        if ret in ("return", "if", "while", "for", "switch", "sizeof"):
            continue
        params = m.group("params")
        if not _is_typed_param_list(params):
            continue  # a call or non-declaration, not a prototype
        actual = _normalize_signature(ret, name, params)
        expected, src = headers[name]
        if actual != expected:
            # Line number of the match start in the original text.
            lineno = text.count("\n", 0, m.start()) + 1
            drifts.append(
                f"compiler/ABI.md:{lineno}: prototype for `{name}` is\n"
                f"      {actual}\n"
                f"    but {HEADER_DIR}/{src} declares\n"
                f"      {expected}\n"
                f"    — update the ABI.md worked example (signature, codegen "
                f"push list, and the stack-offset table) to match the header"
            )
    return drifts


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------


def run_checks(quiet: bool) -> int:
    canonical_ver, canonical_date = canonical_version()
    canonical_n = canonical_examples_count()

    if not quiet:
        print(f"canonical version : {canonical_ver} ({canonical_date})")
        print(f"canonical examples: {canonical_n}")
        print()

    all_drifts: list[str] = []
    all_drifts.extend(check_snes_h_version(canonical_ver))
    all_drifts.extend(check_roadmap_status(canonical_ver))
    all_drifts.extend(check_examples_count(canonical_n))
    all_drifts.extend(check_abi_signatures())

    if all_drifts:
        print("DRIFT DETECTED:", file=sys.stderr)
        for d in all_drifts:
            print(f"  - {d}", file=sys.stderr)
        print(
            f"\nFix the items above and rerun `make lint-docs`. See "
            f"`.claude/rules/doc_consistency.md` for the full policy.",
            file=sys.stderr,
        )
        return 1

    if not quiet:
        print("OK: no doc drift detected.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="OpenSNES doc-drift sentinel — see .claude/rules/doc_consistency.md",
    )
    parser.add_argument("--quiet", action="store_true",
                        help="suppress non-error output")
    args = parser.parse_args()
    try:
        return run_checks(args.quiet)
    except FileNotFoundError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
