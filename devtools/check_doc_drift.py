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

  6. Example paths quoted in onboarding docs (``ROADMAP.md``,
     ``examples/README.md``, ``docs/GETTING_STARTED.md``) must exist under
     ``examples/``. Caught historically as GETTING_STARTED pointing
     newcomers at ``memory/superfx_3d`` (real home:
     ``graphics/effects/superfx_3d``) and ROADMAP listing a ``sa1_speed``
     example that never existed.

  7. The per-category counts in ``examples/README.md``'s table must match
     the per-directory ``main.c`` count, and their sum the corpus total.
     Caught historically as the table summing to 53 under a "56 examples"
     header (basics said 4, was 6; games said 4, was 5).

  8. ``ROADMAP.md``'s footer ``*Last updated: YYYY-MM-DD`` date must not be
     older than the head release date in ``CHANGELOG.md``. Caught
     historically as a 2026-05-07 footer under a post-v0.26.0 status line
     (release dated 2026-07-02).

Count claims (check 3) are matched on a soft-wrapped view of each doc —
single newlines count as spaces — so a claim split across two lines
("54\\nworking examples") can no longer hide from the scan.

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
    re.compile(r"\b(\d{2,3})\s+examples?\s+(?:cover|organized|across|from|by topic|as a)",
               re.IGNORECASE),
    re.compile(r"\bthrough\s+(\d{2,3})\s+examples?\b", re.IGNORECASE),  # "path through N examples"
    re.compile(r"\b(\d{2,3})\s+example\s+ROMs?\b", re.IGNORECASE),
    re.compile(r"\*\*(\d{2,3})\s+examples?\*\*", re.IGNORECASE),  # README bold table cell
    re.compile(r"\bAll\s+(\d{2,3})\s+examples?\b", re.IGNORECASE),
    re.compile(r"\ball\s+(\d{2,3})\s+examples\s+compile\s+cleanly\b", re.IGNORECASE),
    re.compile(r"\bExamples?\s*\((\d{2,3})\)", re.IGNORECASE),  # "### Examples (56)" heading
    re.compile(r"\((\d{2,3})\s*/\s*\d{2,3}\)"),   # "(56 / 56)" completion claim, numerator
    re.compile(r"\(\d{2,3}\s*/\s*(\d{2,3})\)"),   # ...and its denominator
    # Targeted phrasings only (NOT a bare "\d examples") so prose like
    # "12 examples were flagged" in bank0_budget.md stays a non-match.
]

# Files where an example count is allowed to be stale (historical record).
COUNT_STALE_OK_GLOBS = ["CHANGELOG.md"]


def _gather_active_doc_paths() -> list[Path]:
    paths: list[Path] = [repo_path("ROADMAP.md"), repo_path("README.md")]
    rules = repo_path(".claude/rules")
    if rules.is_dir():
        paths.extend(sorted(rules.glob("*.md")))
    # Onboarding docs that quote the corpus count (added 2026-06-23, TIER 1):
    # these are where a newcomer first reads "N examples". docs/ at large is NOT
    # scanned — some tutorials intentionally pin a past state.
    for rel in ("examples/README.md", "docs/EXAMPLES_BY_CATEGORY.md",
                "docs/LEARNING_PATH.md", "docs/mainpage.md"):
        p = repo_path(rel)
        if p.is_file():
            paths.append(p)
    return paths


def soft_wrap(text: str) -> str:
    """Offset-preserving soft-wrap: a single newline (not part of a blank
    line) becomes a space, so a claim split across two lines still matches,
    while paragraph breaks keep unrelated sentences apart. Caught
    historically as ROADMAP's "54\\nworking examples" hiding from the
    line-by-line scan."""
    chars = list(text)
    n = len(text)
    for i, ch in enumerate(text):
        if ch != "\n":
            continue
        prev_nl = i > 0 and text[i - 1] == "\n"
        next_nl = i + 1 < n and text[i + 1] == "\n"
        if not prev_nl and not next_nl:
            chars[i] = " "
    return "".join(chars)


