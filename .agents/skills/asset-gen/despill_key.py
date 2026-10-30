#!/usr/bin/env python3
"""
despill_key.py — chroma key with green-spill removal + feathered alpha.

keyout.py does a hard binary cut, which leaves a green-tinted ring on the
anti-aliased edge (gold+green blend pixels that aren't "green enough" to drop).
This pass:
  1) feathers alpha across an inner..outer distance band (soft edge), and
  2) despills: clamps the green channel down to max(R,B) so any residual green
     tint becomes neutral. Safe for gold/yellow art (red >= green already).

USAGE:
  python3 despill_key.py IN.png [OUT.png] [--inner N] [--outer N]
    --inner  distance fully transparent at/below (default 70)
    --outer  distance fully opaque at/above   (default 140)
Reuses keyout.py's PNG reader/writer.
"""

import os
import sys

from keyout import read_png, unfilter, write_rgba_png, detect_bg


def main():
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    inp = args[0]
    out = None
    inner, outer = 70, 140
    i = 1
    while i < len(args):
        a = args[i]
        if a == "--inner":
            inner = int(args[i + 1])
            i += 2
        elif a == "--outer":
            outer = int(args[i + 1])
            i += 2
        elif not a.startswith("--") and out is None:
            out = a
            i += 1
        else:
            sys.exit(f"ERROR: bad argument {a}")
    if out is None:
        out = os.path.splitext(inp)[0] + "_rgba.png"
    if outer <= inner:
        sys.exit("ERROR: --outer must be greater than --inner")

    w, h, ct, raw = read_png(inp)
    channels = 3 if ct == 2 else 4
    pix = unfilter(raw, w, h, channels)
    kr, kg, kb = detect_bg(pix, w, h, channels)
    print(f"  background: rgb({kr},{kg},{kb})  feather {inner}..{outer}")

    rgba = bytearray(w * h * 4)
    cleared = despilled = 0
    span = outer - inner
    for p in range(w * h):
        s = p * channels
        r, g, b = pix[s], pix[s + 1], pix[s + 2]
        d = max(abs(r - kr), abs(g - kg), abs(b - kb))
        if d <= inner:
            a = 0
            cleared += 1
        elif d >= outer:
            a = 255
        else:
            a = int(255 * (d - inner) / span)
        # despill: kill any green tint by clamping G to the max of R/B
        cap = r if r > b else b
        if g > cap:
            g = cap
            despilled += 1
        o = p * 4
        rgba[o] = r
        rgba[o + 1] = g
        rgba[o + 2] = b
        rgba[o + 3] = a

    write_rgba_png(out, w, h, rgba)
    print(f"keyed {inp} -> {out}")
    print(f"  {w}x{h}, {cleared} px transparent, {despilled} px despilled")


if __name__ == "__main__":
    main()
