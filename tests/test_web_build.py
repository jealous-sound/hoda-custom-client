from __future__ import annotations

from functools import partial
import hashlib
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
import publish_web
import serve_web


FILE_SIZE = 100
SERVED_INDEX = "client-additional-data/resource_index.txt"
SHIPPED_BUNDLE = "cache/a/CAB-hud"
DROPPED_BUNDLE = "cache/b/CAB-c_hero_t1_pre"
LATER_BUNDLE = "cache/c/CAB-store"
FULL_INDEX = (
    "hoda-resource-index 1\n"
    f"bundles 3\n{SHIPPED_BUNDLE}\n{DROPPED_BUNDLE}\n{LATER_BUNDLE}\n"
    "objects 4\n0 100 20 28 Btn_Close\n1 200 30 28 hero skin (1)\n2 300 40 83 click\n1 400 50 28 Btn_Close\n"
)
SHIPPED_INDEX = (
    "hoda-resource-index 1\n"
    f"bundles 2\n{SHIPPED_BUNDLE}\n{LATER_BUNDLE}\n"
    "objects 2\n0 100 20 28 Btn_Close\n1 300 40 83 click\n"
)
WEB_PACKAGING = ROOT / "packaging" / "web"
PNG_SIZE_FIELDS = slice(16, 24)
WINDOW_SOURCE = "window.c"


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


class WholeFramesTest(unittest.TestCase):
    def test_every_screen_draws_its_frames_through_the_window(self) -> None:
        raylib_frames = re.compile(r"\b(?:Begin|End)Drawing\(")
        bypassing = sorted(
            path.name
            for path in (ROOT / "source").glob("*.c")
            if path.name != WINDOW_SOURCE and raylib_frames.search(path.read_text(errors="replace"))
        )
        self.assertEqual(bypassing, [])
        self.assertTrue(raylib_frames.search((ROOT / "source" / WINDOW_SOURCE).read_text()))


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


class ShippedIndexTest(unittest.TestCase):
    def test_drops_bundles_that_do_not_ship_and_renumbers_the_rest(self) -> None:
        self.assertEqual(build_web.shipped_index(FULL_INDEX, {SHIPPED_BUNDLE, LATER_BUNDLE}), SHIPPED_INDEX)

    def test_keeps_an_index_whose_bundles_all_ship(self) -> None:
        shipped = {SHIPPED_BUNDLE, DROPPED_BUNDLE, LATER_BUNDLE}
        self.assertEqual(build_web.shipped_index(FULL_INDEX, shipped), FULL_INDEX)

    def test_refuses_text_that_is_not_an_index(self) -> None:
        for text in ("", "hoda-resource-index 1\n", "hoda-resource-index 1\nbundles 2\nonly-one\n"):
            with self.subTest(text=text), self.assertRaises(SystemExit):
                build_web.shipped_index(text, set())


def md5(text: str) -> str:
    return hashlib.md5(text.encode(), usedforsecurity=False).hexdigest()


