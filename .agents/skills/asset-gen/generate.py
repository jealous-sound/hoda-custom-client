#!/usr/bin/env python3
"""
generate.py — Generate 2D / isometric game art (sprites, tiles, icons, UI)
with OpenAI's official Images API (default model: gpt-image-2).

Standard library only — no pip install needed.

API KEY (never hardcoded, never committed):
  Resolved in this order:
    1. $OPENAI_API_KEY environment variable
    2. <this dir>/secrets/openai.key   (gitignored, chmod 600)

USAGE:
  python3 generate.py "PROMPT" [options]

  python3 generate.py "a stone-and-gold fantasy UI button with a tent/camp icon, \
single object centered, on a flat solid bright green background" \
    --size 1536x1024 --out out/camp_button.png   # quality defaults to medium

OPTIONS:
  --out PATH         Output file (PNG). For --n>1, an index is inserted before .png.
                     Default: out/<slug-from-prompt>.png
  --model SLUG       Default: gpt-image-2
  --size WxH         1024x1024 | 1536x1024 | 1024x1536 | auto
  --quality Q        auto | low | medium   (default medium; --quality low to draft)
  --background B     auto | opaque   (gpt-image-2 does NOT support 'transparent';
                     see SKILL.md "Transparency workflow" for the chroma-key workflow)
  --format F         png | jpeg | webp   (default png)
  --n N              Number of images, 1-10 (default 1)
  --ref PATH_OR_URL  Reference image — routes the request through the image *edit*
                     endpoint so the output keeps the reference's character/style.
                     Repeatable (up to 16). Local files / http(s) URLs / data: URLs.
  --dry-run          Print the request and exit WITHOUT calling the API (free).
  --raw-json         Print the response token/usage summary for debugging.
"""

import argparse
import base64
import binascii
import http.client
import json
import os
import re
import sys
import urllib.error
import urllib.parse
import urllib.request

GENERATIONS_URL = "https://api.openai.com/v1/images/generations"
EDITS_URL = "https://api.openai.com/v1/images/edits"
HERE = os.path.dirname(os.path.abspath(__file__))

# OpenAI gpt-image token pricing (USD per 1M tokens). OpenAI's API returns token
# counts but NOT a dollar cost, so we estimate from these. Update if pricing moves;
# the printed figure is labelled "est". Authoritative spend is the OpenAI dashboard.
RATES = {"text_in": 5.0, "image_in": 10.0, "image_out": 40.0}


def resolve_key():
    k = os.environ.get("OPENAI_API_KEY")
    if k and k.strip():
        return k.strip()
    keyfile = os.path.join(HERE, "secrets", "openai.key")
    if os.path.exists(keyfile):
        with open(keyfile) as f:
            v = f.read().strip()
            if v:
                return v
    sys.exit(
        "ERROR: no API key. Set $OPENAI_API_KEY or create "
        + os.path.join("secrets", "openai.key")
    )


def slugify(text, maxlen=40):
    s = re.sub(r"[^a-zA-Z0-9]+", "_", text.strip().lower()).strip("_")
    return s[:maxlen] or "image"


def load_ref(path_or_url):
    """Return (filename, mime, bytes) for a reference image to upload."""
    if path_or_url.startswith("data:"):
        header, _, b64 = path_or_url.partition(",")
        mime = header[5:].split(";")[0] or "image/png"
        return (f"ref.{mime.split('/')[-1]}", mime, base64.b64decode(b64))
    if path_or_url.startswith(("http://", "https://")):
        try:
            with urllib.request.urlopen(path_or_url, timeout=60) as r:
                content = r.read()
                mime = (r.headers.get("Content-Type") or "image/png").split(";")[0]
        except Exception as e:
            sys.exit(f"ERROR: could not download --ref {path_or_url}: {e}")
        # basename of the URL path only (drop any ?query) for the upload filename
        url_path = urllib.parse.urlparse(path_or_url).path
        name = os.path.basename(url_path) or f"ref.{mime.split('/')[-1]}"
        return (name, mime, content)
    if not os.path.exists(path_or_url):
        sys.exit(f"ERROR: --ref file not found: {path_or_url}")
    ext = os.path.splitext(path_or_url)[1].lower().lstrip(".") or "png"
    mime = "image/" + {"jpg": "jpeg"}.get(ext, ext)
    with open(path_or_url, "rb") as f:
        return (os.path.basename(path_or_url), mime, f.read())


