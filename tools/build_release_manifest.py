#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import re
import subprocess
import sys
import tempfile
import zipfile
from dataclasses import dataclass
from pathlib import Path

CLI_DESCRIPTION = (
    "Write client-update.json for a release: the size, SHA-256 and executable of each platform's zip, which the game "
    "reads to update itself. With --signing-key and --public-key it also writes client-update.json.sig, an Ed25519 "
    "signature in hex, and checks it against the public key compiled into the client."
)

MANIFEST_FORMAT = 1
MANIFEST_NAME = "client-update.json"
SIGNATURE_NAME = MANIFEST_NAME + ".sig"
RELEASE_TAG = re.compile(r"v\d+\.\d+\.\d+(-[0-9A-Za-z]+(\.[0-9A-Za-z]+)*)?")
GIT_DESCRIBE_SUFFIX = re.compile(r"-\d+-g[0-9a-f]+$")
UNCOMMITTED_SUFFIX = "-dirty"
MAX_ARCHIVE_BYTES = 256 * 1024 * 1024
MAX_UNPACKED_BYTES = 512 * 1024 * 1024
DIGEST_CHUNK_BYTES = 1024 * 1024
PUBLIC_KEY_BYTES = 32
SIGNATURE_BYTES = 64
ED25519_PUBLIC_KEY_DER_PREFIX = bytes.fromhex("302a300506032b6570032100")
PEM_LINE_LENGTH = 64


@dataclass(frozen=True)
class PlatformBuild:
    platform: str
    file: str
    executable: str


BUILDS = (
    PlatformBuild(
        "macos",
        "Heroes-of-Dragon-Age-macos-universal.zip",
        "Heroes of Dragon Age.app/Contents/MacOS/Heroes of Dragon Age",
    ),
    PlatformBuild("windows", "Heroes-of-Dragon-Age-windows-x86_64.zip", "Heroes of Dragon Age.exe"),
    PlatformBuild("linux", "Heroes-of-Dragon-Age-linux-x86_64.zip", "Heroes of Dragon Age"),
)


class ReleaseManifestError(Exception):
    pass


def check_release_tag(version: str) -> None:
    described = GIT_DESCRIBE_SUFFIX.search(version) or version.endswith(UNCOMMITTED_SUFFIX)
    if not RELEASE_TAG.fullmatch(version) or described:
        raise ReleaseManifestError(f"{version!r} is not a release tag such as v1.2.3 or v1.2.3-rc.1")


def file_digest(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        while chunk := file.read(DIGEST_CHUNK_BYTES):
            digest.update(chunk)
    return digest.hexdigest()


def check_archive(path: Path, build: PlatformBuild) -> None:
    if not path.is_file():
        raise ReleaseManifestError(f"{path} is missing")
    if path.stat().st_size > MAX_ARCHIVE_BYTES:
        raise ReleaseManifestError(f"{path} is larger than the {MAX_ARCHIVE_BYTES} bytes the game accepts")
    try:
        with zipfile.ZipFile(path) as archive:
            entries = archive.infolist()
    except zipfile.BadZipFile as error:
        raise ReleaseManifestError(f"{path} is not a zip archive") from error
    if build.executable not in {entry.filename for entry in entries}:
        raise ReleaseManifestError(f"{path} does not contain {build.executable!r}")
    if sum(entry.file_size for entry in entries) > MAX_UNPACKED_BYTES:
        raise ReleaseManifestError(f"{path} unpacks to more than the {MAX_UNPACKED_BYTES} bytes the game accepts")


def build_entry(directory: Path, build: PlatformBuild) -> dict[str, object]:
    path = directory / build.file
    check_archive(path, build)
    return {
        "platform": build.platform,
        "file": build.file,
        "size": path.stat().st_size,
        "sha256": file_digest(path),
        "executable": build.executable,
    }


def build_manifest(version: str, directory: Path) -> dict[str, object]:
    check_release_tag(version)
    return {
        "format": MANIFEST_FORMAT,
        "version": version,
        "builds": [build_entry(directory, build) for build in BUILDS],
    }


def write_manifest(version: str, directory: Path) -> Path:
    path = directory / MANIFEST_NAME
    path.write_text(json.dumps(build_manifest(version, directory), separators=(",", ":")) + "\n", encoding="utf-8")
    return path


def public_key_pem(public_key_hex: str) -> str:
    try:
        key = bytes.fromhex(public_key_hex)
    except ValueError as error:
        raise ReleaseManifestError("the public key is not hexadecimal") from error
    if len(key) != PUBLIC_KEY_BYTES:
        raise ReleaseManifestError(f"the public key must be {PUBLIC_KEY_BYTES} bytes")
    encoded = base64.b64encode(ED25519_PUBLIC_KEY_DER_PREFIX + key).decode("ascii")
    lines = [encoded[start : start + PEM_LINE_LENGTH] for start in range(0, len(encoded), PEM_LINE_LENGTH)]
    return "\n".join(["-----BEGIN PUBLIC KEY-----", *lines, "-----END PUBLIC KEY-----", ""])


def run_openssl(arguments: list[str], failure: str) -> None:
    try:
        result = subprocess.run(["openssl", *arguments], capture_output=True, text=True, check=False)
    except FileNotFoundError as error:
        raise ReleaseManifestError("openssl is not installed") from error
    if result.returncode != 0:
        raise ReleaseManifestError(f"{failure}: {result.stderr.strip() or result.stdout.strip()}")


def sign_manifest(manifest: Path, signing_key: Path, public_key_hex: str) -> Path:
    public_key = public_key_pem(public_key_hex)
    with tempfile.TemporaryDirectory() as scratch:
        signature = Path(scratch) / "signature"
        verifying_key = Path(scratch) / "public.pem"
        verifying_key.write_text(public_key, encoding="ascii")
        run_openssl(
            ["pkeyutl", "-sign", "-rawin", "-inkey", str(signing_key), "-in", str(manifest), "-out", str(signature)],
            "openssl could not sign the manifest",
        )
        run_openssl(
            [
                "pkeyutl",
                "-verify",
                "-rawin",
                "-pubin",
                "-inkey",
                str(verifying_key),
                "-in",
                str(manifest),
                "-sigfile",
                str(signature),
            ],
            "the signing key does not match the public key compiled into the client",
        )
        signature_bytes = signature.read_bytes()
    if len(signature_bytes) != SIGNATURE_BYTES:
        raise ReleaseManifestError("the signing key is not an Ed25519 key")
    path = manifest.with_name(SIGNATURE_NAME)
    path.write_text(signature_bytes.hex() + "\n", encoding="ascii")
    return path


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--version", required=True, help="release tag the archives were built from, such as v1.2.3")
    parser.add_argument("--directory", required=True, type=Path, help="folder holding the release zips")
    parser.add_argument("--signing-key", type=Path, help="PEM file with the Ed25519 private release key")
    parser.add_argument("--public-key", help="hex public key compiled into the client (make release-public-key)")
    arguments = parser.parse_args()
    if bool(arguments.signing_key) != bool(arguments.public_key):
        parser.error("--signing-key and --public-key go together")
    try:
        manifest = write_manifest(arguments.version, arguments.directory)
        print(f"wrote {manifest}")
        if arguments.signing_key:
            print(f"wrote {sign_manifest(manifest, arguments.signing_key, arguments.public_key)}")
    except ReleaseManifestError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
