from __future__ import annotations

import hashlib
from pathlib import Path
import sys
import tempfile
import unittest
from urllib.parse import urljoin


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests"))

import build_web
from fake_s3 import FakeS3
import publish_web


EXAMPLE_ACCESS_KEY = "AKIAIOSFODNN7EXAMPLE"
EXAMPLE_S3_SECRET = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY"
EXAMPLE_IAM_SECRET = "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY"
EXAMPLE_REGION = "us-east-1"
BUCKET = "hoda-assets"
CONTENT_URL = "https://cdn.example.test"
GAME_FILES = {
    "client-additional-data/characters.json": b'{"heroes": []}',
    "client-additional-data/meshes/a/model.glb": b"glTF" + bytes(range(256)),
    "data/rules.json": b'{"rules": 1}',
}
CHANGED_RULES = b'{"rules": 2}'
SMALL_PAGE = 2
WORKERS = 2


def md5(body: bytes) -> str:
    return hashlib.md5(body, usedforsecurity=False).hexdigest()


def write_build(root: Path, release: str, files: dict[str, bytes], content_url: str = CONTENT_URL) -> Path:
    out = root / "web"
    objects = out / build_web.OBJECTS_TREE
    objects.mkdir(parents=True, exist_ok=True)
    for body in files.values():
        (objects / md5(body)).write_bytes(body)
    digests = {relative: md5(body) for relative, body in files.items()}
    sizes = {md5(body): len(body) for body in files.values()}
    (out / build_web.FILE_LIST).write_text(build_web.file_list(digests, build_web.LOCAL_OBJECTS, sizes))
    for name in build_web.APP_FILES:
        (out / name).write_bytes(name.encode())
    (out / "index.html").write_text("<script src=index.js></script>")
    (out / "index.js").write_text("var Module = {};")
    (out / "index.wasm").write_bytes(b"\0asm" + publish_web.files_url(content_url, release).encode())
    return out


class SigningTest(unittest.TestCase):
    def test_s3_get_object_example_signs_as_documented(self) -> None:
        headers = {
            "Host": "examplebucket.s3.amazonaws.com",
            "Range": "bytes=0-9",
            "x-amz-content-sha256": publish_web.EMPTY_PAYLOAD_SHA256,
            "x-amz-date": "20130524T000000Z",
        }
        request = publish_web.canonical_request("GET", "/test.txt", {}, headers, publish_web.EMPTY_PAYLOAD_SHA256)
        self.assertEqual(
            hashlib.sha256(request.encode()).hexdigest(),
            "7344ae5b7ee6c3e7e6b0fe0640412a37625d1fbfff95c48bbb2dc43964946972",
        )
        header = publish_web.authorization(
            EXAMPLE_ACCESS_KEY,
            EXAMPLE_S3_SECRET,
            EXAMPLE_REGION,
            "s3",
            "20130524T000000Z",
            request,
            "host;range;x-amz-content-sha256;x-amz-date",
        )
        self.assertTrue(header.endswith("Signature=f0e8bdb87c964420e857bd35b5d6ed310bd44f0170aba48dd91039c6036bdb41"))
        self.assertIn(f"Credential={EXAMPLE_ACCESS_KEY}/20130524/us-east-1/s3/aws4_request", header)

    def test_signing_key_matches_the_documented_derivation(self) -> None:
        self.assertEqual(
            publish_web.signing_key(EXAMPLE_IAM_SECRET, "20150830", EXAMPLE_REGION, "iam").hex(),
            "c4afb1cc5771d871763a393e44b703571b55cc28424d1a5e86da6ed3c154a4b9",
        )

    def test_query_is_sorted_and_slashes_in_values_are_encoded(self) -> None:
        self.assertEqual(
            publish_web.canonical_query({"prefix": "web/v1/", "list-type": "2"}), "list-type=2&prefix=web%2Fv1%2F"
        )


class PublishTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.base = Path(self.temporary.name)
        self.s3 = FakeS3(self.base / "bucket", BUCKET, page_size=SMALL_PAGE).__enter__()
        self.bucket = publish_web.R2Bucket(self.s3.endpoint, BUCKET, "key", "secret")

    def tearDown(self) -> None:
        self.s3.__exit__()
        self.temporary.cleanup()

    def build(self, release: str, files: dict[str, bytes], previous: str = "", reuse: bool = False):
        out = write_build(self.base / release, release, files)
        return publish_web.read_release(out, release, previous, CONTENT_URL, reuse_game_files=reuse)

    def publish(self, release: str, files: dict[str, bytes], previous: str = "", dry_run: bool = False):
        return publish_web.publish(self.bucket, self.build(release, files, previous), dry_run, workers=WORKERS)

    def written(self) -> list[tuple[str, str]]:
        written, self.s3.writes[:] = list(self.s3.writes), []
        return written

    def published_list(self, release: str) -> tuple[dict[str, str], dict[str, int]]:
        listed = self.s3.stored(publish_web.release_key(release, build_web.FILE_LIST)).read_text()
        return build_web.read_list(listed, publish_web.PUBLISHED_OBJECTS)

    def urls(self, release: str) -> dict[str, str]:
        base = publish_web.files_url(CONTENT_URL, release)
        digests, _ = self.published_list(release)
        return {relative: urljoin(base, f"{publish_web.PUBLISHED_OBJECTS}{digest}") for relative, digest in digests.items()}

    def publish_legacy_release(self, release: str, files: dict[str, bytes]) -> None:
        for relative, body in files.items():
            stored = self.s3.stored(publish_web.release_key(release, f"{publish_web.LEGACY_GAME_TREE}/{relative}"))
            stored.parent.mkdir(parents=True, exist_ok=True)
            stored.write_bytes(body)
        folder = self.s3.stored(publish_web.release_key(release, ""))
        (folder / build_web.FILE_LIST).write_text("".join(f"{relative}\n" for relative in sorted(files)))
        hashes = "".join(f"{md5(files[relative])}  {relative}\n" for relative in sorted(files))
        (folder / publish_web.LEGACY_HASH_LIST).write_text(hashes)

    def test_first_release_uploads_objects_then_list_then_page(self) -> None:
        plan, _ = self.publish("v1", GAME_FILES)

        objects = sorted(md5(body) for body in GAME_FILES.values())
        self.assertEqual(sorted(plan.upload), objects)
        keys = [key for _, key in self.written()]
        self.assertCountEqual(keys[: len(objects)], [f"web/objects/{digest}" for digest in objects])
        endings = ["game_files.txt", "index.wasm", "index.js", *build_web.APP_FILES, "index.html"]
        self.assertEqual(keys[len(objects) :], [f"web/v1/{name}" for name in endings])
        for relative, body in GAME_FILES.items():
            self.assertEqual(self.s3.stored(f"web/objects/{md5(body)}").read_bytes(), body)

    def test_every_listed_location_resolves_to_its_object_on_the_cdn(self) -> None:
        self.publish("v1", GAME_FILES)

        for relative, url in self.urls("v1").items():
            self.assertEqual(url, f"{CONTENT_URL}/web/objects/{md5(GAME_FILES[relative])}")

    def test_the_published_list_gives_each_object_the_size_make_web_wrote(self) -> None:
        out = write_build(self.base / "v1", "v1", GAME_FILES)
        digests = {relative: md5(body) for relative, body in GAME_FILES.items()}
        (out / build_web.FILE_LIST).write_text(build_web.file_list(digests, build_web.LOCAL_OBJECTS, {}))
        built = publish_web.read_release(out, "v1", "", CONTENT_URL)

        publish_web.publish(self.bucket, built, dry_run=False, workers=WORKERS)

        _, sizes = self.published_list("v1")
        self.assertEqual(sizes, {md5(body): len(body) for body in GAME_FILES.values()})

    def test_objects_keep_the_type_their_path_names_and_never_expire(self) -> None:
        self.publish("v1", GAME_FILES)

        stored = self.s3.stored_headers(f"web/objects/{md5(GAME_FILES['data/rules.json'])}")
        self.assertEqual(stored["Content-Type"], "application/json")
        self.assertEqual(stored["Cache-Control"], publish_web.IMMUTABLE_CACHE_CONTROL)

    def test_release_that_changes_no_game_file_keeps_every_url_and_uploads_only_list_and_page(self) -> None:
        self.publish("v1", GAME_FILES)
        self.written()

        plan, sent = self.publish("v2", GAME_FILES, previous="v1")

        self.assertEqual((plan.upload, plan.copy, sent), ([], {}, 0))
        self.assertEqual(self.urls("v2"), self.urls("v1"))
        self.assertTrue(all(key.startswith("web/v2/") for _, key in self.written()))

    def test_next_release_uploads_only_changed_bytes_and_keeps_the_other_urls(self) -> None:
        self.publish("v1", GAME_FILES)
        self.written()
        changed = {**GAME_FILES, "data/rules.json": CHANGED_RULES, "data/new.json": b"{}"}

        plan, _ = self.publish("v2", changed)

        self.assertEqual(sorted(plan.upload), sorted([md5(CHANGED_RULES), md5(b"{}")]))
        before, after = self.urls("v1"), self.urls("v2")
        for relative in ("client-additional-data/characters.json", "client-additional-data/meshes/a/model.glb"):
            self.assertEqual(after[relative], before[relative])
        self.assertNotEqual(after["data/rules.json"], before["data/rules.json"])
        self.assertEqual(
            self.s3.stored(f"web/objects/{md5(GAME_FILES['data/rules.json'])}").read_bytes(), b'{"rules": 1}'
        )

    def test_identical_bytes_at_two_paths_are_one_object(self) -> None:
        plan, _ = self.publish("v1", {**GAME_FILES, "data/rules-copy.json": GAME_FILES["data/rules.json"]})

        self.assertEqual(len(plan.upload), len(GAME_FILES))
        urls = self.urls("v1")
        self.assertEqual(urls["data/rules-copy.json"], urls["data/rules.json"])

    def test_rerun_of_a_finished_release_writes_nothing(self) -> None:
        self.publish("v1", GAME_FILES)
        self.written()

        plan, sent = self.publish("v1", GAME_FILES)

        self.assertEqual((plan.upload, plan.copy, plan.endings, sent), ([], {}, [], 0))
        self.assertEqual(len(plan.present), len(GAME_FILES))
        self.assertEqual(self.written(), [])

    def test_an_object_holding_other_bytes_stops_the_run_before_any_write(self) -> None:
        tampered = self.s3.stored(f"web/objects/{md5(GAME_FILES['data/rules.json'])}")
        tampered.parent.mkdir(parents=True)
        tampered.write_bytes(CHANGED_RULES)

        with self.assertRaises(SystemExit):
            self.publish("v1", GAME_FILES)
        self.assertEqual(self.written(), [])

    def test_a_file_changed_after_the_build_listed_it_is_not_uploaded(self) -> None:
        built = self.build("v1", GAME_FILES)
        (built.root / build_web.OBJECTS_TREE / md5(GAME_FILES["data/rules.json"])).write_bytes(CHANGED_RULES)

        with self.assertRaises(SystemExit):
            publish_web.publish(self.bucket, built, dry_run=False, workers=WORKERS)
        self.assertFalse(self.s3.stored(f"web/objects/{md5(GAME_FILES['data/rules.json'])}").exists())
        self.assertFalse(self.s3.stored("web/v1/game_files.txt").exists())

    def test_published_release_never_takes_other_bytes(self) -> None:
        self.publish("v1", GAME_FILES)
        self.written()

        with self.assertRaises(SystemExit):
            self.publish("v1", {**GAME_FILES, "data/rules.json": CHANGED_RULES})
        self.assertEqual(self.written(), [])

    def test_rebuilt_page_under_a_published_release_is_refused_before_any_write(self) -> None:
        self.publish("v1", GAME_FILES)
        self.s3.stored(f"web/objects/{md5(GAME_FILES['data/rules.json'])}").unlink()
        self.written()
        (self.base / "v1" / "web" / "index.js").write_text("var Module = {changed: true};")
        built = publish_web.read_release(self.base / "v1" / "web", "v1", "", CONTENT_URL)

        with self.assertRaises(SystemExit):
            publish_web.publish(self.bucket, built, dry_run=False, workers=WORKERS)
        self.assertEqual(self.written(), [])

    def test_release_after_a_legacy_one_copies_its_game_tree_into_objects(self) -> None:
        self.publish_legacy_release("v0", GAME_FILES)
        changed = {**GAME_FILES, "data/rules.json": CHANGED_RULES}

        plan, sent = self.publish("v1", changed, previous="v0")

        self.assertEqual(plan.upload, [md5(CHANGED_RULES)])
        self.assertEqual(sent, len(CHANGED_RULES))
        self.assertEqual(
            plan.copy[md5(GAME_FILES["client-additional-data/characters.json"])],
            "web/v0/game/client-additional-data/characters.json",
        )
        actions = dict((key, action) for action, key in self.written())
        self.assertEqual(actions[f"web/objects/{md5(GAME_FILES['client-additional-data/characters.json'])}"], "copy")
        self.assertEqual(actions[f"web/objects/{md5(CHANGED_RULES)}"], "upload")
        for relative, url in self.urls("v1").items():
            self.assertEqual(url, f"{CONTENT_URL}/web/objects/{md5(changed[relative])}")

    def test_a_legacy_copy_whose_source_is_gone_is_uploaded_instead(self) -> None:
        self.publish_legacy_release("v0", GAME_FILES)
        self.s3.stored("web/v0/game/data/rules.json").unlink()

        self.publish("v1", GAME_FILES, previous="v0")

        self.assertIn(("upload", f"web/objects/{md5(GAME_FILES['data/rules.json'])}"), self.written())

    def test_summary_names_only_what_this_run_publishes(self) -> None:
        built = self.build("v1", GAME_FILES)
        plan, sent = publish_web.publish(self.bucket, built, dry_run=False, workers=WORKERS)
        report = publish_web.summary(built, plan, sent, dry_run=False)
        self.assertIn("3 game files in 3 objects under web/objects/; uploaded 3", report)
        self.assertIn(
            "published game_files.txt, index.wasm, index.js, manifest.json, icon-180.png, icon-192.png, icon-512.png, "
            "index.html last",
            report,
        )

        plan, sent = publish_web.publish(self.bucket, built, dry_run=False, workers=WORKERS)
        self.assertIn(
            "3 already there; list and page were already there", publish_web.summary(built, plan, sent, False)
        )

    def test_dry_run_plans_without_writing(self) -> None:
        plan, sent = self.publish("v1", GAME_FILES, dry_run=True)

        self.assertEqual(len(plan.upload), len(GAME_FILES))
        self.assertEqual(sent, sum(len(body) for body in GAME_FILES.values()))
        self.assertEqual(self.written(), [])

    def test_previous_release_must_be_published(self) -> None:
        with self.assertRaises(SystemExit):
            self.publish("v2", GAME_FILES, previous="v1")

    def test_page_only_release_keeps_every_url_and_ignores_the_local_cache(self) -> None:
        self.publish("v1", GAME_FILES)
        self.written()
        changed_locally = {**GAME_FILES, "data/rules.json": CHANGED_RULES, "data/new.json": b"{}"}
        built = self.build("v2", changed_locally, previous="v1", reuse=True)

        plan, sent = publish_web.publish(self.bucket, built, dry_run=False, workers=WORKERS)

        self.assertEqual((plan.upload, plan.copy, sent), ([], {}, 0))
        self.assertTrue(all(key.startswith("web/v2/") for _, key in self.written()))
        self.assertEqual(
            self.s3.stored("web/v2/game_files.txt").read_bytes(), self.s3.stored("web/v1/game_files.txt").read_bytes()
        )
        self.assertIn("3 game files in 3 objects", publish_web.summary(built, plan, sent, dry_run=False))
        self.assertEqual(self.s3.stored("web/v2/index.js").read_bytes(), b"var Module = {};")

    def test_page_only_release_after_a_legacy_one_moves_its_game_files_into_objects(self) -> None:
        self.publish_legacy_release("v0", GAME_FILES)
        built = self.build("v1", {}, previous="v0", reuse=True)

        plan, sent = publish_web.publish(self.bucket, built, dry_run=False, workers=WORKERS)

        self.assertEqual((plan.upload, sent, len(plan.copy)), ([], 0, len(GAME_FILES)))
        self.assertEqual(self.published_list("v1")[1], {})
        for relative, url in self.urls("v1").items():
            self.assertEqual(url, f"{CONTENT_URL}/web/objects/{md5(GAME_FILES[relative])}")
            self.assertEqual(
                self.s3.stored(f"web/objects/{md5(GAME_FILES[relative])}").read_bytes(), GAME_FILES[relative]
            )

    def test_page_only_release_stops_when_a_kept_object_is_gone(self) -> None:
        self.publish("v1", GAME_FILES)
        self.s3.stored(f"web/objects/{md5(GAME_FILES['data/rules.json'])}").unlink()
        built = self.build("v2", GAME_FILES, previous="v1", reuse=True)

        with self.assertRaises(SystemExit):
            publish_web.publish(self.bucket, built, dry_run=False, workers=WORKERS)
        self.assertFalse(self.s3.stored("web/v2/game_files.txt").exists())

    def test_page_only_release_stops_when_a_legacy_source_is_gone(self) -> None:
        self.publish_legacy_release("v0", GAME_FILES)
        self.s3.stored("web/v0/game/data/rules.json").unlink()
        built = self.build("v1", GAME_FILES, previous="v0", reuse=True)

        with self.assertRaises(SystemExit):
            publish_web.publish(self.bucket, built, dry_run=False, workers=WORKERS)
        self.assertFalse(self.s3.stored("web/v1/game_files.txt").exists())

    def test_page_only_release_needs_a_previous_release(self) -> None:
        with self.assertRaises(SystemExit):
            self.build("v2", GAME_FILES, reuse=True)


