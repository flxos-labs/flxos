#!/usr/bin/env python3
"""
Validate wallpaper preset release gates.

Gates implemented:
- config.json parseable and required top-level fields present
- metadata.thumbnail path present and file exists
- attribution metadata present: metadata.author + metadata.license
- source size envelope for file-based types:
  - static <= 350KB
  - animated/gif <= 1.2MB
  - lottie <= 250KB
- perf envelope metadata present:
  - metadata.perf.min_fps
  - metadata.perf.max_extra_heap_kb

Dynamic presets are exempt from file-size checks because source is algorithmic.
"""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESETS_DIR = ROOT / "storage" / "data" / "wallpapers" / "presets" / "builtin"

SIZE_LIMITS = {
    "static": 350 * 1024,
    "animated": int(1.2 * 1024 * 1024),
    "gif": int(1.2 * 1024 * 1024),
    "lottie": 250 * 1024,
}


@dataclass
class Issue:
    preset: str
    message: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate wallpaper preset release gates")
    parser.add_argument("--output", default="", help="Optional markdown report output path")
    return parser.parse_args()


def to_abs_storage(vfs_path: str) -> Path:
    return ROOT / "storage" / vfs_path.lstrip("/")


def validate_preset(config_path: Path) -> list[Issue]:
    issues: list[Issue] = []
    preset_id = config_path.parent.name

    try:
        config = json.loads(config_path.read_text(encoding="utf-8"))
    except Exception as exc:
        return [Issue(preset_id, f"Invalid JSON: {exc}")]

    for field in ("id", "name", "type", "source", "metadata"):
        if field not in config:
            issues.append(Issue(preset_id, f"Missing required field: {field}"))

    metadata = config.get("metadata", {}) if isinstance(config.get("metadata", {}), dict) else {}

    thumb = str(metadata.get("thumbnail", "")).strip()
    if not thumb:
        issues.append(Issue(preset_id, "Missing metadata.thumbnail"))
    else:
        thumb_abs = to_abs_storage(thumb)
        if not thumb_abs.exists():
            issues.append(Issue(preset_id, f"Thumbnail file missing: {thumb_abs}"))

    author = str(metadata.get("author", "")).strip()
    license_name = str(metadata.get("license", "")).strip()
    if not author:
        issues.append(Issue(preset_id, "Missing metadata.author"))
    if not license_name:
        issues.append(Issue(preset_id, "Missing metadata.license"))

    perf = metadata.get("perf", {}) if isinstance(metadata.get("perf", {}), dict) else {}
    if "min_fps" not in perf:
        issues.append(Issue(preset_id, "Missing metadata.perf.min_fps"))
    if "max_extra_heap_kb" not in perf:
        issues.append(Issue(preset_id, "Missing metadata.perf.max_extra_heap_kb"))

    preset_type = str(config.get("type", "")).strip().lower()
    source = str(config.get("source", "")).strip()
    limit = SIZE_LIMITS.get(preset_type)

    if limit is not None and source and not source.startswith("algo://"):
        source_abs = to_abs_storage(source)
        if not source_abs.exists():
            issues.append(Issue(preset_id, f"Source file missing: {source_abs}"))
        else:
            size = source_abs.stat().st_size
            if size > limit:
                issues.append(Issue(preset_id, f"Source size {size} exceeds limit {limit} bytes for type {preset_type}"))

    return issues


def write_report(path: Path, issues: list[Issue], total: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "# Wallpaper Preset Validation Report",
        "",
        f"- Presets checked: {total}",
        f"- Issues: {len(issues)}",
        "",
    ]

    if not issues:
        lines.append("✅ All release-gate checks passed.")
    else:
        lines.append("## Issues")
        lines.append("")
        for issue in issues:
            lines.append(f"- [{issue.preset}] {issue.message}")

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    args = parse_args()

    if not PRESETS_DIR.exists():
        if args.output:
            write_report(Path(args.output), [], 0)
        return 0

    configs = sorted(PRESETS_DIR.glob("*/config.json"))
    if not configs:
        print(f"No preset config files found under: {PRESETS_DIR}")
        if args.output:
            write_report(Path(args.output), [], 0)
        return 0

    issues: list[Issue] = []
    for cfg in configs:
        issues.extend(validate_preset(cfg))

    if args.output:
        write_report(Path(args.output), issues, len(configs))

    if issues:
        for issue in issues:
            print(f"[FAIL] {issue.preset}: {issue.message}")
        return 1

    print(f"All {len(configs)} presets passed release-gate validation.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