def build_multipart(fields, files):
    """Encode a multipart/form-data body. Returns (content_type, body_bytes).
    fields: dict of str->str. files: list of (field_name, filename, mime, bytes)."""
    boundary = "----assetgen" + os.urandom(12).hex()
    out = bytearray()
    for name, value in fields.items():
        out += (
            f"--{boundary}\r\n"
            f'Content-Disposition: form-data; name="{name}"\r\n\r\n'
            f"{value}\r\n"
        ).encode()
    for name, filename, mime, content in files:
        # Escape characters that would break the Content-Disposition header line.
        safe = (
            filename.replace("\\", "_")
            .replace('"', "_")
            .replace("\r", "_")
            .replace("\n", "_")
        ) or "ref.png"
        out += (
            f"--{boundary}\r\n"
            f'Content-Disposition: form-data; name="{name}"; filename="{safe}"\r\n'
            f"Content-Type: {mime}\r\n\r\n"
        ).encode()
        out += content + b"\r\n"
    out += f"--{boundary}--\r\n".encode()
    return f"multipart/form-data; boundary={boundary}", bytes(out)


def stream_images(req):
    """POST an image request and collect the finished images from the SSE stream.

    Returns (items, usage): items=[{"b64_json": ...}, ...]; usage is merged token
    counts. The final image arrives as an `image_generation.completed` event whose
    JSON carries `b64_json` and `usage` at the top level.
    """
    b64s = []
    usage = {
        "total_tokens": 0,
        "input_text_tokens": 0,
        "input_image_tokens": 0,
        "output_image_tokens": 0,
    }

    def consume(payload):
        payload = payload.strip()
        if not payload or payload == "[DONE]":
            return
        try:
            obj = json.loads(payload)
        except json.JSONDecodeError:
            return
        if obj.get("error"):
            sys.exit("ERROR from API:\n" + json.dumps(obj["error"], indent=2))
        b64 = obj.get("b64_json")
        # Keep only finished images. Generations emit `image_generation.completed`
        # and edits emit `image_edit.completed`; both end in `.completed`. Progressive
        # `*.partial_image` events are skipped.
        if b64 and obj.get("type", "image_generation.completed").endswith(".completed"):
            b64s.append(b64)
            u = obj.get("usage") or {}
            usage["total_tokens"] += u.get("total_tokens", 0) or 0
            din = u.get("input_tokens_details") or {}
            usage["input_text_tokens"] += din.get("text_tokens", 0) or 0
            usage["input_image_tokens"] += din.get("image_tokens", 0) or 0
            dout = u.get("output_tokens_details") or {}
            usage["output_image_tokens"] += dout.get("image_tokens", 0) or 0

    try:
        resp = urllib.request.urlopen(req, timeout=600)
    except urllib.error.HTTPError as e:
        sys.exit(
            f"ERROR HTTP {e.code} {e.reason}:\n{e.read().decode(errors='replace')}"
        )
    except urllib.error.URLError as e:
        sys.exit(f"ERROR connecting to OpenAI: {e}")

    data_buf = []
    try:
        with resp:
            for raw in resp:
                line = raw.decode("utf-8", "replace").rstrip("\r\n")
                if line == "":  # blank line = end of one SSE event
                    if data_buf:
                        consume("\n".join(data_buf))
                        data_buf = []
                    continue
                if line.startswith("data:"):
                    data_buf.append(line[5:].lstrip(" "))
                # other SSE lines (event:/id:/retry:/comments) are not needed here
        if data_buf:
            consume("\n".join(data_buf))
    except (http.client.HTTPException, OSError) as e:
        # The connection dropped mid-stream. The usual cause is a slow (>60s) render
        # hitting the API's ~60s connection timeout — use --quality medium, not high.
        if not b64s:
            sys.exit(
                f"ERROR: stream closed before any image arrived "
                f"(often a slow render hitting the ~60s API timeout): {e}"
            )
        print(
            f"  WARNING: stream ended early ({e}); "
            f"using the {len(b64s)} image(s) already received"
        )
    return [{"b64_json": b} for b in b64s], usage


