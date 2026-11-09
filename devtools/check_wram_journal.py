#!/usr/bin/env python3
"""A rewrite of testing/baselines/wram.json leaves a line in WRAM_JOURNAL.md.

`wram_regress.py --update --why "..."` appends the line (date, commit, drifted, kind, reason, hash of the file). This
refuses a wram.json whose hash is not the last line's: the oracle was rewritten, or edited, with no reason on record. The
independent review of 2026-11-09 counted it rewritten by 168 of 747 commits, 73 of the 223 that touch lib/, compiler/ or
templates/, with one known catch.

  python3 devtools/check_wram_journal.py              # exit 1 on a mismatch
  python3 devtools/check_wram_journal.py --stats 30   # updates and catches over the last 30 days
"""
import datetime
import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASELINE = ROOT / "testing" / "baselines" / "wram.json"
JOURNAL = ROOT / "testing" / "baselines" / "WRAM_JOURNAL.md"
ROW = re.compile(r"^\|\s*(\d{4}-\d{2}-\d{2})\s*\|\s*([^|]*)\|\s*([^|]*)\|\s*([^|]*)\|\s*(.*?)\s*\|\s*(`[0-9a-f]{12}`|-)\s*\|\s*$")


def rows(text):
    return [m.groups() for m in (ROW.match(l) for l in text.splitlines()) if m]


def mismatch(baseline_bytes, journal_text):
    """None when the last row carries the hash of the baseline, else a message."""
    rs = rows(journal_text)
    if not rs:
        return "WRAM_JOURNAL.md has no row"
    last = rs[-1][5].strip("`")
    want = hashlib.sha256(baseline_bytes).hexdigest()[:12]
    if last != want:
        return (f"wram.json hashes to {want}, the last journal row says {last}: it was rewritten without a line "
                f"(python3 testing/wram_regress.py --update --why \"...\")")
    return None


def stats(journal_text, days, today=None):
    today = today or datetime.date.today()
    n = c = 0
    for d, _commit, dr, kind, _why, h in rows(journal_text):
        if h == "-" or kind.strip() == "":
            continue
        if (today - datetime.date.fromisoformat(d)).days <= days and not dr.strip().startswith("0 of"):
            n += 1
            c += "CATCH" in kind
    return n, c


def main(argv):
    if not BASELINE.is_file() or not JOURNAL.is_file():
        print("check_wram_journal: wram.json or WRAM_JOURNAL.md is missing", file=sys.stderr)
        return 2
    text = JOURNAL.read_text(encoding="utf-8")
    if len(argv) > 1 and argv[1] == "--stats":
        days = int(argv[2]) if len(argv) > 2 else 30
        n, c = stats(text, days)
        print(f"wram.json: {n} rewrite(s) with a drift in the last {days} days, {c} catch(es)")
        return 0
    msg = mismatch(BASELINE.read_bytes(), text)
    if msg:
        print("check_wram_journal: " + msg)
        return 1
    print("OK: wram.json matches the last line of WRAM_JOURNAL.md")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