def check_examples_count(canonical: int) -> list[str]:
    seen: set[tuple[str, int, int]] = set()
    drifts: list[str] = []
    for path in _gather_active_doc_paths():
        if path.name in COUNT_STALE_OK_GLOBS:
            continue
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8")
        wrapped = soft_wrap(text)
        for rx in COUNT_PATTERNS:
            for m in rx.finditer(wrapped):
                count = int(m.group(1))
                # Skip obviously-irrelevant numbers (e.g. "all 256 colours").
                if count < 10 or count > 999:
                    continue
                if count == canonical:
                    continue
                # soft_wrap preserves offsets, so the match position maps
                # back to the original line number.
                lineno = text.count("\n", 0, m.start(1)) + 1
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
# Check 5: phantom API in onboarding docs (added 2026-06-23, TIER 1)
# A function CALLED in a ```c block of an onboarding doc must exist in the
# public headers — else the newcomer's first program fails to link. Caught
# historically as `consoleDrawText()` in GETTING_STARTED.md ("your first
# program") and `audioPlaySample(channel, data, size, pitch)` in TROUBLESHOOTING.
# --------------------------------------------------------------------------

PHANTOM_DOC_PATHS = ["docs/GETTING_STARTED.md", "docs/TROUBLESHOOTING.md",
                     "docs/API_INDEX.md"]

# Control-flow + common C stdlib identifiers that appear as `name(` but are not
# SDK API (so they are not "phantom").
_PHANTOM_ALLOW = {
    "if", "for", "while", "switch", "return", "sizeof", "do", "else", "main",
    "defined", "memcpy", "memset", "memmove", "malloc", "calloc", "realloc",
    "free", "printf", "sprintf", "snprintf", "strlen", "strcpy", "strncpy",
    "strcmp", "abs",
}


def _public_api_names() -> set[str]:
    """Identifiers declared as functions / function-like macros in the headers."""
    names: set[str] = set()
    inc = repo_path("lib/include")
    for hdr in list(inc.glob("snes.h")) + sorted(inc.glob("snes/*.h")):
        text = hdr.read_text(encoding="utf-8", errors="replace")
        for m in re.finditer(r"\b([A-Za-z_]\w*)\s*\(", text):
            names.add(m.group(1))
    return names


def phantom_names_in_code(code: str, api: set[str]) -> list[str]:
    """Pure core (unit-testable): SDK-shaped calls in `code` not in `api`.

    Strips comments, ignores locally-defined functions, control flow, and common
    stdlib; only considers lowerCamel (oamSet) / CapWord (WaitForVBlank) names so
    UPPER_CASE macros (KEY_*, OBJSEL) and casts are never flagged.
    """
    code = _strip_c_comments(code)
    defined = set(re.findall(r"\b(?:void|u8|u16|s16|u32|int|static)\s+(\w+)\s*\(", code))
    bad: list[str] = []
    for m in re.finditer(r"\b([a-zA-Z_]\w*)\s*\(", code):
        name = m.group(1)
        if not re.match(r"^[a-z][A-Za-z0-9]*$|^[A-Z][a-z]\w*$", name):
            continue
        if name in api or name in _PHANTOM_ALLOW or name in defined:
            continue
        bad.append(name)
    return bad


def check_phantom_api() -> list[str]:
    api = _public_api_names()
    drifts: list[str] = []
    for rel in PHANTOM_DOC_PATHS:
        path = repo_path(rel)
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8")
        for block in re.findall(r"```c\b(.*?)```", text, re.DOTALL):
            for name in phantom_names_in_code(block, api):
                idx = text.find(name + "(")
                lineno = text[:idx].count("\n") + 1 if idx >= 0 else 0
                drifts.append(
                    f"{rel}:{lineno}: `{name}(...)` called in a C snippet but not "
                    f"declared in lib/include/snes/*.h — phantom API (won't link)."
                )
    return drifts


# --------------------------------------------------------------------------
# Check 6: example paths quoted in onboarding docs must exist
# --------------------------------------------------------------------------

EXAMPLE_PATH_DOC_PATHS = ["ROADMAP.md", "examples/README.md",
                          "docs/GETTING_STARTED.md",
                          "docs/API_INDEX.md"]

# Backticked `category/name` paths and relative markdown links like
# [text/hello_world](text/hello_world/). The category filter (first segment
# must be a real examples/ subdirectory) keeps lib/, docs/, tools/ paths out.
_EXAMPLE_PATH_RES = [
    re.compile(r"`([a-z0-9_]+(?:/[a-z0-9_]+)+)`"),
    re.compile(r"\]\(([a-z0-9_]+(?:/[a-z0-9_]+)+)/?\)"),
    # docs/ pages link out of their own directory: `](../examples/cat/name/)`
    re.compile(r"\]\(\.\./examples/([a-z0-9_]+(?:/[a-z0-9_]+)+)/?\)"),
]


