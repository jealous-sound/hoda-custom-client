from __future__ import annotations

import io
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build_content_packs
import build_dist
import publish_content


HERO_DIR = f"{build_dist.CHAR_PACK_DIR}/Hero"
RULES_FILE = "data/gear.json"
TEXT = b"heroes of dragon age " * 400
RANDOM_BYTES = os.urandom(4096)
FIXED_VERSION = "20260101T000000Z"


class ContentPacksTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.base = Path(self.temporary.name)
        self.root = self.base / "root"
        self.files = {
            RULES_FILE: TEXT,
            f"{HERO_DIR}/Hero_rl.glb": RANDOM_BYTES,
            f"{HERO_DIR}/skin.png": TEXT,
            f"{build_dist.ORIGINAL_BUNDLES}/0123abcd/CAB-hero": TEXT,
            f"{build_dist.ASSETS}/characters.json": b"{}",
            build_dist.ORIGINAL_APK: RANDOM_BYTES,
        }
        for relative, data in self.files.items():
            self.write(relative, data)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def write(self, relative: str, data: bytes) -> None:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)

    def build(self, out: str, previous: dict | None = None) -> tuple[dict, list[str]]:
        return build_content_packs.build(
            self.root, sorted(self.files), self.base / out, previous, workers=1, version=FIXED_VERSION
        )

    def pack(self, manifest: dict, name: str) -> dict:
        return next(pack for pack in manifest["packs"] if pack["name"] == name)

    def test_files_of_one_group_share_a_bucketed_pack(self) -> None:
        model = build_content_packs.pack_name(f"{HERO_DIR}/Hero_rl.glb")
        self.assertEqual(model, build_content_packs.pack_name(f"{HERO_DIR}/skin.png"))
        self.assertRegex(model, r"^characters-[0-9]{2}$")
        self.assertEqual(build_content_packs.pack_name(RULES_FILE), "rules")
        self.assertEqual(build_content_packs.pack_name(build_dist.ORIGINAL_APK), "apk")
        self.assertEqual(build_content_packs.pack_name(f"{build_dist.ASSETS}/characters.json"), "core")

    def test_buckets_stay_within_the_rule_count(self) -> None:
        names = {build_content_packs.pack_name(f"{HERO_DIR}{index}/a.png") for index in range(500)}
        self.assertEqual(len(names), 32)

    def test_rebuilding_the_same_files_gives_identical_packs(self) -> None:
        first, _ = self.build("first")
        second, _ = self.build("second")
        self.assertEqual(first, second)
        for pack in first["packs"]:
            self.assertEqual((self.base / "first" / pack["file"]).read_bytes(), (self.base / "second" / pack["file"]).read_bytes())

    def test_packs_round_trip_and_only_compressible_files_are_deflated(self) -> None:
        manifest, written = self.build("out")
        self.assertEqual(len(written), len(manifest["packs"]))
        extracted = {}
        methods = {}
        for pack in manifest["packs"]:
            data = (self.base / "out" / pack["file"]).read_bytes()
            self.assertEqual(len(data), pack["size"])
            with zipfile.ZipFile(io.BytesIO(data)) as archive:
                for info in archive.infolist():
                    extracted[info.filename] = archive.read(info)
                    methods[info.filename] = info.compress_type
        self.assertEqual(extracted, self.files)
        self.assertEqual(methods[RULES_FILE], zipfile.ZIP_DEFLATED)
        self.assertEqual(methods[build_dist.ORIGINAL_APK], zipfile.ZIP_STORED)

    def test_manifest_on_disk_matches_the_returned_one(self) -> None:
        manifest, _ = self.build("out")
        on_disk = json.loads((self.base / "out" / build_content_packs.MANIFEST_NAME).read_text())
        self.assertEqual(on_disk, manifest)
        self.assertEqual(manifest["format"], build_content_packs.MANIFEST_FORMAT)
        names = [pack["name"] for pack in manifest["packs"]]
        self.assertEqual(names[:2], ["apk", "core"])
        self.assertEqual(names[2:], sorted(names[2:]))

    def test_paths_windows_cannot_store_stop_the_build(self) -> None:
        self.files[f"{build_dist.MESH_DIR}/scene/unity_14:grass.ply"] = TEXT
        self.write(f"{build_dist.MESH_DIR}/scene/unity_14:grass.ply", TEXT)
        with self.assertRaises(SystemExit) as raised:
            self.build("out")
        self.assertIn("unity_14:grass.ply", str(raised.exception))

    def test_paths_too_long_for_a_windows_install_stop_the_build(self) -> None:
        self.assertEqual(build_content_packs.WINDOWS_ENTRY_PATH_LIMIT, 210)
        prefix = f"{build_dist.MESH_DIR}/scene/"
        fitting = prefix + "f" * (build_content_packs.WINDOWS_ENTRY_PATH_LIMIT - len(prefix))
        build_content_packs.require_portable([fitting])
        with self.assertRaises(SystemExit) as raised:
            build_content_packs.require_portable([fitting + "x"])
        self.assertIn(fitting + "x", str(raised.exception))

    def test_unchanged_packs_reuse_the_published_entry(self) -> None:
        published, _ = self.build("first")
        manifest, written = self.build("second", published)
        self.assertEqual(written, [])
        self.assertEqual(manifest["packs"], published["packs"])

    def test_a_changed_file_rebuilds_only_its_pack(self) -> None:
        published, _ = self.build("first")
        self.write(RULES_FILE, b"changed")
        manifest, written = self.build("second", published)
        self.assertEqual(written, [self.pack(manifest, "rules")["file"]])
        self.assertNotEqual(self.pack(manifest, "rules")["sha256"], self.pack(published, "rules")["sha256"])
        self.assertEqual(self.pack(manifest, "apk"), self.pack(published, "apk"))

    def test_a_manifest_of_another_format_is_not_reused(self) -> None:
        published, _ = self.build("first")
        published["format"] = build_content_packs.MANIFEST_FORMAT + 1
        _, written = self.build("second", published)
        self.assertEqual(len(written), len(published["packs"]))


class PublishContentTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.base = Path(self.temporary.name)
        self.root = self.base / "root"
        (self.root / "data").mkdir(parents=True)
        (self.root / RULES_FILE).write_bytes(TEXT)
        self.uploads: list[tuple[str, str, str]] = []

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def upload(self, bucket: str, key: str, path: Path, content_type: str, cache_control: str) -> None:
        self.assertTrue(path.is_file())
        self.uploads.append((bucket, key, cache_control))

    def publish(self, published: dict | None, dry_run: bool = False) -> tuple[dict, list[str], bool]:
        return publish_content.publish(
            self.root,
            [RULES_FILE],
            self.base / "out",
            "bucket",
            "https://example.test/",
            upload=self.upload,
            fetch=lambda url: published,
            dry_run=dry_run,
            workers=1,
        )

    def test_packs_upload_before_the_manifest(self) -> None:
        manifest, written, released = self.publish(None)
        self.assertTrue(released)
        keys = [key for _, key, _ in self.uploads]
        self.assertEqual(keys, written + [build_content_packs.MANIFEST_NAME])
        self.assertEqual(self.uploads[0][2], publish_content.PACK_CACHE_CONTROL)
        self.assertEqual(self.uploads[-1][2], publish_content.MANIFEST_CACHE_CONTROL)

    def test_nothing_is_uploaded_when_the_published_packs_are_current(self) -> None:
        published, _, _ = self.publish(None)
        self.uploads.clear()
        _, written, released = self.publish(published)
        self.assertEqual((written, released, self.uploads), ([], False, []))

    def test_a_format_change_uploads_only_packs_the_bucket_lacks(self) -> None:
        published, _, _ = self.publish(None)
        published["format"] = build_content_packs.MANIFEST_FORMAT - 1
        self.uploads.clear()
        manifest, written, released = self.publish(published)
        self.assertTrue(written)
        self.assertTrue(released)
        self.assertEqual([key for _, key, _ in self.uploads], [build_content_packs.MANIFEST_NAME])
        self.assertEqual(manifest["packs"], [dict(pack) for pack in published["packs"]])

    def test_dry_run_builds_without_uploading(self) -> None:
        _, written, released = self.publish(None, dry_run=True)
        self.assertTrue(written)
        self.assertFalse(released)
        self.assertEqual(self.uploads, [])

    def test_manifest_url_joins_the_base_once(self) -> None:
        self.assertEqual(publish_content.manifest_url("https://example.test/"), "https://example.test/manifest.json")


if __name__ == "__main__":
    unittest.main()
