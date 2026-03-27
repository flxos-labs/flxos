#!/usr/bin/env python3
"""
Generate pass/fail summary for wallpaper benchmark CSV captures.

Expected CSV columns:
- timestamp_ms
- type
- fps
- avg_frame_ms
- p95_frame_ms
- max_frame_ms
- extra_heap_bytes
- watchdog_incidents
- source
"""

from __future__ import annotations

import argparse
import csv
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


@dataclass
class Sample:
    timestamp_ms: int
    wallpaper_type: str
    fps: float
    avg_frame_ms: float
    p95_frame_ms: float
    max_frame_ms: float
    extra_heap_bytes: int
    watchdog_incidents: int
    source: str


@dataclass
class Summary:
    profile: str
    rows: int
    duration_s: float
    min_fps: float
    avg_fps: float
    max_p95_frame_ms: float
    peak_extra_heap_bytes: int
    watchdog_incidents: int
    pass_fps: bool
    pass_heap: bool
    pass_watchdog: bool
    pass_duration: bool

    @property
    def passed(self) -> bool:
        return self.pass_fps and self.pass_heap and self.pass_watchdog and self.pass_duration


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Summarize wallpaper benchmark CSV files.")
    parser.add_argument("--input", required=True, help="Input benchmark CSV path")
    parser.add_argument("--profile", required=True, help="Board/profile name for report")
    parser.add_argument("--output", required=True, help="Output markdown path")
    parser.add_argument("--type", default="animated", choices=["animated", "gif", "lottie"], help="Filter benchmark type")
    parser.add_argument("--export-csv", default="", help="Optional output path for filtered CSV artifact")
    parser.add_argument("--min-fps", type=float, default=24.0, help="Minimum sustained fps")
    parser.add_argument("--max-extra-heap-kb", type=int, default=180, help="Maximum allowed extra heap in KB")
    parser.add_argument("--min-duration-s", type=float, default=60.0, help="Minimum captured duration in seconds")
    return parser.parse_args()


def _to_int(raw: str, default: int = 0) -> int:
    try:
        return int(raw)
    except (TypeError, ValueError):
        return default


def _to_float(raw: str, default: float = 0.0) -> float:
    try:
        return float(raw)
    except (TypeError, ValueError):
        return default


def read_samples(csv_path: Path, wallpaper_type: str) -> list[Sample]:
    samples: list[Sample] = []
    with csv_path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            row_type = (row.get("type") or "").strip().lower()
            if row_type != wallpaper_type:
                continue
            timestamp_ms = _to_int(row.get("timestamp_ms"), -1)
            if timestamp_ms < 0:
                continue

            p95_raw = row.get("p95_frame_ms")
            if p95_raw is None or p95_raw == "":
                # Backward compatibility for early CSV format.
                p95_value = _to_float(row.get("max_frame_ms"), 0.0)
            else:
                p95_value = _to_float(p95_raw, 0.0)

            samples.append(
                Sample(
                    timestamp_ms=timestamp_ms,
                    wallpaper_type=row_type,
                    fps=_to_float(row.get("fps"), 0.0),
                    avg_frame_ms=_to_float(row.get("avg_frame_ms"), 0.0),
                    p95_frame_ms=p95_value,
                    max_frame_ms=_to_float(row.get("max_frame_ms"), 0.0),
                    extra_heap_bytes=_to_int(row.get("extra_heap_bytes"), 0),
                    watchdog_incidents=_to_int(row.get("watchdog_incidents"), 0),
                    source=(row.get("source") or "").strip(),
                )
            )

    samples.sort(key=lambda sample: sample.timestamp_ms)
    return samples


def estimate_duration_seconds(samples: Iterable[Sample]) -> float:
    seq = list(samples)
    if not seq:
        return 0.0
    if len(seq) == 1:
        return 1.0
    return max(0.0, (seq[-1].timestamp_ms - seq[0].timestamp_ms) / 1000.0)


