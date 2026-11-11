from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build_release_manifest


VERSION = "v1.2.3"
PUBLIC_KEY_DER_BYTES = 44


def openssl(*arguments: str) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(["openssl", *arguments], capture_output=True, check=False)


def generate_key(path: Path) -> str | None:
    try:
        generated = openssl("genpkey", "-algorithm", "ed25519", "-out", str(path))
        public = openssl("pkey", "-in", str(path), "-pubout", "-outform", "DER")
    except FileNotFoundError:
        return None
    if generated.returncode != 0 or public.returncode != 0 or len(public.stdout) != PUBLIC_KEY_DER_BYTES:
        return None
    return public.stdout[-build_release_manifest.PUBLIC_KEY_BYTES :].hex()


def openssl_signs_ed25519() -> bool:
    with tempfile.TemporaryDirectory() as scratch:
        key = Path(scratch) / "key.pem"
        message = Path(scratch) / "message"
        message.write_bytes(b"probe")
        if generate_key(key) is None:
            return False
        signed = openssl(
            "pkeyutl", "-sign", "-rawin", "-inkey", str(key), "-in", str(message), "-out", str(message) + ".sig"
        )
        return signed.returncode == 0


OPENSSL_SIGNS_ED25519 = openssl_signs_ed25519()


class ReleaseManifestTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.directory = Path(self.temporary.name)
        for build in build_release_manifest.BUILDS:
            self.write_archive(build.file, {build.executable: build.platform.encode()})

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def write_archive(self, name: str, files: dict[str, bytes]) -> None:
        with zipfile.ZipFile(self.directory / name, "w") as archive:
            for entry, data in files.items():
                archive.writestr(entry, data)

    def test_manifest_lists_every_platform_archive(self) -> None:
        manifest = build_release_manifest.build_manifest(VERSION, self.directory)
        self.assertEqual(manifest["format"], build_release_manifest.MANIFEST_FORMAT)
        self.assertEqual(manifest["version"], VERSION)
        self.assertEqual([build["platform"] for build in manifest["builds"]], ["macos", "windows", "linux"])
        for build in manifest["builds"]:
            archive = (self.directory / build["file"]).read_bytes()
            self.assertEqual(build["size"], len(archive))
            self.assertEqual(build["sha256"], hashlib.sha256(archive).hexdigest())
            with zipfile.ZipFile(self.directory / build["file"]) as packed:
                self.assertIn(build["executable"], packed.namelist())

    def test_written_manifest_is_the_signed_text(self) -> None:
        path = build_release_manifest.write_manifest(VERSION, self.directory)
        self.assertEqual(path.name, "client-update.json")
        self.assertEqual(json.loads(path.read_text()), build_release_manifest.build_manifest(VERSION, self.directory))

    def test_only_release_tags_are_accepted(self) -> None:
        for accepted in ("v0.4.0", "v1.12.3", "v0.5.0-rc.1"):
            build_release_manifest.check_release_tag(accepted)
        for refused in ("1.2.3", "v1.2", "v1.2.3-4-gabc1234", "v1.2.3-dirty", "v1.2.3-rc.1-2-g0f3a9c1", "latest", ""):
            with self.assertRaises(build_release_manifest.ReleaseManifestError, msg=refused):
                build_release_manifest.check_release_tag(refused)

    def test_missing_archive_is_an_error(self) -> None:
        (self.directory / build_release_manifest.BUILDS[2].file).unlink()
        with self.assertRaisesRegex(build_release_manifest.ReleaseManifestError, "is missing"):
            build_release_manifest.build_manifest(VERSION, self.directory)

    def test_archive_without_its_executable_is_an_error(self) -> None:
        self.write_archive(build_release_manifest.BUILDS[1].file, {"readme.txt": b"no game here"})
        with self.assertRaisesRegex(build_release_manifest.ReleaseManifestError, "does not contain"):
            build_release_manifest.build_manifest(VERSION, self.directory)

    def test_file_that_is_not_a_zip_is_an_error(self) -> None:
        (self.directory / build_release_manifest.BUILDS[0].file).write_bytes(b"not a zip")
        with self.assertRaisesRegex(build_release_manifest.ReleaseManifestError, "not a zip archive"):
            build_release_manifest.build_manifest(VERSION, self.directory)

    def test_public_key_must_be_thirty_two_hex_bytes(self) -> None:
        self.assertIn("BEGIN PUBLIC KEY", build_release_manifest.public_key_pem("ab" * 32))
        for refused in ("", "ab" * 31, "zz" * 32):
            with self.assertRaises(build_release_manifest.ReleaseManifestError, msg=refused):
                build_release_manifest.public_key_pem(refused)

    @unittest.skipUnless(OPENSSL_SIGNS_ED25519, "this openssl cannot sign with Ed25519")
    def test_signature_verifies_against_the_matching_public_key(self) -> None:
        key = self.directory / "key.pem"
        public_key = generate_key(key)
        manifest = build_release_manifest.write_manifest(VERSION, self.directory)
        signature = build_release_manifest.sign_manifest(manifest, key, public_key)
        self.assertEqual(signature.name, "client-update.json.sig")
        signature_hex = signature.read_text().strip()
        self.assertEqual(len(bytes.fromhex(signature_hex)), build_release_manifest.SIGNATURE_BYTES)

        raw = self.directory / "signature.bin"
        raw.write_bytes(bytes.fromhex(signature_hex))
        verifying_key = self.directory / "public.pem"
        verifying_key.write_text(build_release_manifest.public_key_pem(public_key))
        verified = openssl(
            "pkeyutl",
            "-verify",
            "-rawin",
            "-pubin",
            "-inkey",
            str(verifying_key),
            "-in",
            str(manifest),
            "-sigfile",
            str(raw),
        )
        self.assertEqual(verified.returncode, 0)

    @unittest.skipUnless(OPENSSL_SIGNS_ED25519, "this openssl cannot sign with Ed25519")
    def test_signing_key_that_is_not_the_clients_is_refused(self) -> None:
        key = self.directory / "key.pem"
        generate_key(key)
        other_public_key = generate_key(self.directory / "other.pem")
        manifest = build_release_manifest.write_manifest(VERSION, self.directory)
        with self.assertRaisesRegex(build_release_manifest.ReleaseManifestError, "does not match the public key"):
            build_release_manifest.sign_manifest(manifest, key, other_public_key)
        self.assertFalse((self.directory / "client-update.json.sig").exists())


if __name__ == "__main__":
    unittest.main()