class ReadReleaseTest(unittest.TestCase):
    def test_page_built_for_another_release_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            out = write_build(Path(temporary), "v1", GAME_FILES)
            with self.assertRaises(SystemExit):
                publish_web.read_release(out, "v2", "", CONTENT_URL)

    def test_release_names_cannot_leave_their_folder_or_take_the_objects_one(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            for name in ("", "../manifest", "v1/x", ".hidden", build_web.OBJECTS_TREE):
                with self.subTest(name=name):
                    out = write_build(Path(temporary), name, GAME_FILES)
                    with self.assertRaises(SystemExit):
                        publish_web.read_release(out, name, "", CONTENT_URL)

    def test_a_listed_object_missing_from_the_build_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            out = write_build(Path(temporary), "v1", GAME_FILES)
            (out / build_web.OBJECTS_TREE / md5(GAME_FILES["data/rules.json"])).unlink()
            with self.assertRaises(SystemExit):
                publish_web.read_release(out, "v1", "", CONTENT_URL)

    def test_files_url_matches_the_makefile_layout(self) -> None:
        self.assertEqual(
            publish_web.files_url("https://cdn.hodareborn.com/", "v0.31.0"), "https://cdn.hodareborn.com/web/v0.31.0/"
        )

    def test_published_objects_sit_beside_the_release_folders(self) -> None:
        self.assertEqual(
            urljoin(publish_web.files_url(CONTENT_URL, "v1"), f"{publish_web.PUBLISHED_OBJECTS}abc"),
            f"{CONTENT_URL}/{publish_web.object_key('abc')}",
        )


if __name__ == "__main__":
    unittest.main()
