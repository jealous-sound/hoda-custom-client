#!/usr/bin/env python3
"""
crop.py — trim transparent padding around a sprite to a tight bounding box.

After keying out a background (keyout.py / despill_key.py), the art usually
sits inside a lot of empty transparent space. This finds the bounding box of
the non-transparent pixels and crops to it, keeping a small even margin.

USAGE:
  python3 crop.py IN.png [OUT.png] [--pad N] [--threshold N]
    --pad        margin in px kept on every side (default 8)
    --threshold  alpha <= this counts as transparent (default 8)
  OUT.png defaults to overwriting IN.png in place.
Reuses keyout.py's PNG reader/writer. Pure stdlib (no Pillow).
"""

import os
import sys

from keyout import read_png, unfilter, write_rgba_png


def main():
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    inp = args[0]
    out = None
    pad, thresh = 8, 8
    i = 1
    while i < len(args):
        a = args[i]
        if a == "--pad":
            pad = int(args[i + 1])
            i += 2
        elif a == "--threshold":
            thresh = int(args[i + 1])
            i += 2
        elif not a.startswith("--") and out is None:
            out = a
            i += 1
        else:
            sys.exit(f"ERROR: bad argument {a}")
    if out is None:
        out = inp  # crop in place

    w, h, ct, raw = read_png(inp)
    channels = 3 if ct == 2 else 4
    pix = unfilter(raw, w, h, channels)

    if channels == 4:
        rgba = bytearray(pix)
    else:  # RGB has no alpha to trim
        rgba = bytearray(w * h * 4)
        for p in range(w * h):
            rgba[p * 4 : p * 4 + 3] = pix[p * 3 : p * 3 + 3]
            rgba[p * 4 + 3] = 255

    minx, miny, maxx, maxy = w, h, -1, -1
    for y in range(h):
        row = y * w * 4
        for x in range(w):
            if rgba[row + x * 4 + 3] > thresh:
                if x < minx:
                    minx = x
                if x > maxx:
                    maxx = x
                if y < miny:
                    miny = y
                if y > maxy:
                    maxy = y
    if maxx < 0:
        sys.exit("ERROR: image is fully transparent, nothing to crop")

    minx = max(0, minx - pad)
    miny = max(0, miny - pad)
    maxx = min(w - 1, maxx + pad)
    maxy = min(h - 1, maxy + pad)
    nw, nh = maxx - minx + 1, maxy - miny + 1

    crop = bytearray(nw * nh * 4)
    for y in range(nh):
        s = (y + miny) * w * 4 + minx * 4
        d = y * nw * 4
        crop[d : d + nw * 4] = rgba[s : s + nw * 4]

    write_rgba_png(out, nw, nh, crop)
    print(f"cropped {inp} -> {out}")
    print(f"  {w}x{h} -> {nw}x{nh} (pad {pad}, alpha threshold {thresh})")


if __name__ == "__main__":
    main()
