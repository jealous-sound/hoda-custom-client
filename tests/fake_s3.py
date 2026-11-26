from __future__ import annotations

import base64
import hashlib
import shutil
import threading
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, unquote, urlsplit
from xml.sax.saxutils import escape

NAMESPACE = "http://s3.amazonaws.com/doc/2006-03-01/"
SIGNED = "AWS4-HMAC-SHA256 Credential="
DEFAULT_PAGE_SIZE = 1000


class FakeS3:
    def __init__(self, root: Path, bucket: str, page_size: int = DEFAULT_PAGE_SIZE, port: int = 0):
        self.root, self.bucket, self.page_size = root, bucket, page_size
        self.lock = threading.Lock()
        self.writes: list[tuple[str, str]] = []
        self.etags: dict[str, str] = {}
        self.server = ThreadingHTTPServer(("127.0.0.1", port), self.handler())
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)

    @property
    def endpoint(self) -> str:
        return f"http://127.0.0.1:{self.server.server_address[1]}"

    def __enter__(self) -> FakeS3:
        self.thread.start()
        return self

    def __exit__(self, *_: object) -> None:
        self.server.shutdown()
        self.server.server_close()

    def stored(self, key: str) -> Path:
        return self.root / key

    def etag(self, key: str) -> str:
        with self.lock:
            known = self.etags.get(key)
        return known or hashlib.md5(self.stored(key).read_bytes(), usedforsecurity=False).hexdigest()

    def keys(self, prefix: str) -> list[str]:
        if not self.root.is_dir():
            return []
        found = (path.relative_to(self.root).as_posix() for path in self.root.rglob("*") if path.is_file())
        return sorted(key for key in found if key.startswith(prefix))

    def remember(self, action: str, key: str, etag: str) -> None:
        with self.lock:
            self.writes.append((action, key))
            self.etags[key] = etag

    def handler(self) -> type[BaseHTTPRequestHandler]:
        store = self

        class Handler(BaseHTTPRequestHandler):
            protocol_version = "HTTP/1.1"

            def log_message(self, *_: object) -> None:
                pass

            def reply(self, status: int, body: bytes = b"", headers: dict[str, str] | None = None) -> None:
                self.send_response(status)
                for name, value in (headers or {}).items():
                    self.send_header(name, value)
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                if self.command != "HEAD":
                    self.wfile.write(body)

            def missing(self) -> None:
                self.reply(HTTPStatus.NOT_FOUND, b"<Error><Code>NoSuchKey</Code></Error>")

            def target(self) -> tuple[str, dict[str, list[str]]] | None:
                if not self.headers.get("Authorization", "").startswith(SIGNED):
                    self.reply(HTTPStatus.FORBIDDEN, b"<Error><Code>AccessDenied</Code></Error>")
                    return None
                parts = urlsplit(self.path)
                bucket, _, key = unquote(parts.path).lstrip("/").partition("/")
                if bucket != store.bucket:
                    self.reply(HTTPStatus.NOT_FOUND, b"<Error><Code>NoSuchBucket</Code></Error>")
                    return None
                return key, parse_qs(parts.query)

            def do_GET(self) -> None:
                found = self.target()
                if found is None:
                    return
                key, query = found
                if not key:
                    self.listing(query)
                elif store.stored(key).is_file():
                    self.reply(HTTPStatus.OK, store.stored(key).read_bytes(), {"ETag": f'"{store.etag(key)}"'})
                else:
                    self.missing()

            def listing(self, query: dict[str, list[str]]) -> None:
                keys = store.keys(query.get("prefix", [""])[0])
                start = int(query.get("continuation-token", ["0"])[0])
                page = keys[start : start + store.page_size]
                truncated = start + store.page_size < len(keys)
                contents = "".join(
                    f"<Contents><Key>{escape(key)}</Key><ETag>&quot;{store.etag(key)}&quot;</ETag></Contents>"
                    for key in page
                )
                token = f"<NextContinuationToken>{start + store.page_size}</NextContinuationToken>" if truncated else ""
                body = (
                    f'<ListBucketResult xmlns="{NAMESPACE}">{contents}'
                    f"<IsTruncated>{str(truncated).lower()}</IsTruncated>{token}</ListBucketResult>"
                )
                self.reply(HTTPStatus.OK, body.encode())

            def do_PUT(self) -> None:
                found = self.target()
                if found is None:
                    return
                key, _ = found
                body = self.rfile.read(int(self.headers.get("Content-Length") or 0))
                source = self.headers.get("x-amz-copy-source")
                destination = store.stored(key)
                if source:
                    _, _, source_key = unquote(source).lstrip("/").partition("/")
                    if not store.stored(source_key).is_file():
                        self.missing()
                        return
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(store.stored(source_key), destination)
                    etag = store.etag(source_key)
                    store.remember("copy", key, etag)
                    self.reply(
                        HTTPStatus.OK, f"<CopyObjectResult><ETag>&quot;{etag}&quot;</ETag></CopyObjectResult>".encode()
                    )
                    return
                digest = hashlib.md5(body, usedforsecurity=False)
                claimed = self.headers.get("Content-MD5")
                if claimed and base64.b64decode(claimed) != digest.digest():
                    self.reply(HTTPStatus.BAD_REQUEST, b"<Error><Code>BadDigest</Code></Error>")
                    return
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(body)
                store.remember("upload", key, digest.hexdigest())
                self.reply(HTTPStatus.OK, headers={"ETag": f'"{digest.hexdigest()}"'})

        return Handler
