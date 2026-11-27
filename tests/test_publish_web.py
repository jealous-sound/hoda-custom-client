from __future__ import annotations

import hashlib
from pathlib import Path
import sys
import tempfile
import unittest


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
SMALL_PAGE = 2


def write_build(root: Path, release: str, files: dict[str, bytes], content_url: str = CONTENT_URL) -> Path:
    out = root / "web"
    for relative, body in files.items():
        target = out / build_web.GAME_TREE / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(body)
    (out / build_web.FILE_LIST).write_text(
        "".join(build_web.list_line(relative, len(files[relative])) for relative in sorted(files))
    )
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

    def publish(self, release: str, files: dict[str, bytes], previous: str = "", dry_run: bool = False):
        out = write_build(self.base / release, release, files)
        built = publish_web.read_release(out, release, previous, CONTENT_URL, workers=2)
        return publish_web.publish(self.bucket, built, dry_run, workers=2)

    def written(self) -> list[tuple[str, str]]:
        written, self.s3.writes[:] = list(self.s3.writes), []
        return written

    def test_first_release_uploads_game_files_then_lists_then_page(self) -> None:
        plan, _ = self.publish("v1", GAME_FILES)

        self.assertEqual(sorted(plan.upload), sorted(GAME_FILES))
        written = self.written()
        keys = [key for _, key in written]
        self.assertTrue(all(key.startswith("web/v1/") for key in keys))
        game = [f"web/v1/game/{relative}" for relative in GAME_FILES]
        self.assertCountEqual(keys[: len(game)], game)
        endings = ["game_files.md5", "game_files.txt", "index.wasm", "index.js", *build_web.APP_FILES, "index.html"]
        self.assertEqual(keys[len(game) :], [f"web/v1/{name}" for name in endings])
        for relative, body in GAME_FILES.items():
            self.assertEqual(self.s3.stored(f"web/v1/game/{relative}").read_bytes(), body)
        published = publish_web.parse_hash_list(self.s3.stored("web/v1/game_files.md5").read_bytes())
        self.assertEqual(published, {path: hashlib.md5(body).hexdigest() for path, body in GAME_FILES.items()})

    def test_rerun_of_a_finished_release_writes_nothing(self) -> None:
        self.publish("v1", GAME_FILES)
        self.written()

        plan, sent = self.publish("v1", GAME_FILES)

        self.assertEqual((plan.upload, plan.copy, sent), ([], [], 0))
        self.assertEqual(sorted(plan.present), sorted(GAME_FILES))
        self.assertEqual(self.written(), [])

    def test_next_release_copies_unchanged_files_from_the_previous_one(self) -> None:
        self.publish("v1", GAME_FILES)
        self.written()
        changed = {**GAME_FILES, "data/rules.json": b'{"rules": 2}', "data/new.json": b"{}"}

        plan, _ = self.publish("v2", changed, previous="v1")

        self.assertEqual(sorted(plan.upload), ["data/new.json", "data/rules.json"])
        self.assertEqual(
            sorted(plan.copy), ["client-additional-data/characters.json", "client-additional-data/meshes/a/model.glb"]
        )
        actions = dict((key, action) for action, key in self.written())
        self.assertEqual(actions["web/v2/game/client-additional-data/characters.json"], "copy")
        self.assertEqual(actions["web/v2/game/data/rules.json"], "upload")
        self.assertEqual(self.s3.stored("web/v2/game/data/rules.json").read_bytes(), b'{"rules": 2}')
        self.assertEqual(self.s3.stored("web/v1/game/data/rules.json").read_bytes(), b'{"rules": 1}')

    def test_a_copy_whose_source_is_gone_is_uploaded_instead(self) -> None:
        self.publish("v1", GAME_FILES)
        self.s3.stored("web/v1/game/data/rules.json").unlink()
        self.written()

        self.publish("v2", GAME_FILES, previous="v1")

        self.assertEqual(self.s3.stored("web/v2/game/data/rules.json").read_bytes(), GAME_FILES["data/rules.json"])
        self.assertIn(("upload", "web/v2/game/data/rules.json"), self.written())

    def test_published_release_never_takes_other_bytes(self) -> None:
        self.publish("v1", GAME_FILES)
        self.written()

        with self.assertRaises(SystemExit):
            self.publish("v1", {**GAME_FILES, "data/rules.json": b'{"rules": 2}'})
        self.assertEqual(self.written(), [])

    def test_rebuilt_page_under_a_published_release_is_refused_before_any_write(self) -> None:
        self.publish("v1", GAME_FILES)
        self.s3.stored("web/v1/game/data/rules.json").unlink()
        self.written()
        (self.base / "v1" / "web" / "index.js").write_text("var Module = {changed: true};")
        built = publish_web.read_release(self.base / "v1" / "web", "v1", "", CONTENT_URL, workers=2)

        with self.assertRaises(SystemExit):
            publish_web.publish(self.bucket, built, dry_run=False, workers=2)
        self.assertEqual(self.written(), [])

    def test_summary_names_only_what_this_run_publishes(self) -> None:
        out = write_build(self.base / "v1", "v1", GAME_FILES)
        built = publish_web.read_release(out, "v1", "", CONTENT_URL, workers=2)
        plan, sent = publish_web.publish(self.bucket, built, dry_run=False, workers=2)
        self.assertIn(
            "published game_files.md5, game_files.txt, index.wasm, index.js, manifest.json, icon-180.png, "
            "icon-192.png, icon-512.png, index.html last",
            publish_web.summary(built, plan, sent, dry_run=False),
        )

        plan, sent = publish_web.publish(self.bucket, built, dry_run=False, workers=2)
        self.assertIn("already there", publish_web.summary(built, plan, sent, dry_run=False))

    def test_dry_run_plans_without_writing(self) -> None:
        plan, sent = self.publish("v1", GAME_FILES, dry_run=True)

        self.assertEqual(sorted(plan.upload), sorted(GAME_FILES))
        self.assertEqual(sent, sum(len(body) for body in GAME_FILES.values()))
        self.assertEqual(self.written(), [])

    def test_previous_release_must_be_published(self) -> None:
        with self.assertRaises(SystemExit):
            self.publish("v2", GAME_FILES, previous="v1")

    def test_page_only_release_copies_every_game_file_and_ignores_the_local_cache(self) -> None:
        self.publish("v1", GAME_FILES)
        self.written()
        changed_locally = {**GAME_FILES, "data/rules.json": b'{"rules": 2}', "data/new.json": b"{}"}
        out = write_build(self.base / "v2", "v2", changed_locally)
        built = publish_web.read_release(out, "v2", "v1", CONTENT_URL, workers=2, reuse_game_files=True)

        plan, sent = publish_web.publish(self.bucket, built, dry_run=False, workers=2)

        self.assertEqual((plan.upload, sent), ([], 0))
        self.assertEqual(sorted(plan.copy), sorted(GAME_FILES))
        self.assertEqual(self.s3.stored("web/v2/game/data/rules.json").read_bytes(), GAME_FILES["data/rules.json"])
        self.assertFalse(self.s3.stored("web/v2/game/data/new.json").exists())
        for listed in (build_web.FILE_LIST, publish_web.HASH_LIST):
            self.assertEqual(
                self.s3.stored(f"web/v2/{listed}").read_bytes(), self.s3.stored(f"web/v1/{listed}").read_bytes()
            )
        self.assertEqual(self.s3.stored("web/v2/index.js").read_bytes(), b"var Module = {};")

    def test_page_only_release_stops_when_a_reused_file_is_gone(self) -> None:
        self.publish("v1", GAME_FILES)
        self.s3.stored("web/v1/game/data/rules.json").unlink()
        out = write_build(self.base / "v2", "v2", GAME_FILES)
        built = publish_web.read_release(out, "v2", "v1", CONTENT_URL, workers=2, reuse_game_files=True)

        with self.assertRaises(SystemExit):
            publish_web.publish(self.bucket, built, dry_run=False, workers=2)

    def test_page_only_release_needs_a_previous_release(self) -> None:
        out = write_build(self.base / "v2", "v2", GAME_FILES)
        with self.assertRaises(SystemExit):
            publish_web.read_release(out, "v2", "", CONTENT_URL, workers=2, reuse_game_files=True)


class ReadReleaseTest(unittest.TestCase):
    def test_page_built_for_another_release_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            out = write_build(Path(temporary), "v1", GAME_FILES)
            with self.assertRaises(SystemExit):
                publish_web.read_release(out, "v2", "", CONTENT_URL, workers=1)

    def test_release_names_cannot_leave_their_folder(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            for name in ("", "../manifest", "v1/x", ".hidden"):
                with self.subTest(name=name):
                    out = write_build(Path(temporary), name, GAME_FILES)
                    with self.assertRaises(SystemExit):
                        publish_web.read_release(out, name, "", CONTENT_URL, workers=1)

    def test_files_url_matches_the_makefile_layout(self) -> None:
        self.assertEqual(
            publish_web.files_url("https://cdn.hodareborn.com/", "v0.31.0"), "https://cdn.hodareborn.com/web/v0.31.0/"
        )


if __name__ == "__main__":
    unittest.main()
