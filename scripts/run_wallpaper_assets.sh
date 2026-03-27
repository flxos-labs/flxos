#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
report_dir="$repo_root/reports"
mkdir -p "$report_dir"

python3 "$repo_root/scripts/generate_wallpaper_thumbnails.py"
python3 "$repo_root/scripts/validate_wallpaper_presets.py" --output "$report_dir/wallpaper_preset_validation.md"

echo "Wallpaper asset pipeline complete."
echo "Validation report: $report_dir/wallpaper_preset_validation.md"
