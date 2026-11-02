#!/usr/bin/env python3
"""Turns frames recorded by hudsim into pictures.

  python3 tools/hudsim/render.py build/hudsim/frames.jsonl docs/images/screens

Writes every frame to <out>/all/, plus overview.png (the main screens with
captions) and demo.gif (the demo mode playing, at double speed).
Needs Pillow (pip install pillow).
"""
from __future__ import annotations

import json
import sys
from collections import defaultdict
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ON = (236, 246, 255)
OFF = (8, 10, 14)
BEZEL = (28, 31, 36)
SCALE = 4
BORDER = 12

# Which frame of each scene goes on the overview sheet, and its caption.
OVERVIEW = [
    ("splash", 0, "Starting up"),
    ("home_no_phone", -1, "Home, waiting for the iPhone"),
    ("home", -1, "Home: date, time, weather"),
    ("message", 0, "A message pops up"),
    ("claude", 0, "Claude answer, first page"),
    ("claude", 3, "...scrolls by itself"),
    ("call", 0, "Incoming call"),
    ("long_title", 0, "Long title wraps (accents kept)"),
    ("mirror_toast", 0, "Mirrored for the beam splitter"),
]


def bits(hexstr: str) -> list[list[int]]:
    data = bytes.fromhex(hexstr)
    px = [[0] * 128 for _ in range(64)]
    for page in range(8):
        for x in range(128):
            b = data[page * 128 + x]
            for bit in range(8):
                if (b >> bit) & 1:
                    px[page * 8 + bit][x] = 1
    return px


def oled(px: list[list[int]], scale: int = SCALE) -> Image.Image:
    w, h = 128 * scale + 2 * BORDER, 64 * scale + 2 * BORDER
    img = Image.new("RGB", (w, h), BEZEL)
    d = ImageDraw.Draw(img)
    d.rectangle([BORDER - 2, BORDER - 2, w - BORDER + 1, h - BORDER + 1], fill=OFF)
    gap = 1 if scale >= 3 else 0
    for y in range(64):
        row = px[y]
        for x in range(128):
            if row[x]:
                x0, y0 = BORDER + x * scale, BORDER + y * scale
                d.rectangle([x0, y0, x0 + scale - 1 - gap, y0 + scale - 1 - gap], fill=ON)
    return img


def font(size: int) -> ImageFont.ImageFont:
    for name in ("DejaVuSans.ttf", "LiberationSans-Regular.ttf", "Arial.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def main() -> None:
    src, out = Path(sys.argv[1]), Path(sys.argv[2])
    (out / "all").mkdir(parents=True, exist_ok=True)
    scenes: dict[str, list[tuple[int, Image.Image]]] = defaultdict(list)
    for line in src.read_text().splitlines():
        rec = json.loads(line)
        scenes[rec["scene"]].append((rec["t"], oled(bits(rec["frame"]))))

    for name, frames in scenes.items():
        for i, (_, img) in enumerate(frames):
            img.save(out / "all" / f"{name}_{i:02d}.png", optimize=True)

    # Overview sheet: 3 columns of captioned screens.
    tiles = []
    for name, idx, caption in OVERVIEW:
        frames = scenes.get(name)
        if frames:
            tiles.append((frames[idx][1], caption))
    tw, th = tiles[0][0].size
    cap_h, pad, cols = 34, 16, 3
    rows = (len(tiles) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * tw + (cols + 1) * pad, rows * (th + cap_h) + (rows + 1) * pad), (245, 247, 246))
    d = ImageDraw.Draw(sheet)
    f = font(17)
    for i, (img, caption) in enumerate(tiles):
        x = pad + (i % cols) * (tw + pad)
        y = pad + (i // cols) * (th + cap_h + pad)
        sheet.paste(img, (x, y))
        d.text((x + 2, y + th + 8), caption, fill=(30, 36, 34), font=f)
    sheet.save(out / "overview.png", optimize=True)

    # Demo GIF at double speed.
    demo = scenes.get("demo", [])
    if demo:
        imgs = [img.resize((img.width // 2, img.height // 2), Image.NEAREST) for _, img in demo]
        durations = []
        for i, (t, _) in enumerate(demo):
            nxt = demo[i + 1][0] if i + 1 < len(demo) else t + 3000
            durations.append(max(60, (nxt - t) // 2))
        imgs[0].save(out / "demo.gif", save_all=True, append_images=imgs[1:], duration=durations, loop=0,
                     optimize=True)
    print(f"{sum(len(v) for v in scenes.values())} frames from {len(scenes)} scenes -> {out}")


if __name__ == "__main__":
    main()