def est_cost(u):
    return (
        u["input_text_tokens"] * RATES["text_in"]
        + u["input_image_tokens"] * RATES["image_in"]
        + u["output_image_tokens"] * RATES["image_out"]
    ) / 1_000_000


def main():
    p = argparse.ArgumentParser(
        description="Generate game art via the OpenAI Images API."
    )
    p.add_argument("prompt", help="Text prompt describing the asset.")
    p.add_argument("--out")
    p.add_argument("--model", default="gpt-image-2")
    p.add_argument("--size", choices=["1024x1024", "1536x1024", "1024x1536", "auto"])
    p.add_argument("--quality", default="medium", choices=["auto", "low", "medium"])
    p.add_argument(
        "--background", choices=["auto", "opaque"]
    )  # gpt-image-2 rejects transparent
    p.add_argument("--format", default="png", choices=["png", "jpeg", "webp"])
    p.add_argument("--n", type=int, default=1)
    p.add_argument("--ref", action="append", default=[])
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--raw-json", action="store_true")
    args = p.parse_args()
    if not 1 <= args.n <= 10:
        p.error("--n must be between 1 and 10")

    # Shared parameters (string-valued; reused for JSON body or multipart fields).
    params = {"model": args.model, "prompt": args.prompt, "quality": args.quality}
    if args.size:
        params["size"] = args.size
    if args.background:
        params["background"] = args.background
    if args.format:
        params["output_format"] = args.format
    if args.n and args.n != 1:
        params["n"] = args.n

    key = resolve_key()
    headers = {"Authorization": f"Bearer {key}", "Accept": "text/event-stream"}

    if args.ref:
        # References -> the image *edit* endpoint (multipart). The reference images
        # are how a set keeps one consistent character/style (gpt-image-2 has no seed).
        fields = {k: str(v) for k, v in params.items()}
        fields["stream"] = "true"
        files = [("image[]", *load_ref(r)) for r in args.ref]
        if args.dry_run:
            print(
                json.dumps(
                    {
                        "endpoint": EDITS_URL,
                        "fields": fields,
                        "images": [f[1] for f in files],
                    },
                    indent=2,
                )
            )
            return
        ctype, mpbody = build_multipart(fields, files)
        headers["Content-Type"] = ctype
        req = urllib.request.Request(
            EDITS_URL, data=mpbody, method="POST", headers=headers
        )
    else:
        body = dict(params)
        body["stream"] = True
        if args.dry_run:
            print(json.dumps(body, indent=2))
            return
        headers["Content-Type"] = "application/json"
        req = urllib.request.Request(
            GENERATIONS_URL,
            data=json.dumps(body).encode(),
            method="POST",
            headers=headers,
        )

    items, usage = stream_images(req)
    if not items:
        sys.exit("ERROR: no image returned (no completed event received)")

    ext = args.format
    base_out = args.out or os.path.join(HERE, "out", f"{slugify(args.prompt)}.{ext}")
    os.makedirs(os.path.dirname(os.path.abspath(base_out)) or ".", exist_ok=True)

    written = []
    for i, item in enumerate(items):
        b64 = item.get("b64_json") or ""
        if not b64:
            print(f"  WARNING: item {i} had no image bytes")
            continue
        if b64.strip().startswith("data:") and "," in b64:
            b64 = b64.split(",", 1)[1]
        try:
            raw = base64.b64decode(b64)
        except (binascii.Error, ValueError):
            print(f"  WARNING: item {i} had undecodable image data")
            continue
        if len(items) > 1:
            stem, dot, e = base_out.rpartition(".")
            path = f"{stem}_{i + 1}.{e}" if dot else f"{base_out}_{i + 1}"
        else:
            path = base_out
        with open(path, "wb") as f:
            f.write(raw)
        written.append((path, len(raw)))

    for path, nbytes in written:
        print(f"saved {path}  ({nbytes // 1024} KB)")
    print(
        f"tokens: {usage['total_tokens']} total "
        f"(img-out {usage['output_image_tokens']})   est cost: ~${est_cost(usage):.4f}"
    )
    if args.raw_json:
        print(json.dumps(usage, indent=2))


if __name__ == "__main__":
    main()