def extract_example_paths(text: str, categories: set[str]) -> list[tuple[str, int]]:
    """Pure core (unit-testable): (path, lineno) of every quoted example path."""
    out: list[tuple[str, int]] = []
    for rx in _EXAMPLE_PATH_RES:
        for m in rx.finditer(text):
            path = m.group(1)
            if path.split("/")[0] in categories:
                out.append((path, text.count("\n", 0, m.start(1)) + 1))
    return out


def check_example_paths() -> list[str]:
    ex_root = repo_path("examples")
    if not ex_root.is_dir():
        return []
    categories = {p.name for p in ex_root.iterdir() if p.is_dir()}
    drifts: list[str] = []
    seen: set[tuple[str, str]] = set()
    for rel in EXAMPLE_PATH_DOC_PATHS:
        doc = repo_path(rel)
        if not doc.is_file():
            continue
        text = doc.read_text(encoding="utf-8")
        for path, lineno in extract_example_paths(text, categories):
            if (ex_root / path).is_dir() or (rel, path) in seen:
                continue
            seen.add((rel, path))
            drifts.append(
                f"{rel}:{lineno}: references `{path}` but examples/{path} "
                f"does not exist — fix the path or drop the reference"
            )
    return drifts


# --------------------------------------------------------------------------
# Check 7: examples/README.md per-category counts vs the filesystem
# --------------------------------------------------------------------------

_CATEGORY_ROW_RE = re.compile(r"\|\s*\[(\w+)/\]\([^)]+\)\s*\|\s*(\d+)\s*\|")


def parse_category_rows(text: str) -> dict[str, int]:
    """Pure core (unit-testable): {category: claimed count} from the table."""
    return {m.group(1): int(m.group(2)) for m in _CATEGORY_ROW_RE.finditer(text)}


def check_category_sums(canonical: int) -> list[str]:
    doc = repo_path("examples/README.md")
    if not doc.is_file():
        return []
    rows = parse_category_rows(doc.read_text(encoding="utf-8"))
    if not rows:
        return ["examples/README.md: category table not found (parser drift?)"]
    drifts: list[str] = []
    for cat, claimed in rows.items():
        cat_dir = repo_path("examples", cat)
        actual = len(list(cat_dir.rglob("main.c"))) if cat_dir.is_dir() else 0
        if actual != claimed:
            drifts.append(
                f"examples/README.md: category [{cat}/] claims {claimed} "
                f"example(s) but the directory holds {actual}"
            )
    total = sum(rows.values())
    if total != canonical:
        drifts.append(
            f"examples/README.md: category rows sum to {total} but the corpus "
            f"holds {canonical} examples — a row is stale"
        )
    return drifts


# --------------------------------------------------------------------------
# Check 8: ROADMAP footer date must not predate the head release
# --------------------------------------------------------------------------

ROADMAP_FOOTER_RE = re.compile(r"\*Last updated:\s*(\d{4}-\d{2}-\d{2})")


def check_roadmap_footer_date(canonical_date: str) -> list[str]:
    text = repo_path("ROADMAP.md").read_text(encoding="utf-8")
    m = ROADMAP_FOOTER_RE.search(text)
    if not m:
        return ["ROADMAP.md: footer '*Last updated: YYYY-MM-DD' not found"]
    footer = m.group(1)
    # ISO dates compare correctly as strings.
    if footer < canonical_date:
        return [
            f"ROADMAP.md: footer says 'Last updated: {footer}' but the head "
            f"release in CHANGELOG.md is dated {canonical_date} — the footer "
            f"must move with (or after) each release"
        ]
    return []


# --------------------------------------------------------------------------
# Check 9: stale pre-A6 bank-assumption comments in hand-written ASM
# --------------------------------------------------------------------------

# Post-A6, every lib ASM function reads the source bank from the far
# pointer's bank byte — prose claiming a bank $00 assumption or 16-bit-only
# pointers describes the pre-A6 world. dma.asm's "BANK LIMITATION" header
# survived two chantiers this way (the ABI lint only reads `lda N,s`
# annotations, never prose).
_STALE_BANK_COMMENT_RE = re.compile(
    r"assumes?[^\n]*bank \$00|only passes 16-bit pointers", re.IGNORECASE)


