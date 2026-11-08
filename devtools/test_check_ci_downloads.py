"""Unit tests of check_ci_downloads.py: the check must be seen failing."""
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import check_ci_downloads as c  # noqa: E402


class Faults(unittest.TestCase):
    def test_curl_without_retry_is_a_fault(self):
        self.assertEqual(len(c.faults_in("run: curl -fsSL -o x https://e/x\n")), 1)

    def test_curl_with_retry_passes(self):
        self.assertEqual(c.faults_in("run: curl -fsSL --retry 5 -o x https://e/x\n"), [])

    def test_wget_without_tries_is_a_fault(self):
        self.assertEqual(len(c.faults_in("wget https://e/x\n")), 1)
        self.assertEqual(c.faults_in("wget --tries=5 https://e/x\n"), [])

    def test_continued_command_is_read_as_one_line(self):
        text = "run: |\n  curl -fsSL \\\n    --retry 5 \\\n    -o x https://e/x\n"
        self.assertEqual(c.faults_in(text), [])
        text = "run: |\n  curl -fsSL \\\n    -o x https://e/x\n"
        self.assertEqual(len(c.faults_in(text)), 1)

    def test_marker_on_the_line_or_the_one_above(self):
        self.assertEqual(c.faults_in("curl -I u  # retried: the loop below\n"), [])
        self.assertEqual(c.faults_in("# retried: the loop below\ncurl -I u\n"), [])
        # a marker with no reason does not count
        self.assertEqual(len(c.faults_in("curl -I u  # retried:\n")), 1)

    def test_comments_and_words_are_not_commands(self):
        self.assertEqual(c.faults_in("# curl is used below\n"), [])
        self.assertEqual(c.faults_in("name: curlers\n"), [])
        self.assertEqual(c.faults_in("echo $CURL_BIN\n"), [])

    def test_main_on_a_tree_fails_then_passes(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            (root / ".github" / "workflows").mkdir(parents=True)
            (root / "scripts").mkdir()
            wf = root / ".github" / "workflows" / "a.yml"
            wf.write_text("run: curl -fsSL -o x https://e/x\n")
            self.assertEqual(c.main(["x", d]), 1)
            wf.write_text("run: curl -fsSL --retry 5 -o x https://e/x\n")
            self.assertEqual(c.main(["x", d]), 0)

    def test_empty_tree_is_an_error_not_a_pass(self):
        with tempfile.TemporaryDirectory() as d:
            self.assertEqual(c.main(["x", d]), 2)


if __name__ == "__main__":
    unittest.main()
