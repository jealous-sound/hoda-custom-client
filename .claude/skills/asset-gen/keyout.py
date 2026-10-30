#!/usr/bin/env python3
"""
keyout.py — Make a solid background color transparent in a PNG (chroma key).

gpt-image-2 cannot output transparent backgrounds, so the workflow is:
generate sprites on a flat solid background (e.g. magenta #FF00FF), then run
this to bake an RGBA PNG with that color knocked out to alpha 0.

Standard library only (uses zlib, which ships with Python) — no Pillow needed.

USAGE:
  python3 keyout.py IN.png [OUT.png] [--key R,G,B] [--threshold N]

  python3 keyout.py out/knight.png out/knight_rgba.png --key 255,0,255 --threshold 60

  --key R,G,B     Background color to remove. Default: AUTO-DETECT from the image
                  border (recommended — image models do NOT honor exact hex
                  requests, so the real background is rarely the color you asked
                  for). Pass an explicit R,G,B to override.
  --threshold N   Chebyshev color distance (0-255) counted as background.
                  Higher = removes more (and more anti-aliased edge pixels).
                  Default 70.

Supports 8-bit non-interlaced RGB / RGBA PNGs (what the image API returns).
Note: raylib can do the same at load time without this tool — see SKILL.md.
"""

import binascii
import os
import struct
import sys
import zlib


def read_png(path):
    try:
        with open(path, "rb") as f:
            data = f.read()
    except OSError as e:
        sys.exit(f"ERROR: cannot read {path}: {e}")
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        sys.exit(f"ERROR: {path} is not a PNG")
    pos, idat = 8, bytearray()
    w = h = bd = ct = il = None
    while pos + 8 <= len(data):  # need 4-byte length + 4-byte type
        (ln,) = struct.unpack(">I", data[pos : pos + 4])
        typ = data[pos + 4 : pos + 8]
        start, end = pos + 8, pos + 8 + ln
        if end > len(data):
            sys.exit(f"ERROR: {path} is truncated (chunk {typ} runs past end of file)")
        chunk = data[start:end]
        pos = end + 4  # skip the 4-byte CRC
        if typ == b"IHDR":
            if len(chunk) < 13:
                sys.exit(f"ERROR: {path} has a malformed IHDR")
            w, h, bd, ct, _comp, _filt, il = struct.unpack(">IIBBBBB", chunk[:13])
        elif typ == b"IDAT":
            idat += chunk
        elif typ == b"IEND":
            break
    if w is None:
        sys.exit(f"ERROR: {path} has no IHDR header (not a valid PNG)")
    if bd != 8 or ct not in (2, 6) or il != 0:
        sys.exit(
            f"ERROR: unsupported PNG (bitdepth={bd}, colortype={ct}, "
            f"interlace={il}); need 8-bit non-interlaced RGB/RGBA."
        )
    if not idat:
        sys.exit(f"ERROR: {path} has no image data (IDAT)")
    try:
        raw = zlib.decompress(bytes(idat))
    except zlib.error as e:
        sys.exit(f"ERROR: {path} has corrupt compressed image data: {e}")
    channels = 3 if ct == 2 else 4
    expected = h * (1 + w * channels)  # 1 filter byte + scanline per row
    if len(raw) < expected:
        sys.exit(
            f"ERROR: {path} image data is short ({len(raw)} < {expected} bytes) "
            f"— truncated PNG"
        )
    return w, h, ct, raw


def unfilter(raw, w, h, channels):
    stride = w * channels
    bpp = channels
    out = bytearray()
    prev = bytearray(stride)
    i = 0
    for _y in range(h):
        ft = raw[i]
        i += 1
        line = bytearray(raw[i : i + stride])
        i += stride
        if ft == 1:  # Sub
            for x in range(bpp, stride):
                line[x] = (line[x] + line[x - bpp]) & 255
        elif ft == 2:  # Up
            for x in range(stride):
                line[x] = (line[x] + prev[x]) & 255
        elif ft == 3:  # Average
            for x in range(stride):
                a = line[x - bpp] if x >= bpp else 0
                line[x] = (line[x] + ((a + prev[x]) >> 1)) & 255
        elif ft == 4:  # Paeth
            for x in range(stride):
                a = line[x - bpp] if x >= bpp else 0
                b = prev[x]
                c = prev[x - bpp] if x >= bpp else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[x] = (line[x] + pr) & 255
        elif ft != 0:
            sys.exit(f"ERROR: unknown PNG filter type {ft}")
        out += line
        prev = line
    return out


