#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
  cat <<'USAGE'
Usage:
  scripts/run_wallpaper_benchmark_report.sh <profile> <input_csv> [type]

Examples:
  scripts/run_wallpaper_benchmark_report.sh generic-esp32s3 /tmp/wallpaper_benchmark.csv animated
  scripts/run_wallpaper_benchmark_report.sh generic-esp32 /tmp/wallpaper_benchmark.csv lottie
USAGE
  exit 1
fi

profile="$1"
input_csv="$2"
wallpaper_type="${3:-animated}"

if [[ ! -f "$input_csv" ]]; then
  echo "Input CSV not found: $input_csv" >&2
  exit 2
fi

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
artifacts_dir="$repo_root/artifacts/wallpaper"
mkdir -p "$artifacts_dir"

artifact_prefix="${wallpaper_type}_benchmark"
if [[ "$wallpaper_type" == "gif" || "$wallpaper_type" == "animated" ]]; then
  artifact_prefix="gif_benchmark"
elif [[ "$wallpaper_type" == "lottie" ]]; then
  artifact_prefix="lottie_benchmark"
fi

csv_out="$artifacts_dir/${artifact_prefix}_${profile}.csv"
md_out="$artifacts_dir/${artifact_prefix}_summary_${profile}.md"

min_fps="24"
max_heap_kb="180"
if [[ "$wallpaper_type" == "lottie" ]]; then
  max_heap_kb="220"
fi

python3 "$repo_root/scripts/wallpaper_benchmark_report.py" \
  --input "$input_csv" \
  --profile "$profile" \
  --type "$wallpaper_type" \
  --min-fps "$min_fps" \
  --max-extra-heap-kb "$max_heap_kb" \
  --min-duration-s 60 \
  --export-csv "$csv_out" \
  --output "$md_out"

echo "Artifacts written:"
echo "  $csv_out"
echo "  $md_out"
