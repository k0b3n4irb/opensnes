#!/usr/bin/env python3
"""Unit tests for the luna harness's own parsing and decision logic.

The harness decides what counts as live, which frames are captured, how a
luna budget line is read and how manifest input scripts are merged. A bug
there passes or fails the whole corpus wrongly, and no ROM test would see
it. No luna binary needed: every input below is a luna output or a
manifest shape copied from a real run (luna v1.27.0, 2026-09-26).

    python3 -m unittest tools/luna-test/test_harness.py
"""
from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import luna_runner  # noqa: E402
import nmi_budget  # noqa: E402
import rom_coverage  # noqa: E402


def state(frames=200, nmis=198, stopped=False, last_nmi=None):
    sch = {"frame_count": frames, "nmis_serviced": nmis}
    if last_nmi is not None:
        sch["last_nmi_frame"] = last_nmi
    return {"scheduler": sch, "cpu": {"stopped": stopped}}


class Liveness(unittest.TestCase):
    def test_running_rom_is_live(self):
        live, why = luna_runner.liveness(state(last_nmi=200))
        self.assertTrue(live)
        self.assertIn("200f/198nmi", why)

    def test_stp_is_dead(self):
        self.assertFalse(luna_runner.liveness(state(stopped=True))[0])

    def test_no_frames_is_dead(self):
        self.assertFalse(luna_runner.liveness(state(frames=0, nmis=0))[0])

    def test_no_nmi_is_dead(self):
        live, why = luna_runner.liveness(state(nmis=0))
        self.assertFalse(live)
        self.assertIn("NMI", why)

    def test_latest_nmi_this_frame_is_live(self):
        st = state(200, 198)
        st["scheduler"]["last_nmi_frame"] = 200
        self.assertTrue(luna_runner.liveness(st)[0])

    def test_nmi_dead_after_boot(self):
        # The negative control of 2026-09-26: print_string with $4200 = 0
        # after 100 frames — 102 NMIs, the last one long ago.
        st = state(200, 102)
        st["scheduler"]["last_nmi_frame"] = 101
        live, why = luna_runner.liveness(st)
        self.assertFalse(live)
        self.assertIn("NMI died after boot", why)

    def test_no_nmi_frame_reported_is_dead(self):
        self.assertFalse(luna_runner.liveness(state(200, 198))[0])


class FramePoints(unittest.TestCase):
    def test_scalar_becomes_list(self):
        self.assertEqual(luna_runner.frame_points(120), [120])

    def test_list_kept_in_order(self):
        self.assertEqual(luna_runner.frame_points([60, 300]), [60, 300])

    def test_list_is_copied(self):
        points = [60, 300]
        luna_runner.frame_points(points).append(1)
        self.assertEqual(points, [60, 300])


class PoolMap(unittest.TestCase):
    """The luna runs go through a pool; the report must not depend on which
    run finishes first."""

    def test_order_kept_when_later_items_finish_first(self):
        import time
        def slow_first(i):
            time.sleep(0.02 * (5 - i))
            return i * 10
        with mock.patch.dict("os.environ", {"LUNA_JOBS": "5"}):
            self.assertEqual(luna_runner._pool_map(slow_first, range(5)),
                             [0, 10, 20, 30, 40])

    def test_serial_width(self):
        import threading
        seen = set()
        def who(_):
            seen.add(threading.get_ident())
        with mock.patch.dict("os.environ", {"LUNA_JOBS": "1"}):
            luna_runner._pool_map(who, range(8))
        self.assertEqual(len(seen), 1)