def check_asm_bank_comments() -> list[str]:
    drifts: list[str] = []
    src = repo_path("lib/source")
    if not src.is_dir():
        return []
    for asm in sorted(src.glob("*.asm")):
        text = asm.read_text(encoding="utf-8", errors="replace")
        m = _STALE_BANK_COMMENT_RE.search(text)
        if m:
            lineno = text.count("\n", 0, m.start()) + 1
            drifts.append(
                f"lib/source/{asm.name}:{lineno}: comment claims a pre-A6 "
                f"bank-$00 / 16-bit-pointer limitation — post-A6 the bank is "
                f"read from the far pointer; fix the prose (see compiler/ABI.md)"
            )
    return drifts


# --------------------------------------------------------------------------
# Check: example screenshot basenames must be unique across READMEs
#
# Doxygen flattens every markdown-referenced image into the single html/
# output dir keyed by BASENAME. Two example READMEs referencing the same
# basename (the historical `screenshot.png`) collide there — one image ends
# up served on every page (the Mario-on-audio-pages bug). Enforce that each
# example's screenshot has a unique basename so the collision cannot return.
# --------------------------------------------------------------------------

_README_IMG_RE = re.compile(r"!\[[^\]]*\]\(([^)]+?\.png)\)")


def _doxyfile_image_path_dirs() -> set[str]:
    """The set of directories (repo-relative) listed in Doxyfile IMAGE_PATH."""
    dox = repo_path("docs/Doxyfile")
    if not dox.is_file():
        return set()
    text = dox.read_text(encoding="utf-8", errors="replace")
    m = re.search(r"^IMAGE_PATH\s*=(.*?)(?=^\S|\Z)", text, re.M | re.S)
    if not m:
        return set()
    dirs = set()
    for tok in m.group(1).replace("\\", " ").split():
        # IMAGE_PATH entries are relative to docs/, e.g. ../examples/text/foo
        dirs.add(tok.lstrip("./").replace("../", "", 1) if tok.startswith("../")
                 else tok)
    return dirs


def check_screenshot_basenames() -> list[str]:
    ex_root = repo_path("examples")
    if not ex_root.is_dir():
        return []
    image_dirs = _doxyfile_image_path_dirs()
    seen: dict[str, str] = {}
    drifts: list[str] = []
    for readme in sorted(ex_root.rglob("README.md")):
        text = readme.read_text(encoding="utf-8", errors="replace")
        rel = readme.relative_to(repo_path("."))
        exdir = readme.parent
        exrel = exdir.relative_to(repo_path(".")).as_posix()
        for m in _README_IMG_RE.finditer(text):
            ref = m.group(1)
            base = ref.rsplit("/", 1)[-1]
            # 1. unique basename (Doxygen flattens by basename)
            if base in seen:
                drifts.append(
                    f"{rel}: screenshot basename '{base}' also used by "
                    f"{seen[base]} — Doxygen flattens images by basename, so "
                    f"identical names collide site-wide. Name it after the "
                    f"example's folder (e.g. <example>.png)."
                )
            else:
                seen[base] = str(rel)
            # 2. the referenced image file must exist
            if "/" not in ref and not (exdir / ref).is_file():
                drifts.append(
                    f"{rel}: references screenshot '{ref}' but the file does "
                    f"not exist in {exrel}/."
                )
            # 3. the example dir must be in Doxyfile IMAGE_PATH, or Doxygen
            #    (which does not recurse IMAGE_PATH) will not copy the image.
            if image_dirs and exrel not in image_dirs:
                drifts.append(
                    f"{rel}: {exrel} references a screenshot but is not in "
                    f"docs/Doxyfile IMAGE_PATH — the image will not be copied "
                    f"to the site. Add '../{exrel}' to IMAGE_PATH."
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
    all_drifts.extend(check_phantom_api())
    all_drifts.extend(check_abi_signatures())
    all_drifts.extend(check_example_paths())
    all_drifts.extend(check_category_sums(canonical_n))
    all_drifts.extend(check_roadmap_footer_date(canonical_date))
    all_drifts.extend(check_asm_bank_comments())
    all_drifts.extend(check_screenshot_basenames())

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
