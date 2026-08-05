#!/usr/bin/env python3
"""Convert the renderer's PPM to a PNG, masked and scaled to look like the physical panel.

The GC9A01 is a 240x240 *round* display, so pixels outside the circle are never visible.
Masking them (rather than shipping a square image) is what makes the screenshot read as
a device photo. Upscaled 2x nearest-neighbour so individual pixels stay crisp on GitHub
instead of being smoothed away.

Preview tooling only.
"""
import sys
from PIL import Image, ImageDraw

SCALE = 2

def main(src, dst):
    img = Image.open(src).convert("RGB")
    w, h = img.size

    # Round bezel mask, supersampled so the rim is smooth rather than jagged.
    ss = 8
    mask = Image.new("L", (w * ss, h * ss), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, w * ss - 1, h * ss - 1), fill=255)
    mask = mask.resize((w * SCALE, h * SCALE), Image.LANCZOS)

    big = img.resize((w * SCALE, h * SCALE), Image.NEAREST)
    out = Image.new("RGBA", big.size, (0, 0, 0, 0))
    out.paste(big, (0, 0), mask)
    out.save(dst)

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