def chunk(typ, data):
    return (
        struct.pack(">I", len(data))
        + typ
        + data
        + struct.pack(">I", binascii.crc32(typ + data) & 0xFFFFFFFF)
    )


def write_rgba_png(path, w, h, rgba):
    stride = w * 4
    buf = bytearray()
    for y in range(h):
        buf.append(0)  # filter type 0 (None)
        buf += rgba[y * stride : (y + 1) * stride]
    out = b"\x89PNG\r\n\x1a\n"
    out += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    out += chunk(b"IDAT", zlib.compress(bytes(buf), 9))
    out += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(out)


def detect_bg(pix, w, h, channels):
    """Most common color along the 1px border — the flat background fill."""
    counts = {}

    def sample(x, y):
        s = (y * w + x) * channels
        c = (pix[s], pix[s + 1], pix[s + 2])
        counts[c] = counts.get(c, 0) + 1

    for x in range(w):
        sample(x, 0)
        sample(x, h - 1)
    for y in range(h):
        sample(0, y)
        sample(w - 1, y)
    return max(counts, key=counts.get)


def main():
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    inp = args[0]
    out = None
    key = None  # None -> auto-detect from border
    thr = 70
    i = 1
    while i < len(args):
        a = args[i]
        if a == "--key":
            if i + 1 >= len(args):
                sys.exit("ERROR: --key needs a value (R,G,B or 'auto')")
            v = args[i + 1]
            i += 2
            if v == "auto":
                key = None
            else:
                try:
                    key = tuple(int(x) for x in v.split(","))
                except ValueError:
                    sys.exit("ERROR: --key must be R,G,B integers, e.g. 255,0,255")
                if len(key) != 3 or not all(0 <= c <= 255 for c in key):
                    sys.exit("ERROR: --key must be three integers 0-255 (R,G,B)")
        elif a == "--threshold":
            if i + 1 >= len(args):
                sys.exit("ERROR: --threshold needs a value (0-255)")
            try:
                thr = int(args[i + 1])
            except ValueError:
                sys.exit("ERROR: --threshold must be an integer 0-255")
            i += 2
            if not 0 <= thr <= 255:
                sys.exit("ERROR: --threshold must be 0-255")
        elif not a.startswith("--") and out is None:
            out = a
            i += 1
        else:
            sys.exit(f"ERROR: bad argument {a}")
    if out is None:
        out = os.path.splitext(inp)[0] + "_rgba.png"

    w, h, ct, raw = read_png(inp)
    channels = 3 if ct == 2 else 4
    pix = unfilter(raw, w, h, channels)

    if key is None:
        key = detect_bg(pix, w, h, channels)
        print(f"  auto-detected background: rgb{key}")
    kr, kg, kb = key
    rgba = bytearray(w * h * 4)
    n_clear = 0
    for p in range(w * h):
        s = p * channels
        r, g, b = pix[s], pix[s + 1], pix[s + 2]
        a = pix[s + 3] if channels == 4 else 255
        if max(abs(r - kr), abs(g - kg), abs(b - kb)) <= thr:
            a = 0
            n_clear += 1
        d = p * 4
        rgba[d] = r
        rgba[d + 1] = g
        rgba[d + 2] = b
        rgba[d + 3] = a

    write_rgba_png(out, w, h, rgba)
    pct = 100.0 * n_clear / (w * h)
    print(f"keyed {inp} -> {out}")
    print(
        f"  {w}x{h}, removed {n_clear} px ({pct:.1f}%) within {thr} of "
        f"rgb{key} -> alpha 0"
    )


if __name__ == "__main__":
    main()
