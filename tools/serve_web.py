#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
import threading
import time
import urllib.error
import urllib.request
from collections.abc import Callable
from functools import partial
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

CLI_DESCRIPTION = (
    "Serve the browser build on localhost with the cross-origin isolation headers threads need, byte ranges for "
    "lazily read game files, an optional /api/ proxy to a backend, and an optional delay and bandwidth limit that act "
    "like a distant CDN. With --allow-origin it stands in for the CDN "
    "instead: paths are taken as they come, as R2 does, files carry the bucket's immutable caching headers, and a page "
    "on that origin gets the CORS answers the bucket policy gives."
)

ISOLATION_HEADERS = {
    "Cross-Origin-Opener-Policy": "same-origin",
    "Cross-Origin-Embedder-Policy": "require-corp",
    "Cross-Origin-Resource-Policy": "same-origin",
}
CDN_ALLOWED_METHODS = ("GET", "HEAD")
CDN_ALLOWED_HEADERS = frozenset({"range"})
CDN_EXPOSED_HEADERS = "Content-Length, Content-Range, Accept-Ranges, ETag"
CDN_PREFLIGHT_MAX_AGE_SECONDS = 86400
CDN_CACHE_CONTROL = "public, max-age=31536000, immutable"
PAGE_CACHE_CONTROL = "no-cache"
PAGE = "index.html"
API_PREFIX = "/api/"
STATS_PATH = "/__stats"
RANGE_PATTERN = re.compile(r"bytes=(\d*)-(\d*)$")
HOP_HEADERS = frozenset(
    {"connection", "keep-alive", "transfer-encoding", "te", "trailer", "upgrade", "proxy-authorization", "host"}
)
COPY_BYTES = 1 << 20
LINK_CHUNK_BYTES = 1 << 15
BITS_PER_BYTE = 8
MEGABIT = 1_000_000
PROXY_TIMEOUT_SECONDS = 120


class Stats:
    def __init__(self) -> None:
        self.lock = threading.Lock()
        self.requests = 0
        self.bytes = 0
        self.preflights = 0

    def add(self, sent: int) -> None:
        with self.lock:
            self.requests += 1
            self.bytes += sent

    def add_preflight(self) -> None:
        with self.lock:
            self.preflights += 1

    def snapshot(self) -> dict[str, int]:
        with self.lock:
            return {"requests": self.requests, "bytes": self.bytes, "preflights": self.preflights}


class Link:
    def __init__(
        self,
        megabits_per_second: float,
        clock: Callable[[], float] = time.monotonic,
        sleep: Callable[[float], None] = time.sleep,
    ) -> None:
        self.bytes_per_second = megabits_per_second * MEGABIT / BITS_PER_BYTE
        self.clock = clock
        self.sleep = sleep
        self.lock = threading.Lock()
        self.free_at = clock()

    def carry(self, size: int) -> None:
        if self.bytes_per_second <= 0:
            return
        with self.lock:
            now = self.clock()
            self.free_at = max(now, self.free_at) + size / self.bytes_per_second
            wait = self.free_at - now
        if wait > 0:
            self.sleep(wait)


def parse_range(header: str, size: int) -> tuple[int, int] | None:
    match = RANGE_PATTERN.match(header.strip())
    if not match or (not match.group(1) and not match.group(2)):
        return None
    if match.group(1):
        start = int(match.group(1))
        end = int(match.group(2)) if match.group(2) else size - 1
    else:
        start = max(size - int(match.group(2)), 0)
        end = size - 1
    end = min(end, size - 1)
    return (start, end) if start <= end else None


def file_etag(path: Path) -> str:
    status = path.stat()
    return f'"{status.st_size:x}-{status.st_mtime_ns:x}"'


def requested_headers(header: str) -> set[str]:
    return {name.strip().lower() for name in header.split(",") if name.strip()}


