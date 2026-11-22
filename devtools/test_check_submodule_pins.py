"""Unit test of check_submodule_pins.py --coherent: the bound exempts what predates the check, nothing after."""
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import check_submodule_pins as c  # noqa: E402

A, B, C = "a" * 40, "b" * 40, "c" * 40


def run(repo, *args):
    return subprocess.run(["git", "-c", "user.name=t", "-c", "user.email=t@t", *args], cwd=repo,
                          capture_output=True, text=True, check=True).stdout.strip()


def commit(repo, pin, table_pin, msg):
    """One commit: the submodule gitlink at `pin`, PINS.md saying `table_pin`."""
    run(repo, "update-index", "--add", "--cacheinfo", f"160000,{pin},compiler/qbe")
    (repo / "compiler").mkdir(exist_ok=True)
    (repo / "compiler/PINS.md").write_text(f"| compiler/qbe | {table_pin[:12]} |\n")
    run(repo, "add", "compiler/PINS.md")
    run(repo, "commit", "-q", "-m", msg)
    return run(repo, "rev-parse", "HEAD")


class Coherent(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name)
        run(self.repo, "init", "-q")
        self.before = commit(self.repo, B, A, "before the check: pin and PINS.md disagree")
        self.bound = commit(self.repo, B, B, "the bound")
        self.good = commit(self.repo, C, C, "after, agreeing")

    def tearDown(self):
        self.tmp.cleanup()

    def judge(self, since):
        return c.coherent("HEAD", since=since, repo=self.repo)

    def test_commit_before_the_bound_passes(self):
        self.assertEqual(self.judge(self.bound), 0)

    def test_without_the_bound_it_is_seen(self):
        self.assertEqual(self.judge("0" * 40), 1)

    def test_faulty_commit_after_the_bound_fails(self):
        commit(self.repo, A, C, "after, disagreeing")
        self.assertEqual(self.judge(self.bound), 1)


if __name__ == "__main__":
    unittest.main()
