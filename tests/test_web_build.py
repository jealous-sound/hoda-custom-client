from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build_web
import serve_web


FILE_SIZE = 100
SERVED_INDEX = "client-additional-data/resource_index.txt"


class ParseRangeTest(unittest.TestCase):
    def test_open_ended_range_runs_to_the_last_byte(self) -> None:
        self.assertEqual(serve_web.parse_range("bytes=0-", FILE_SIZE), (0, FILE_SIZE - 1))

    def test_closed_range_is_kept(self) -> None:
        self.assertEqual(serve_web.parse_range("bytes=10-19", FILE_SIZE), (10, 19))

    def test_range_past_the_end_is_clamped(self) -> None:
        self.assertEqual(serve_web.parse_range("bytes=90-500", FILE_SIZE), (90, FILE_SIZE - 1))

    def test_suffix_range_counts_from_the_end(self) -> None:
        self.assertEqual(serve_web.parse_range("bytes=-10", FILE_SIZE), (90, FILE_SIZE - 1))

    def test_range_starting_past_the_end_is_unsatisfiable(self) -> None:
        self.assertIsNone(serve_web.parse_range(f"bytes={FILE_SIZE}-", FILE_SIZE))

    def test_malformed_ranges_are_rejected(self) -> None:
        for header in ("bytes=-", "items=0-1", "bytes=a-b", "bytes=0-1,4-5"):
            with self.subTest(header=header):
                self.assertIsNone(serve_web.parse_range(header, FILE_SIZE))


class BuildWebTest(unittest.TestCase):
    def test_lists_links_and_index_the_page_mounts(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            root, out = base / "repo", base / "web"
            runtime = ["data/rules.json", "client-additional-data/characters.json"]
            for relative in runtime:
                (root / relative).parent.mkdir(parents=True, exist_ok=True)
                (root / relative).write_text(relative)
            out.mkdir()
            for name in build_web.PAGE_FILES:
                (out / name).write_text(name)
            index = base / "resource_index.txt"
            index.write_text("hoda-resource-index 1\n")
            with mock.patch.object(build_web.build_dist, "content_files", return_value=runtime):
                count, _ = build_web.build(root, out, index, SERVED_INDEX)

            listed = (out / build_web.FILE_LIST).read_text().splitlines()
            self.assertEqual(count, len(runtime) + 1)
            self.assertEqual(listed, sorted(runtime + [SERVED_INDEX]))
            tree = out / build_web.GAME_TREE
            for relative in runtime:
                self.assertTrue((tree / relative).is_symlink())
                self.assertEqual((tree / relative).read_text(), relative)
            self.assertEqual((tree / SERVED_INDEX).read_text(), index.read_text())

    def test_refuses_an_output_without_the_compiled_page(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            index = base / "resource_index.txt"
            index.write_text("hoda-resource-index 1\n")
            with self.assertRaises(SystemExit):
                build_web.build(base, base, index, SERVED_INDEX)


if __name__ == "__main__":
    unittest.main()