class BuildWebTest(unittest.TestCase):
    RUNTIME = ["data/rules.json", "client-additional-data/characters.json", SHIPPED_BUNDLE, LATER_BUNDLE]
    TWIN = "data/rules-twin.json"

    def build(self, base: Path) -> tuple[Path, int]:
        root, out = base / "repo", base / "web"
        for relative in self.RUNTIME:
            (root / relative).parent.mkdir(parents=True, exist_ok=True)
            (root / relative).write_text(relative)
        (root / self.TWIN).write_text("data/rules.json")
        out.mkdir(exist_ok=True)
        for name in build_web.PAGE_FILES:
            (out / name).write_text(name)
        index = base / "resource_index.txt"
        index.write_text(FULL_INDEX)
        with mock.patch.object(build_web.build_dist, "content_files", return_value=[*self.RUNTIME, self.TWIN]):
            count, _ = build_web.build(root, out, index, SERVED_INDEX, workers=2)
        return out, count

    def test_lists_each_file_the_page_mounts_beside_the_object_holding_its_bytes(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            out, count = self.build(Path(temporary))

            listed = (out / build_web.FILE_LIST).read_text().splitlines()
            mounted = sorted([*self.RUNTIME, self.TWIN, SERVED_INDEX])
            self.assertEqual(count, len(mounted))
            self.assertEqual([line.partition(build_web.LIST_SEPARATOR)[2] for line in listed], mounted)
            digests = build_web.list_digests("\n".join(listed), build_web.LOCAL_OBJECTS)
            for relative in self.RUNTIME:
                self.assertEqual(digests[relative], md5(relative))
                self.assertTrue((out / build_web.OBJECTS_TREE / md5(relative)).is_symlink())
                self.assertEqual((out / build_web.LOCAL_OBJECTS / digests[relative]).read_text(), relative)
            self.assertEqual((out / build_web.LOCAL_OBJECTS / digests[SERVED_INDEX]).read_text(), SHIPPED_INDEX)

    def test_identical_files_share_one_object(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            out, _ = self.build(Path(temporary))

            digests = build_web.list_digests((out / build_web.FILE_LIST).read_text(), build_web.LOCAL_OBJECTS)
            self.assertEqual(digests[self.TWIN], digests["data/rules.json"])
            self.assertEqual(len(list((out / build_web.OBJECTS_TREE).iterdir())), len(self.RUNTIME) + 1)

    def test_a_rebuild_drops_objects_no_file_holds_any_more(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            out, _ = self.build(Path(temporary))
            stale = out / build_web.OBJECTS_TREE / md5("gone")
            stale.write_text("gone")

            out, _ = self.build(Path(temporary))

            self.assertFalse(stale.exists())

    def test_refuses_an_output_without_the_compiled_page(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            index = base / "resource_index.txt"
            index.write_text("hoda-resource-index 1\n")
            with self.assertRaises(SystemExit):
                build_web.build(base, base, index, SERVED_INDEX)


class FileListTest(unittest.TestCase):
    DIGESTS = {"data/rules.json": md5("rules"), "data/with space.json": md5("space")}

    def test_reads_back_what_it_writes(self) -> None:
        for objects in (build_web.LOCAL_OBJECTS, "../objects/"):
            with self.subTest(objects=objects):
                text = build_web.file_list(self.DIGESTS, objects)
                self.assertEqual(build_web.list_digests(text, objects), self.DIGESTS)

    def test_lines_are_sorted_by_path_with_the_location_first(self) -> None:
        self.assertEqual(
            build_web.file_list(self.DIGESTS, "objects/").splitlines(),
            [f"objects/{md5('rules')}  data/rules.json", f"objects/{md5('space')}  data/with space.json"],
        )

    def test_refuses_lines_that_name_no_object(self) -> None:
        for line in ("data/rules.json", f"objects/{md5('rules')}  ", f"elsewhere/{md5('rules')}  a", "objects/xyz  a"):
            with self.subTest(line=line), self.assertRaises(SystemExit):
                build_web.list_digests(line, build_web.LOCAL_OBJECTS)


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
        self.assertEqual(found.getheader("Cache-Control"), serve_web.LOCAL_CACHE_CONTROL)
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

    def test_cdn_files_carry_the_cache_control_the_publisher_stores_and_misses_do_not(self) -> None:
        self.serve(self.PAGE_ORIGIN)
        found = self.ask("GET", "/game/data/rules.json", {"Origin": self.PAGE_ORIGIN})
        self.assertEqual(found.getheader("Cache-Control"), publish_web.IMMUTABLE_CACHE_CONTROL)
        missing = self.ask("GET", "/game/data/absent.json", {"Origin": self.PAGE_ORIGIN})
        self.assertEqual(missing.status, 404)
        self.assertIsNone(missing.getheader("Cache-Control"))

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
