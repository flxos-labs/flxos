#!/usr/bin/env python3
"""Guardrail: block new hardcoded lv_color_hex usage in UI/application code.

This check is intentionally strict for app/UI modules, with a small explicit allowlist
for modules where literal colors are part of the feature semantics.
"""

from __future__ import annotations

import pathlib
import re
import sys
from typing import Iterable

ROOT = pathlib.Path(__file__).resolve().parent.parent
TARGET_DIRS = (
    "Applications",
    "Apps",
    "UI",
)

ALLOWLIST_SUFFIXES = {
    "Applications/tools/implementation/DisplayTester.cpp",
}

COLOR_RE = re.compile(r"\blv_color_hex\s*\(")


def iter_files() -> Iterable[pathlib.Path]:
    for top in TARGET_DIRS:
        base = ROOT / top
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if not path.is_file():
                continue
            if path.suffix not in {".c", ".cc", ".cpp", ".h", ".hpp"}:
                continue
            yield path


def rel(path: pathlib.Path) -> str:
    return path.relative_to(ROOT).as_posix()


def is_allowlisted(path: pathlib.Path) -> bool:
    return rel(path) in ALLOWLIST_SUFFIXES


def main() -> int:
    violations: list[str] = []

    for path in iter_files():
        if is_allowlisted(path):
            continue

        text = path.read_text(encoding="utf-8", errors="ignore")
        for line_no, line in enumerate(text.splitlines(), start=1):
            if COLOR_RE.search(line):
                violations.append(f"{rel(path)}:{line_no}: {line.strip()}")

    if violations:
        print("Hardcoded color guardrail failed: use theme tokens/helpers instead of lv_color_hex().")
        print("Allowed exceptions:")
        for item in sorted(ALLOWLIST_SUFFIXES):
            print(f"  - {item}")
        print("\nViolations:")
        for item in violations:
            print(f"  {item}")
        return 1

    print("Literal color check passed (no restricted lv_color_hex usage found).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