class GameHandler(SimpleHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    backend: str = ""
    allowed_origin: str = ""
    latency_seconds: float = 0.0
    link: Link = Link(0)
    stats: Stats

    def end_headers(self) -> None:
        if self.allowed_origin:
            self.send_cors_headers()
        else:
            for name, value in ISOLATION_HEADERS.items():
                self.send_header(name, value)
            self.send_header("Cache-Control", PAGE_CACHE_CONTROL)
        super().end_headers()

    def from_allowed_origin(self) -> bool:
        return bool(self.allowed_origin) and self.headers.get("Origin") == self.allowed_origin

    def send_cors_headers(self) -> None:
        self.send_header("Vary", "Origin")
        if self.from_allowed_origin():
            self.send_header("Access-Control-Allow-Origin", self.allowed_origin)
            self.send_header("Access-Control-Expose-Headers", CDN_EXPOSED_HEADERS)

    def preflight_allowed(self) -> bool:
        method = self.headers.get("Access-Control-Request-Method", "")
        headers = requested_headers(self.headers.get("Access-Control-Request-Headers", ""))
        return self.from_allowed_origin() and method in CDN_ALLOWED_METHODS and headers <= CDN_ALLOWED_HEADERS

    def do_OPTIONS(self) -> None:
        self.stats.add_preflight()
        if not self.preflight_allowed():
            self.send_response(HTTPStatus.FORBIDDEN)
            self.send_header("Content-Length", "0")
            self.end_headers()
            return
        self.send_response(HTTPStatus.NO_CONTENT)
        self.send_header("Access-Control-Allow-Methods", ", ".join(CDN_ALLOWED_METHODS))
        self.send_header("Access-Control-Allow-Headers", ", ".join(sorted(CDN_ALLOWED_HEADERS)))
        self.send_header("Access-Control-Max-Age", str(CDN_PREFLIGHT_MAX_AGE_SECONDS))
        self.end_headers()

    def do_GET(self) -> None:
        if self.path == STATS_PATH:
            self.send_json(self.stats.snapshot())
        elif self.path.startswith(API_PREFIX):
            self.proxy()
        else:
            self.send_file(include_body=True)

    def do_HEAD(self) -> None:
        if self.path.startswith(API_PREFIX):
            self.proxy()
        else:
            self.send_file(include_body=False)

    def do_POST(self) -> None:
        self.proxy()

    def do_PUT(self) -> None:
        self.proxy()

    def do_DELETE(self) -> None:
        self.proxy()

    def send_json(self, payload: dict[str, int]) -> None:
        body = json.dumps(payload).encode()
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def send_file(self, include_body: bool) -> None:
        time.sleep(self.latency_seconds)
        path = Path(self.translate_path(self.path))
        if path.is_dir():
            path = path / PAGE
        if "//" in self.path.split("?", 1)[0] or not path.is_file():
            self.send_error(HTTPStatus.NOT_FOUND)
            return
        size = path.stat().st_size
        requested = self.headers.get("Range")
        span = parse_range(requested, size) if requested else None
        if requested and span is None and size > 0:
            self.send_response(HTTPStatus.REQUESTED_RANGE_NOT_SATISFIABLE)
            self.send_header("Content-Range", f"bytes */{size}")
            self.end_headers()
            return
        start, end = span if span else (0, size - 1)
        length = max(end - start + 1, 0)
        self.send_response(HTTPStatus.PARTIAL_CONTENT if span else HTTPStatus.OK)
        self.send_header("Content-Type", self.guess_type(str(path)))
        self.send_header("Accept-Ranges", "bytes")
        self.send_header("ETag", file_etag(path))
        if self.allowed_origin:
            self.send_header("Cache-Control", CDN_CACHE_CONTROL)
        self.send_header("Content-Length", str(length if include_body or span else size))
        if span:
            self.send_header("Content-Range", f"bytes {start}-{end}/{size}")
        self.end_headers()
        if not include_body:
            self.stats.add(0)
            return
        with path.open("rb") as source:
            source.seek(start)
            remaining = length
            chunk_bytes = LINK_CHUNK_BYTES if self.link.bytes_per_second > 0 else COPY_BYTES
            while remaining > 0:
                chunk = source.read(min(chunk_bytes, remaining))
                if not chunk:
                    break
                self.link.carry(len(chunk))
                self.wfile.write(chunk)
                remaining -= len(chunk)
        self.stats.add(length)

    def proxy(self) -> None:
        if not self.backend or not self.path.startswith(API_PREFIX):
            self.send_error(HTTPStatus.BAD_GATEWAY, "no backend configured")
            return
        target = self.backend.rstrip("/") + "/" + self.path[len(API_PREFIX) :]
        length = int(self.headers.get("Content-Length") or 0)
        body = self.rfile.read(length) if length else None
        headers = {name: value for name, value in self.headers.items() if name.lower() not in HOP_HEADERS}
        request = urllib.request.Request(target, data=body, headers=headers, method=self.command)
        try:
            with urllib.request.urlopen(request, timeout=PROXY_TIMEOUT_SECONDS) as reply:
                self.relay(reply.status, reply.headers.items(), reply.read())
        except urllib.error.HTTPError as error:
            self.relay(error.code, error.headers.items(), error.read())
        except urllib.error.URLError as error:
            self.send_error(HTTPStatus.BAD_GATEWAY, str(error.reason))

    def relay(self, status: int, headers: list[tuple[str, str]], body: bytes) -> None:
        self.send_response(status)
        for name, value in headers:
            if name.lower() not in HOP_HEADERS and name.lower() != "content-length":
                self.send_header(name, value)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(body)
        self.stats.add(len(body))


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--root", type=Path, required=True, help="folder made by make web")
    parser.add_argument("--port", type=int, default=8077, help="localhost port (default 8077)")
    parser.add_argument("--backend", default="", help="backend URL that /api/ forwards to, e.g. http://127.0.0.1:8080")
    parser.add_argument(
        "--allow-origin", default="", help="serve as the CDN for a page on this origin, e.g. http://localhost:8080"
    )
    parser.add_argument(
        "--latency-ms", type=float, default=0.0, help="delay before each file answer, to act like a distant CDN"
    )
    parser.add_argument(
        "--rate-mbps", type=float, default=0.0, help="bandwidth all file answers share, in Mbit/s (default unlimited)"
    )
    args = parser.parse_args()
    root = args.root.resolve()
    if not args.allow_origin and not (root / PAGE).is_file():
        raise SystemExit(f"{root} has no {PAGE}; run make web")
    GameHandler.backend = args.backend
    GameHandler.allowed_origin = args.allow_origin
    GameHandler.latency_seconds = args.latency_ms / 1000.0
    GameHandler.link = Link(args.rate_mbps)
    GameHandler.stats = Stats()
    server = ThreadingHTTPServer(("127.0.0.1", args.port), partial(GameHandler, directory=str(root)))
    role = f", the CDN for {args.allow_origin}" if args.allow_origin else ""
    backend = f", /api/ -> {args.backend}" if args.backend else ""
    print(f"Serving {root} at http://127.0.0.1:{args.port}/{role}{backend}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
