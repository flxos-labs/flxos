#!/usr/bin/env python3
"""
Generate baseline wallpaper preset thumbnails.

- Scans storage/data/wallpapers/presets/builtin/*/config.json
- Creates thumbnail PNGs when missing (or when --force is used)
- Ensures config metadata.thumbnail points to the generated file

This script intentionally avoids external dependencies (Pillow not required).
"""

from __future__ import annotations

import argparse
import json
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESETS_DIR = ROOT / "storage" / "data" / "wallpapers" / "presets" / "builtin"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate baseline wallpaper thumbnails")
    parser.add_argument("--force", action="store_true", help="Regenerate thumbnails even if they already exist")
    parser.add_argument("--size", type=int, default=96, help="Square thumbnail size in pixels")
    return parser.parse_args()


def png_chunk(chunk_type: bytes, data: bytes) -> bytes:
    crc = zlib.crc32(chunk_type + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + chunk_type + data + struct.pack(">I", crc)


def write_png_rgb(path: Path, width: int, height: int, rgb_rows: list[bytes]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    raw = b"".join(b"\x00" + row for row in rgb_rows)  # filter type 0 per row

    signature = b"\x89PNG\r\n\x1a\n"
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)  # 8-bit RGB
    idat = zlib.compress(raw, level=9)

    with path.open("wb") as f:
        f.write(signature)
        f.write(png_chunk(b"IHDR", ihdr))
        f.write(png_chunk(b"IDAT", idat))
        f.write(png_chunk(b"IEND", b""))


def palette_rgb(name: str) -> tuple[tuple[int, int, int], tuple[int, int, int], tuple[int, int, int]]:
    if name == "cool":
        return ((16, 42, 94), (45, 160, 210), (204, 235, 255))
    if name == "sunset":
        return ((90, 25, 40), (220, 90, 40), (255, 210, 120))
    return ((40, 24, 100), (110, 70, 220), (255, 160, 80))


def infer_palette(config: dict) -> str:
    source = str(config.get("source", ""))
    source_l = source.lower()
    if "palette=sunset" in source_l or "sunset" in source_l:
        return "sunset"
    if "palette=cool" in source_l or "perlin" in source_l:
        return "cool"
    return "vivid"


def make_gradient_rows(size: int, colors: tuple[tuple[int, int, int], tuple[int, int, int], tuple[int, int, int]]) -> list[bytes]:
    c0, c1, c2 = colors
    rows: list[bytes] = []
    for y in range(size):
        t = y / max(1, size - 1)
        if t < 0.5:
            k = t * 2.0
            a, b = c0, c1
        else:
            k = (t - 0.5) * 2.0
            a, b = c1, c2

        row = bytearray()
        for x in range(size):
            w = (x / max(1, size - 1)) * 0.35
            kk = min(1.0, max(0.0, k + w - 0.15))
            r = int(a[0] + (b[0] - a[0]) * kk)
            g = int(a[1] + (b[1] - a[1]) * kk)
            bch = int(a[2] + (b[2] - a[2]) * kk)
            row.extend((r, g, bch))
        rows.append(bytes(row))
    return rows


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def save_json(path: Path, payload: dict) -> None:
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def process_preset(config_path: Path, size: int, force: bool) -> tuple[bool, str]:
    config = load_json(config_path)
    preset_id = str(config.get("id", config_path.parent.name))

    metadata = config.setdefault("metadata", {})
    thumb_vfs = metadata.get("thumbnail")
    if not thumb_vfs:
        thumb_vfs = f"/data/wallpapers/presets/builtin/{preset_id}/thumbnail.png"
        metadata["thumbnail"] = thumb_vfs

    thumb_rel = str(thumb_vfs).lstrip("/")
    thumb_abs = ROOT / "storage" / thumb_rel

    changed = False
    if force or not thumb_abs.exists():
        palette = infer_palette(config)
        colors = palette_rgb(palette)
        rows = make_gradient_rows(size, colors)
        write_png_rgb(thumb_abs, size, size, rows)
        changed = True

    if config.get("metadata", {}).get("thumbnail") != thumb_vfs:
        config.setdefault("metadata", {})["thumbnail"] = thumb_vfs
        changed = True

    if changed:
        save_json(config_path, config)

    return changed, str(thumb_abs)


def main() -> int:
    args = parse_args()

    if not PRESETS_DIR.exists():
        return 0

    configs = sorted(PRESETS_DIR.glob("*/config.json"))
    if not configs:
        print("No preset config.json files found.")
        return 0

    changed_count = 0
    for cfg in configs:
        changed, out = process_preset(cfg, args.size, args.force)
        status = "generated" if changed else "kept"
        print(f"[{status}] {cfg.parent.name} -> {out}")
        if changed:
            changed_count += 1

    print(f"Done. Processed {len(configs)} presets, updated {changed_count}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