def summarize(samples: list[Sample], profile: str, min_fps: float, max_extra_heap_kb: int, min_duration_s: float) -> Summary:
    if not samples:
        return Summary(
            profile=profile,
            rows=0,
            duration_s=0.0,
            min_fps=0.0,
            avg_fps=0.0,
            max_p95_frame_ms=0.0,
            peak_extra_heap_bytes=0,
            watchdog_incidents=0,
            pass_fps=False,
            pass_heap=False,
            pass_watchdog=False,
            pass_duration=False,
        )

    duration_s = estimate_duration_seconds(samples)
    min_observed_fps = min(sample.fps for sample in samples)
    avg_observed_fps = sum(sample.fps for sample in samples) / len(samples)
    max_p95 = max(sample.p95_frame_ms for sample in samples)
    peak_extra_heap = max(sample.extra_heap_bytes for sample in samples)
    watchdog_total = sum(sample.watchdog_incidents for sample in samples)

    max_extra_heap_bytes = max_extra_heap_kb * 1024

    return Summary(
        profile=profile,
        rows=len(samples),
        duration_s=duration_s,
        min_fps=min_observed_fps,
        avg_fps=avg_observed_fps,
        max_p95_frame_ms=max_p95,
        peak_extra_heap_bytes=peak_extra_heap,
        watchdog_incidents=watchdog_total,
        pass_fps=min_observed_fps >= min_fps,
        pass_heap=peak_extra_heap <= max_extra_heap_bytes,
        pass_watchdog=watchdog_total == 0,
        pass_duration=duration_s >= min_duration_s,
    )


def format_status(ok: bool) -> str:
    return "PASS" if ok else "FAIL"


def write_filtered_csv(samples: list[Sample], output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow([
            "timestamp_ms",
            "type",
            "fps",
            "avg_frame_ms",
            "p95_frame_ms",
            "max_frame_ms",
            "extra_heap_bytes",
            "watchdog_incidents",
            "source",
        ])
        for sample in samples:
            writer.writerow([
                sample.timestamp_ms,
                sample.wallpaper_type,
                f"{sample.fps:.2f}",
                f"{sample.avg_frame_ms:.2f}",
                f"{sample.p95_frame_ms:.2f}",
                f"{sample.max_frame_ms:.2f}",
                sample.extra_heap_bytes,
                sample.watchdog_incidents,
                sample.source,
            ])


def write_markdown(summary: Summary, output: Path, min_fps: float, max_extra_heap_kb: int, min_duration_s: float, source_csv: Path, wallpaper_type: str) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)

    peak_kb = summary.peak_extra_heap_bytes / 1024.0
    lines = [
        f"# Wallpaper Benchmark Summary: {summary.profile}",
        "",
        f"- Source CSV: {source_csv}",
        f"- Wallpaper type: {wallpaper_type}",
        f"- Samples: {summary.rows}",
        f"- Captured duration: {summary.duration_s:.1f}s (required >= {min_duration_s:.1f}s)",
        "",
        "## Acceptance Gates",
        "",
        f"- FPS gate: {format_status(summary.pass_fps)} (min observed {summary.min_fps:.2f}, required >= {min_fps:.2f})",
        f"- Heap gate: {format_status(summary.pass_heap)} (peak {peak_kb:.1f} KB, required <= {max_extra_heap_kb} KB)",
        f"- Watchdog gate: {format_status(summary.pass_watchdog)} (incidents {summary.watchdog_incidents}, required 0)",
        f"- Duration gate: {format_status(summary.pass_duration)} (captured {summary.duration_s:.1f}s)",
        "",
        "## Metrics",
        "",
        f"- Avg FPS: {summary.avg_fps:.2f}",
        f"- Min FPS: {summary.min_fps:.2f}",
        f"- Max P95 frame time: {summary.max_p95_frame_ms:.2f} ms",
        f"- Peak extra heap: {summary.peak_extra_heap_bytes} bytes",
        "",
        "## Final Result",
        "",
        f"- Overall: {format_status(summary.passed)}",
    ]

    output.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    args = parse_args()

    input_path = Path(args.input)
    output_path = Path(args.output)

    if not input_path.exists():
        print(f"Input CSV not found: {input_path}")
        return 2

    samples = read_samples(input_path, args.type)
    summary = summarize(
        samples=samples,
        profile=args.profile,
        min_fps=args.min_fps,
        max_extra_heap_kb=args.max_extra_heap_kb,
        min_duration_s=args.min_duration_s,
    )

    write_markdown(
        summary=summary,
        output=output_path,
        min_fps=args.min_fps,
        max_extra_heap_kb=args.max_extra_heap_kb,
        min_duration_s=args.min_duration_s,
        source_csv=input_path,
        wallpaper_type=args.type,
    )

    if args.export_csv:
        export_path = Path(args.export_csv)
        write_filtered_csv(samples, export_path)
        print(f"Wrote filtered CSV: {export_path}")

    print(f"Wrote summary: {output_path}")
    return 0 if summary.passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