class BudgetLine(unittest.TestCase):
    # Verbatim `luna profile --budget` lines, v1.27.0.
    OK = "budget: NmiHandler max 8490 mclk (frame 30) <= 1000000000 — ok"
    OVER = "budget: NmiHandler max 8490 mclk (frame 30) > 10 — OVER"

    def test_ok_line(self):
        m = nmi_budget.BUDGET_RE.search(self.OK)
        self.assertEqual((m.group(1), m.group(2), m.group(3)),
                         ("NmiHandler", "8490", "30"))

    def test_over_line(self):
        m = nmi_budget.BUDGET_RE.search(self.OVER)
        self.assertEqual(int(m.group(2)), 8490)

    def test_line_among_profile_output(self):
        out = "entries: 0\n" + self.OK + "\ntotal_mclk 123\n"
        self.assertIsNotNone(nmi_budget.BUDGET_RE.search(out))

    def test_no_line(self):
        self.assertIsNone(nmi_budget.BUDGET_RE.search("error: unknown symbol"))

    def test_references_are_under_the_ceiling(self):
        for key, ref, _ in nmi_budget.SUBSET:
            self.assertLess(ref, nmi_budget.CEILING, key)


class ManifestRuns(unittest.TestCase):
    """rom_coverage.manifest_runs() replays a manifest's input exactly as
    `luna test` does: checkpoint scripts merge into one timeline."""

    def run_manifests(self, *manifests: str):
        with tempfile.TemporaryDirectory() as td:
            d = Path(td)
            rom = d / "game.sfc"
            rom.write_bytes(b"")
            for i, text in enumerate(manifests):
                (d / f"m{i}.toml").write_text(text, encoding="utf-8")
            with mock.patch.object(rom_coverage, "MANIFESTS", d):
                return rom_coverage.manifest_runs(rom)

    def test_checkpoint_scripts_merge_in_frame_order(self):
        runs = self.run_manifests('''
rom = "game.sfc"
input = "10:0x80"
[[checkpoint]]
at_frame = 200
input = "150:0x1000,160:0"
[[checkpoint]]
at_frame = 100
input = "50:0x100,60:0"
''')
        self.assertEqual(len(runs), 1)
        name, flags, script = runs[0]
        self.assertEqual(name, "m0")
        self.assertEqual(flags, ["--until-frame", "200"])
        self.assertEqual(script, "10:0x80,50:0x100,60:0,150:0x1000,160:0")

    def test_frames_key_wins_over_checkpoints(self):
        runs = self.run_manifests('''
rom = "game.sfc"
frames = 500
[[checkpoint]]
at_frame = 100
''')
        self.assertEqual(runs[0][1], ["--until-frame", "500"])

    def test_steps_bound(self):
        runs = self.run_manifests('rom = "game.sfc"\nsteps = 1500000\n')
        self.assertEqual(runs[0][1], ["-n", "1500000"])
        self.assertIsNone(runs[0][2])

    def test_unbounded_manifest_is_skipped(self):
        self.assertEqual(self.run_manifests('rom = "game.sfc"\n'), [])

    def test_other_rom_is_skipped(self):
        self.assertEqual(self.run_manifests('rom = "other.sfc"\nframes = 10\n'), [])

    def test_peripherals_become_luna_flags(self):
        runs = self.run_manifests('''
rom = "game.sfc"
frames = 300
input2 = "20:0x80"
mouse = "40:5,0,1;30:1,1,0"
superscope = "90:128,112,1"
''')
        flags = runs[0][1]
        self.assertEqual(flags[:2], ["--until-frame", "300"])
        self.assertIn("--input2", flags)
        self.assertEqual(flags[flags.index("--input2") + 1], "20:0x80")
        i = flags.index("--mouse")
        self.assertEqual(flags[i - 2:i], ["--port1", "mouse"])
        self.assertEqual(flags[i + 1], "30:1,1,0;40:5,0,1")   # sorted by frame
        j = flags.index("--superscope")
        self.assertEqual(flags[j - 2:j], ["--port2", "superscope"])

    def test_broken_toml_is_skipped(self):
        self.assertEqual(self.run_manifests('rom = "game.sfc"\nframes = [\n'), [])


if __name__ == "__main__":
    unittest.main()
