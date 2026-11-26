from __future__ import annotations

from functools import partial
import http.client
import json
from pathlib import Path
import re
import struct
import sys
import tempfile
import threading
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build_web
import serve_web


FILE_SIZE = 100
SERVED_INDEX = "client-additional-data/resource_index.txt"
WEB_PACKAGING = ROOT / "packaging" / "web"
PNG_SIZE_FIELDS = slice(16, 24)


def png_size(path: Path) -> str:
    width, height = struct.unpack(">II", path.read_bytes()[PNG_SIZE_FIELDS])
    return f"{width}x{height}"


class AppFilesTest(unittest.TestCase):
    def test_manifest_icons_ship_at_the_sizes_they_claim(self) -> None:
        manifest = json.loads((WEB_PACKAGING / "manifest.json").read_text())
        self.assertEqual((manifest["display"], manifest["orientation"]), ("fullscreen", "landscape"))
        for icon in manifest["icons"]:
            self.assertIn(icon["src"], build_web.APP_FILES)
            self.assertEqual(png_size(WEB_PACKAGING / icon["src"]), icon["sizes"])

    def test_the_page_links_only_app_files_the_build_copies(self) -> None:
        linked = re.findall(r'<link [^>]*href="([^"]+)"', (WEB_PACKAGING / "shell.html").read_text())
        self.assertTrue(linked)
        self.assertLessEqual(set(linked), set(build_web.APP_FILES))
        makefile = (ROOT / "Makefile").read_text().replace("\\\n", " ")
        copied = re.search(r"^WEB_APP_FILES = (.+)$", makefile, re.MULTILINE).group(1).split()
        self.assertEqual([Path(path).name for path in copied], list(build_web.APP_FILES))
        self.assertEqual(png_size(WEB_PACKAGING / "icon-180.png"), "180x180")


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


class ServeWebTest(unittest.TestCase):
    PAGE_ORIGIN = "http://localhost:8080"

    def serve(self, allowed_origin: str) -> None:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        (root / "index.html").write_text("page")
        (root / "game" / "data").mkdir(parents=True)
        (root / "game" / "data" / "rules.json").write_text("0123456789")
        handler = type(
            "QuietHandler",
            (serve_web.GameHandler,),
            {"allowed_origin": allowed_origin, "stats": serve_web.Stats(), "log_message": lambda *_: None},
        )
        server = serve_web.ThreadingHTTPServer(("127.0.0.1", 0), partial(handler, directory=str(root)))
        threading.Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        self.port = server.server_address[1]

    def ask(self, method: str, path: str, headers: dict[str, str] | None = None) -> http.client.HTTPResponse:
        connection = http.client.HTTPConnection("127.0.0.1", self.port)
        self.addCleanup(connection.close)
        connection.request(method, path, headers=headers or {})
        response = connection.getresponse()
        response.read()
        return response

    def test_page_is_isolated_and_doubled_slashes_miss_like_on_the_cdn(self) -> None:
        self.serve("")
        found = self.ask("GET", "/game/data/rules.json")
        self.assertEqual(found.status, 200)
        self.assertEqual(found.getheader("Cross-Origin-Embedder-Policy"), "require-corp")
        self.assertEqual(self.ask("HEAD", "/game/data//rules.json").status, 404)
        self.assertEqual(self.ask("GET", "/game//data/rules.json").status, 404)

    def test_cdn_answers_ranged_head_with_cors_for_the_page_origin(self) -> None:
        self.serve(self.PAGE_ORIGIN)
        reply = self.ask("HEAD", "/game/data/rules.json", {"Origin": self.PAGE_ORIGIN, "Range": "bytes=0-"})
        self.assertEqual(reply.status, 206)
        self.assertEqual(reply.getheader("Access-Control-Allow-Origin"), self.PAGE_ORIGIN)
        self.assertIn("Accept-Ranges", reply.getheader("Access-Control-Expose-Headers"))
        self.assertIsNone(reply.getheader("Cross-Origin-Embedder-Policy"))
        stranger = self.ask("GET", "/game/data/rules.json", {"Origin": "https://elsewhere.test"})
        self.assertIsNone(stranger.getheader("Access-Control-Allow-Origin"))

    def test_cdn_preflight_allows_only_ranged_reads_from_the_page_origin(self) -> None:
        self.serve(self.PAGE_ORIGIN)
        ranged = {"Origin": self.PAGE_ORIGIN, "Access-Control-Request-Method": "HEAD"}
        allowed = self.ask("OPTIONS", "/game/data/rules.json", {**ranged, "Access-Control-Request-Headers": "range"})
        self.assertEqual(allowed.status, 204)
        self.assertEqual(allowed.getheader("Access-Control-Allow-Origin"), self.PAGE_ORIGIN)
        self.assertIn("HEAD", allowed.getheader("Access-Control-Allow-Methods"))
        refused = [
            {**ranged, "Access-Control-Request-Headers": "authorization"},
            {**ranged, "Access-Control-Request-Method": "PUT"},
            {**ranged, "Origin": "https://elsewhere.test"},
        ]
        for headers in refused:
            with self.subTest(headers=headers):
                self.assertEqual(self.ask("OPTIONS", "/game/data/rules.json", headers).status, 403)


if __name__ == "__main__":
    unittest.main()
