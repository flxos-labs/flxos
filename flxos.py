#!/usr/bin/env python3
"""
FlxOS CLI — Unified build tool.

Usage:
    python flxos.py list [--json]            List all profiles
    python flxos.py select <id>              Select profile for build
    python flxos.py build [--all] [--dev]    Build current/all profiles
    python flxos.py validate [id]            Validate profile YAMLs
    python flxos.py info <id>                Show profile details
    python flxos.py new <id>                 Scaffold profile YAML
    python flxos.py diff <a> <b> [--json]    Compare two profiles
    python flxos.py hwgen [id] [--all]       Generate HWD init scaffold from profile.yaml
    python flxos.py flash [--port]           Flash current build
    python flxos.py release <version>        Package release artifacts
    python flxos.py cdn <version>            Generate ESP Web Tools manifests
    python flxos.py doctor                   Check build environment
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any, Optional


# ── Constants ──────────────────────────────────────────────────────────────

SCRIPT_DIR = Path(__file__).parent.resolve()
BUILD_DIR = SCRIPT_DIR / "build"
RELEASES_DIR = SCRIPT_DIR / "releases"
PROFILES_DIR = SCRIPT_DIR / "Profiles"
SCHEMA_FILE = PROFILES_DIR / "schema.yaml"
SDKCONFIG_FILE = SCRIPT_DIR / "sdkconfig"
SDKCONFIG_DEFAULTS = SCRIPT_DIR / "sdkconfig.defaults"

# ANSI Colors
C_RESET = "\033[0m"
C_BOLD = "\033[1m"
C_DIM = "\033[2m"
C_RED = "\033[31m"
C_GREEN = "\033[32m"
C_YELLOW = "\033[33m"
C_CYAN = "\033[36m"
C_MAGENTA = "\033[35m"
C_WHITE = "\033[37m"


# ── YAML Parser (minimal, matches profile.cmake's parser) ─────────────────

def parse_yaml(filepath: Path) -> dict:
    """Parse a simple YAML file into a nested dict.
    Supports: scalars, nested maps, arrays. No flow mappings or anchors.
    """
    result = {}
    stack = [(result, -1)]  # (current_dict, indent_level)

    with open(filepath) as f:
        for line in f:
            # Skip comments and blank lines
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue

            indent = len(line) - len(line.lstrip())

            # Pop stack to find parent
            while len(stack) > 1 and stack[-1][1] >= indent:
                stack.pop()

            current = stack[-1][0]

            # Array item
            if stripped.startswith("- "):
                val = stripped[2:].strip().strip('"').strip("'")
                # Normalize booleans in arrays
                if isinstance(val, str):
                    if val.lower() in ("true", "yes"):
                        val = True
                    elif val.lower() in ("false", "no"):
                        val = False

                if isinstance(current, list):
                    current.append(val)
                elif isinstance(current, dict):
                    # Find the last key added and convert its value to a list
                    parent_dict, _ = stack[-2] if len(stack) > 1 else (result, -1)
                    # Find key in parent that points to current
                    for k, v in parent_dict.items():
                        if v is current:
                            parent_dict[k] = [val]
                            # Update the stack to point to the new list
                            stack[-1] = (parent_dict[k], stack[-1][1])
                            break
                continue

            # Key: value
            match = re.match(r'^([a-zA-Z0-9_-]+):\s*(.*)', stripped)
            if match:
                key = match.group(1).replace("-", "_")
                val = match.group(2).strip()

                # Remove inline comments
                val = re.sub(r'\s+#.*$', '', val)
                # Strip quotes
                val = val.strip('"').strip("'")

                if val == "" or val is None:
                    # Nested map or array
                    new_dict = {}
                    current[key] = new_dict
                    stack.append((new_dict, indent))
                elif val.startswith("[") and val.endswith("]"):
                    # Inline array
                    items = [x.strip().strip('"').strip("'") for x in val[1:-1].split(",")]
                    current[key] = items
                else:
                    # Check for array marker on next line (handled by array item above)
                    # Normalize booleans
                    if val.lower() in ("true", "yes"):
                        val = True
                    elif val.lower() in ("false", "no"):
                        val = False
                    elif val.lower() == "null":
                        val = None
                    else:
                        # Try int
                        try:
                            val = int(val)
                        except ValueError:
                            try:
                                val = float(val)
                            except ValueError:
                                pass
                    current[key] = val

    return result


def get_nested(d: dict, key: str, default=None):
    """Get a nested dict value by dot-separated key."""
    keys = key.split(".")
    for k in keys:
        if isinstance(d, dict) and k in d:
            d = d[k]
        else:
            return default
    return d


def load_schema() -> dict:
    """Load validation schema metadata from Profiles/schema.yaml."""
    default_schema = {
        "required_fields": ["id", "vendor", "name", "target", "flash_size"],
        "enums": {
            "target": ["esp32", "esp32s3", "esp32c6", "esp32p4"],
            "flash_size": ["4MB", "8MB", "16MB"],
            "flash_mode": ["QIO", "DIO", "QOUT", "DOUT"],
            "lvgl_ui_density": ["normal", "compact"],
        },
        "fields": {
            "name": {"allow_string": True, "allow_list": True},
            "lvgl_ui_density": {"default": "normal"},
        },
        "patterns": {"sdkconfig_key": r"^CONFIG_[A-Z0-9_]+$"},
    }
    if not SCHEMA_FILE.exists():
        return default_schema
    try:
        parsed = parse_yaml(SCHEMA_FILE)
        if isinstance(parsed, dict):
            return parsed
    except Exception as e:
        print(f"{C_YELLOW}Warning: failed to parse {SCHEMA_FILE}: {e}{C_RESET}")
    return default_schema


def canonicalize_name(name_value: Any) -> tuple[str, list[str]]:
    """Return canonical board name + aliases from scalar/list input."""
    if isinstance(name_value, list):
        names = [str(n).strip() for n in name_value if str(n).strip()]
        if not names:
            return "Unknown", []
        return names[0], names[1:]
    if isinstance(name_value, str) and name_value.strip():
        return name_value.strip(), []
    return "Unknown", []


def flatten_profile(data: Any, prefix: str = "") -> dict[str, Any]:
    """Flatten profile dict into dot-path keys for diff output."""
    flat: dict[str, Any] = {}
    if isinstance(data, dict):
        for key in sorted(data.keys()):
            if key.startswith("_"):
                continue
            child_prefix = f"{prefix}.{key}" if prefix else key
            flat.update(flatten_profile(data[key], child_prefix))
    else:
        flat[prefix] = data
    return flat


def yaml_quote(value: str) -> str:
    """Quote string for YAML scalar emission."""
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    return f"\"{escaped}\""


# ── Profile Discovery ─────────────────────────────────────────────────────

def discover_profiles() -> list[dict]:
    """Discover all profiles by scanning Profiles/*/profile.yaml."""
    profiles = []
    if not PROFILES_DIR.exists():
        return profiles

    for entry in sorted(PROFILES_DIR.iterdir()):
        if entry.name.startswith("_") or not entry.is_dir():
            continue
        yaml_file = entry / "profile.yaml"
        if yaml_file.exists():
            try:
                data = parse_yaml(yaml_file)
                data["_path"] = str(entry)
                profiles.append(data)
            except Exception as e:
                print(f"{C_RED}Error parsing {yaml_file}: {e}{C_RESET}")
    return profiles


def get_current_profile() -> Optional[str]:
    """Read the currently selected profile from sdkconfig or sdkconfig.defaults."""
    for path in [SDKCONFIG_DEFAULTS, SDKCONFIG_FILE]:
        if path.exists():
            with open(path) as f:
                for line in f:
                    m = re.match(r'^CONFIG_FLXOS_PROFILE="(.+)"', line.strip())
                    if m:
                        return m.group(1)
    return None


# ── Commands ──────────────────────────────────────────────────────────────

def cmd_list(args):
    """List all available profiles."""
    profiles = discover_profiles()
    if not profiles:
        print(f"{C_RED}No profiles found in {PROFILES_DIR}{C_RESET}")
        return 1

    rows = []
    for p in profiles:
        pid = str(p.get("id", "?"))
        target = str(p.get("target", "?"))
        flash = str(p.get("flash_size", "?"))
        headless = bool(p.get("headless", False))
        display_drv = None if headless else get_nested(p, "hardware.display.driver", None)
        tags = p.get("distribution", {}).get("tags", [])
        if isinstance(tags, dict):
            tags = []
        tags = [str(t) for t in tags]

        rows.append({
            "profile": pid,
            "target": target,
            "flash_size": flash,
            "headless": headless,
            "display": display_drv,
            "tags": tags,
        })

    rows.sort(key=lambda row: row["profile"])

    if args.json:
        print(json.dumps(rows, separators=(",", ":")))
        return 0

    current = get_current_profile()

    # Table header
    print()
    header = f"{'':2} {'Profile':<25} {'Target':<10} {'Flash':<8} {'Headless':<10} {'Display':<12} {'Tags'}"
    print(f"{C_BOLD}{header}{C_RESET}")
    print(f"{'─' * 95}")

    for row in rows:
        pid = row["profile"]
        target = row["target"]
        flash = row["flash_size"]
        headless = row["headless"]
        display_drv = row["display"] if row["display"] else "—"
        tags_str = ", ".join(row["tags"]) if row["tags"] else "—"

        marker = f"{C_GREEN}▸{C_RESET}" if pid == current else " "
        hl_str = f"{C_YELLOW}yes{C_RESET}" if headless else "no"
        disp_str = display_drv if not headless else f"{C_DIM}—{C_RESET}"

        print(f"{marker:2} {pid:<25} {target:<10} {flash:<8} {hl_str:<19} {disp_str:<21} {C_DIM}{tags_str}{C_RESET}")

    print()
    if current:
        print(f"  {C_GREEN}▸{C_RESET} = currently selected profile ({C_BOLD}{current}{C_RESET})")
    else:
        print(f"  {C_YELLOW}No profile selected.{C_RESET} Run: python flxos.py select <id>")
    print()
    return 0


def cmd_select(args):
    """Select a profile for building."""
    profile_id = args.profile_id
    profile_dir = PROFILES_DIR / profile_id
    yaml_file = profile_dir / "profile.yaml"

    if not yaml_file.exists():
        print(f"{C_RED}Error: Profile '{profile_id}' not found at {yaml_file}{C_RESET}")
        print(f"Run 'python flxos.py list' to see available profiles.")
        return 1

    data = parse_yaml(yaml_file)
    new_target = data.get("target", "esp32")

    # Read current target from CMake cache (actual source of truth)
    old_target = _get_cached_idf_target()

    # Keep both defaults and active sdkconfig aligned to avoid stale profile builds.
    _set_sdkconfig_value(SDKCONFIG_DEFAULTS, "CONFIG_FLXOS_PROFILE", profile_id)
    _set_sdkconfig_value(SDKCONFIG_FILE, "CONFIG_FLXOS_PROFILE", profile_id, create_if_missing=False)

    print(f"{C_GREEN}✓{C_RESET} Selected profile: {C_BOLD}{profile_id}{C_RESET}")
    print(f"  Target: {new_target}")
    print(f"  Flash:  {data.get('flash_size', '4MB')}")
    if data.get('headless'):
        print(f"  Mode:   {C_YELLOW}Headless{C_RESET}")

    # Handle target change
    if old_target and old_target != new_target:
        print(f"\n  {C_YELLOW}⚠ Target change: {old_target} → {new_target}{C_RESET}")

        # Wipe build dir and sdkconfig (stale for new target)
        build_dir = SCRIPT_DIR / "build"
        if build_dir.exists():
            import shutil
            shutil.rmtree(build_dir)
            print(f"  Deleted build/ (target changed)")
        if SDKCONFIG_FILE.exists():
            SDKCONFIG_FILE.unlink()
            print(f"  Deleted sdkconfig (stale)")

        # Remove IDF_TARGET from env so set-target doesn't conflict
        set_env = os.environ.copy()
        set_env.pop("IDF_TARGET", None)

        print(f"  Running: idf.py set-target {new_target}")
        result = subprocess.run(
            ["idf.py", "set-target", new_target],
            cwd=str(SCRIPT_DIR),
            env=set_env
        )
        if result.returncode != 0:
            print(f"  {C_RED}set-target failed.{C_RESET}")
            return 1
        print(f"  {C_GREEN}✓{C_RESET} Target set to {new_target}")
    elif not old_target:
        print(f"\n  Run: idf.py set-target {new_target}")

    print(f"\n  Next: python flxos.py build")
    return 0


def cmd_build(args):
    """Build the current profile or all profiles."""
    if args.all:
        return _build_all(args)

    profile_id = get_current_profile()
    if not profile_id:
        print(f"{C_RED}Error: No profile selected.{C_RESET}")
        print(f"Run: python flxos.py select <id>")
        return 1

    # Ensure active sdkconfig tracks the selected profile before invoking idf.py.
    _set_sdkconfig_value(SDKCONFIG_DEFAULTS, "CONFIG_FLXOS_PROFILE", profile_id)
    _set_sdkconfig_value(SDKCONFIG_FILE, "CONFIG_FLXOS_PROFILE", profile_id, create_if_missing=False)

    # Auto-recover target mismatch before building
    yaml_file = PROFILES_DIR / profile_id / "profile.yaml"
    if yaml_file.exists():
        data = parse_yaml(yaml_file)
        expected_target = data.get("target", "esp32")
        cached_target = _get_cached_idf_target()

        if cached_target and cached_target != expected_target:
            print(f"  {C_YELLOW}⚠ Target mismatch: cache has {cached_target}, profile needs {expected_target}{C_RESET}")
            print(f"  {C_CYAN}Auto-recovering...{C_RESET}")

            # Wipe build dir and sdkconfig
            build_dir = SCRIPT_DIR / "build"
            if build_dir.exists():
                import shutil
                shutil.rmtree(build_dir)
                print(f"  Deleted build/ (target changed)")
            if SDKCONFIG_FILE.exists():
                SDKCONFIG_FILE.unlink()
                print(f"  Deleted sdkconfig (stale)")

            # Run set-target with clean env
            set_env = os.environ.copy()
            set_env.pop("IDF_TARGET", None)

            print(f"  Running: idf.py set-target {expected_target}")
            result = subprocess.run(
                ["idf.py", "set-target", expected_target],
                cwd=str(SCRIPT_DIR),
                env=set_env,
            )
            if result.returncode != 0:
                print(f"  {C_RED}set-target failed.{C_RESET}")
                return 1
            print(f"  {C_GREEN}✓{C_RESET} Target set to {expected_target}\n")

    # Re-read profile target to set IDF_TARGET env correctly for the build
    yaml_file = PROFILES_DIR / profile_id / "profile.yaml"
    build_target = "esp32"
    if yaml_file.exists():
        data = parse_yaml(yaml_file)
        build_target = data.get("target", "esp32")

    print(f"{C_BOLD}Building profile: {profile_id}{C_RESET}")

    if args.dev:
        print(f"  {C_YELLOW}Dev mode: forcing 4MB partition table for faster flash{C_RESET}")

    build_env = os.environ.copy()
    build_env["IDF_TARGET"] = build_target

    cmd = ["idf.py", "build"]
    result = subprocess.run(cmd, cwd=str(SCRIPT_DIR), env=build_env)
    return result.returncode


def _build_all(args):
    """Build all profiles sequentially."""
    profiles = discover_profiles()
    if not profiles:
        print(f"{C_RED}No profiles found.{C_RESET}")
        return 1

    results = {}
    original_profile = get_current_profile()

    print(f"{C_BOLD}Building {len(profiles)} profiles...{C_RESET}\n")

    for i, p in enumerate(profiles, 1):
        pid = p.get("id", "?")
        target = p.get("target", "esp32")
        print(f"{'─' * 60}")
        print(f"  [{i}/{len(profiles)}] {C_BOLD}{pid}{C_RESET} (target: {target})")
        print(f"{'─' * 60}")

        # Keep selected profile synced in both defaults and active sdkconfig.
        _set_sdkconfig_value(SDKCONFIG_DEFAULTS, "CONFIG_FLXOS_PROFILE", pid)
        _set_sdkconfig_value(SDKCONFIG_FILE, "CONFIG_FLXOS_PROFILE", pid, create_if_missing=False)

        # Detect current IDF_TARGET from CMake cache
        current_target = _get_cached_idf_target()

        if current_target and current_target != target:
            print(f"  {C_YELLOW}Target change: {current_target} → {target}{C_RESET}")
            # Must wipe build directory for target switch
            build_dir = SCRIPT_DIR / "build"
            if build_dir.exists():
                import shutil
                shutil.rmtree(build_dir)
                print(f"  Deleted build/ (target changed)")
            if SDKCONFIG_FILE.exists():
                SDKCONFIG_FILE.unlink()
                print(f"  Deleted sdkconfig (stale)")

        # Build env with correct IDF_TARGET — this is critical because ESP-IDF
        # checks the IDF_TARGET env var and refuses set-target if it doesn't match.
        build_env = os.environ.copy()
        build_env["IDF_TARGET"] = target

        # For set-target, REMOVE IDF_TARGET from env so it doesn't conflict
        set_target_env = os.environ.copy()
        set_target_env.pop("IDF_TARGET", None)

        # Set target
        set_result = subprocess.run(
            ["idf.py", "set-target", target],
            cwd=str(SCRIPT_DIR),
            env=set_target_env
        )
        if set_result.returncode != 0:
            print(f"  {C_RED}set-target failed, retrying with clean state...{C_RESET}")
            build_dir = SCRIPT_DIR / "build"
            if build_dir.exists():
                import shutil
                shutil.rmtree(build_dir)
            if SDKCONFIG_FILE.exists():
                SDKCONFIG_FILE.unlink()
            subprocess.run(
                ["idf.py", "set-target", target],
                cwd=str(SCRIPT_DIR),
                env=set_target_env
            )

        # Build with IDF_TARGET set correctly
        build_result = subprocess.run(
            ["idf.py", "build"],
            cwd=str(SCRIPT_DIR),
            env=build_env
        )
        results[pid] = "✅" if build_result.returncode == 0 else "❌"

    # Restore original profile
    if original_profile:
        _set_sdkconfig_value(SDKCONFIG_DEFAULTS, "CONFIG_FLXOS_PROFILE", original_profile)
        _set_sdkconfig_value(SDKCONFIG_FILE, "CONFIG_FLXOS_PROFILE", original_profile, create_if_missing=False)

    # Summary
    print(f"\n{'═' * 60}")
    print(f"{C_BOLD}Build Results:{C_RESET}\n")
    for pid, status in results.items():
        print(f"  {status} {pid}")
    print()

    failed = sum(1 for s in results.values() if s == "❌")
    return 1 if failed else 0


def cmd_validate(args):
    """Validate profile YAML files."""
    schema = load_schema()
    required_fields = schema.get("required_fields", ["id", "vendor", "name", "target", "flash_size"])
    valid_targets = [str(v) for v in get_nested(schema, "enums.target", ["esp32", "esp32s3", "esp32c6", "esp32p4"])]
    valid_flash = [str(v) for v in get_nested(schema, "enums.flash_size", ["4MB", "8MB", "16MB"])]
    valid_flash_modes = [str(v).upper() for v in get_nested(schema, "enums.flash_mode", ["QIO", "DIO", "QOUT", "DOUT"])]
    valid_ui_density = [str(v).lower() for v in get_nested(schema, "enums.lvgl_ui_density", ["normal", "compact"])]
    ui_density_default = str(get_nested(schema, "fields.lvgl_ui_density.default", "normal")).lower()
    allow_name_string = bool(get_nested(schema, "fields.name.allow_string", True))
    allow_name_list = bool(get_nested(schema, "fields.name.allow_list", True))
    sdkconfig_key_pattern = str(get_nested(schema, "patterns.sdkconfig_key", r"^CONFIG_[A-Z0-9_]+$"))

    if args.profile_id:
        profiles = []
        yaml_file = PROFILES_DIR / args.profile_id / "profile.yaml"
        if yaml_file.exists():
            data = parse_yaml(yaml_file)
            data["_path"] = str(yaml_file.parent)
            profiles = [data]
        else:
            print(f"{C_RED}Profile '{args.profile_id}' not found.{C_RESET}")
            return 1
    else:
        profiles = discover_profiles()

    if not profiles:
        print(f"{C_RED}No profiles found.{C_RESET}")
        return 1

    total_errors = 0
    total_warnings = 0

    for p in profiles:
        pid = p.get("id", "unknown")
        errors = []
        warnings = []

        # Required fields
        for field in required_fields:
            if field not in p or p[field] is None:
                errors.append(f"Missing required field: {field}")

        # name type validation (string or list[string])
        name_val = p.get("name")
        if isinstance(name_val, list):
            if not allow_name_list:
                errors.append("Field 'name' does not allow list values")
            elif not name_val:
                errors.append("Field 'name' list cannot be empty")
            else:
                bad_names = [v for v in name_val if not isinstance(v, str) or not v.strip()]
                if bad_names:
                    errors.append("Field 'name' list must contain non-empty strings")
        elif isinstance(name_val, str):
            if not allow_name_string:
                errors.append("Field 'name' does not allow string values")
            elif not name_val.strip():
                errors.append("Field 'name' cannot be empty")
        elif name_val is not None:
            errors.append("Field 'name' must be string or list of strings")

        # Target validation
        target = str(p.get("target", ""))
        if target not in valid_targets:
            errors.append(f"Invalid target '{target}'. Valid: {valid_targets}")

        # Flash size validation
        flash = str(p.get("flash_size", ""))
        if flash not in valid_flash:
            errors.append(f"Invalid flash_size '{flash}'. Valid: {valid_flash}")

        # Flash mode validation
        flash_mode = p.get("flash_mode")
        if flash_mode is not None and str(flash_mode).lower() not in ("", "null"):
            flash_mode_upper = str(flash_mode).upper()
            if flash_mode_upper not in valid_flash_modes:
                errors.append(f"Invalid flash_mode '{flash_mode}'. Valid: {valid_flash_modes}")

        # UI density validation
        ui_density = get_nested(p, "lvgl.ui_density", ui_density_default)
        if ui_density is not None and str(ui_density).lower() not in ("", "null"):
            ui_density_lower = str(ui_density).lower()
            if ui_density_lower not in valid_ui_density:
                errors.append(f"Invalid lvgl.ui_density '{ui_density}'. Valid: {valid_ui_density}")

        # sdkconfig passthrough key validation
        sdkconfig = p.get("sdkconfig", {})
        if sdkconfig and not isinstance(sdkconfig, dict):
            errors.append("Field 'sdkconfig' must be a key/value map")
        elif isinstance(sdkconfig, dict):
            for key in sdkconfig.keys():
                if not re.match(sdkconfig_key_pattern, str(key)):
                    errors.append(
                        f"Invalid sdkconfig key '{key}'. Must match pattern: {sdkconfig_key_pattern}"
                    )

        # SPIRAM speed / flash freq sync check
        spiram = p.get("hardware", {}).get("spiram", {})
        if spiram.get("enabled") and spiram.get("speed") == "120M":
            flash_freq = p.get("flash_freq")
            if flash_freq and flash_freq != "null" and flash_freq != "120M":
                warnings.append(f"SPIRAM speed=120M but flash_freq={flash_freq}. They must match at 120MHz.")

        # Display validation for non-headless
        if not p.get("headless", False):
            hw = p.get("hardware", {})
            display = hw.get("display", {})
            if not display.get("enabled"):
                warnings.append("Non-headless profile has display.enabled=false")
            elif not display.get("driver"):
                errors.append("Display enabled but no driver specified")

        # Print results
        if errors:
            print(f"{C_RED}✗{C_RESET} {C_BOLD}{pid}{C_RESET}")
            for e in errors:
                print(f"    {C_RED}ERROR:{C_RESET} {e}")
            total_errors += len(errors)
        elif warnings:
            print(f"{C_YELLOW}⚠{C_RESET} {C_BOLD}{pid}{C_RESET}")
        else:
            print(f"{C_GREEN}✓{C_RESET} {C_BOLD}{pid}{C_RESET}")

        for w in warnings:
            print(f"    {C_YELLOW}WARN:{C_RESET} {w}")
            total_warnings += 1

    print()
    if total_errors == 0:
        print(f"{C_GREEN}All {len(profiles)} profiles valid.{C_RESET}", end="")
        if total_warnings:
            print(f" ({C_YELLOW}{total_warnings} warning(s){C_RESET})", end="")
        print()
    else:
        print(f"{C_RED}{total_errors} error(s){C_RESET} in {len(profiles)} profiles.")

    return 1 if total_errors else 0


def cmd_info(args):
    """Show detailed profile information."""
    profile_id = args.profile_id
    yaml_file = PROFILES_DIR / profile_id / "profile.yaml"

    if not yaml_file.exists():
        print(f"{C_RED}Profile '{profile_id}' not found.{C_RESET}")
        return 1

    data = parse_yaml(yaml_file)

    print(f"\n{C_BOLD}Profile: {data.get('id', '?')}{C_RESET}")
    print(f"{'─' * 50}")

    # Core info
    canonical_name, aliases = canonicalize_name(data.get("name", "Unknown"))
    print(f"  Vendor:     {data.get('vendor', '?')}")
    print(f"  Name:       {canonical_name}")
    if aliases:
        print(f"  Aliases:    {', '.join(aliases)}")
    print(f"  Target:     {C_CYAN}{data.get('target', '?')}{C_RESET}")
    print(f"  Flash:      {data.get('flash_size', '?')}")
    flash_mode = data.get('flash_mode')
    if flash_mode and flash_mode is not None:
        print(f"  Flash Mode: {flash_mode}")
    flash_freq = data.get('flash_freq')
    if flash_freq and flash_freq is not None:
        print(f"  Flash Freq: {flash_freq}")
    print(f"  Headless:   {'yes' if data.get('headless') else 'no'}")
    inherits = data.get('inherits')
    if inherits and inherits is not None:
        print(f"  Inherits:   {inherits}")

    # Hardware
    hw = data.get("hardware", {})
    spiram = hw.get("spiram", {})
    if spiram.get("enabled"):
        speed = spiram.get("speed", "80M")
        print(f"\n  {C_BOLD}SPIRAM:{C_RESET} enabled (speed: {speed})")

    display = hw.get("display", {})
    if display.get("enabled"):
        print(f"\n  {C_BOLD}Display:{C_RESET}")
        print(f"    Driver:     {display.get('driver', '?')}")
        print(f"    Resolution: {display.get('width', '?')}×{display.get('height', '?')}")
        print(f"    Color:      {display.get('color_depth', '?')}-bit")
        print(f"    Size:       {display.get('size_inches', '?')}\"")
        print(f"    Rotation:   {display.get('rotation', 0)}")
        pins = display.get("pins", {})
        if pins:
            pin_str = ", ".join(f"{k}={v}" for k, v in pins.items() if v != -1)
            print(f"    Pins:       {pin_str}")

    touch = hw.get("touch", {})
    if touch.get("enabled"):
        print(f"\n  {C_BOLD}Touch:{C_RESET}")
        print(f"    Driver: {touch.get('driver', '?')}")
        tpins = touch.get("pins", {})
        if tpins:
            pin_str = ", ".join(f"{k}={v}" for k, v in tpins.items() if v != -1)
            print(f"    Pins:   {pin_str}")

    sdcard = hw.get("sdcard", {})
    if sdcard.get("enabled"):
        print(f"\n  {C_BOLD}SD Card:{C_RESET} CS={sdcard.get('cs', '?')}, {sdcard.get('max_freq_khz', '?')}kHz")

    battery = hw.get("battery", {})
    if battery.get("enabled"):
        print(f"\n  {C_BOLD}Battery:{C_RESET} ADC{battery.get('adc_unit', '?')}:{battery.get('adc_channel', '?')}")

    # CLI
    cli = data.get("cli", {})
    if cli.get("enabled"):
        print(f"\n  {C_BOLD}CLI:{C_RESET} enabled (prompt: \"{cli.get('prompt', 'flxos> ')}\")")

    # Capabilities
    caps = data.get("capabilities", {})
    if caps:
        enabled_caps = [k for k, v in caps.items() if v is True]
        if enabled_caps:
            print(f"\n  {C_BOLD}Capabilities:{C_RESET} {', '.join(enabled_caps)}")

    # Distribution
    dist = data.get("distribution", {})
    tags = dist.get("tags", [])
    if isinstance(tags, dict):
        tags = []
    if tags:
        print(f"\n  {C_BOLD}Tags:{C_RESET} {', '.join(str(t) for t in tags)}")
    warning = dist.get("warning_message")
    if warning:
        print(f"  {C_YELLOW}⚠ {warning}{C_RESET}")

    print()
    return 0


def cmd_new(args):
    """Scaffold a new profile directory with profile.yaml."""
    profile_id = args.profile_id.strip()
    if not re.fullmatch(r"[a-z0-9][a-z0-9-]*", profile_id):
        print(f"{C_RED}Invalid profile id '{profile_id}'. Use lowercase letters, numbers, and hyphens.{C_RESET}")
        return 1

    profile_dir = PROFILES_DIR / profile_id
    yaml_file = profile_dir / "profile.yaml"
    if profile_dir.exists():
        print(f"{C_RED}Profile directory already exists: {profile_dir}{C_RESET}")
        return 1

    template_name = args.template.strip() if args.template else ("base-headless" if args.headless else "base-esp32-spi")
    if template_name.startswith("_bases/"):
        template_name = template_name[len("_bases/"):]
    if template_name.endswith(".yaml"):
        template_name = template_name[:-5]

    template_file = PROFILES_DIR / "_bases" / f"{template_name}.yaml"
    if not template_file.exists():
        print(f"{C_RED}Template not found: {template_file}{C_RESET}")
        return 1

    inherits = f"_bases/{template_name}"
    is_headless = args.headless or template_name == "base-headless"
    target = args.target
    flash_size = args.flash_size
    vendor = args.vendor.strip() if args.vendor else ("Generic" if is_headless else "FlxOS")

    if args.name:
        names = [n.strip() for n in args.name.split(",") if n.strip()]
    else:
        names = [profile_id]
    if not names:
        print(f"{C_RED}Profile name cannot be empty.{C_RESET}")
        return 1

    lines: list[str] = [
        f"# FlxOS Profile: {profile_id}",
        f"id: {profile_id}",
        f"vendor: {yaml_quote(vendor)}",
    ]
    if len(names) == 1:
        lines.append(f"name: {yaml_quote(names[0])}")
    else:
        lines.append("name:")
        for name in names:
            lines.append(f"  - {yaml_quote(name)}")

    lines.extend([
        f"target: {target}",
        f"flash_size: {flash_size}",
        "flash_mode: QIO",
        "flash_freq: null",
        f"headless: {'true' if is_headless else 'false'}",
        f"inherits: {inherits}",
        "",
        "cli:",
        "  enabled: true",
        f"  prompt: {yaml_quote('flxos> ')}",
        "  max_cmdline_length: 256",
    ])

    if not is_headless:
        lines.extend([
            "",
            "lvgl:",
            "  font_size: 14",
            "  theme: DefaultDark",
            "  ui_density: normal",
        ])

    lines.extend([
        "",
        "capabilities:",
        "  wifi: true",
        "  bluetooth: true",
        "  ble: true",
        "  gps: false",
        "  lora: false",
        "  camera: false",
        "  audio: false",
        "  keyboard: false",
        "  trackball: false",
        "",
        "distribution:",
        "  incubating: true",
        "  tags:",
        f"    - {'headless' if is_headless else 'spi'}",
        "    - scaffold",
    ])

    profile_dir.mkdir(parents=True, exist_ok=False)
    yaml_file.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(f"{C_GREEN}✓{C_RESET} Created profile: {C_BOLD}{profile_id}{C_RESET}")
    print(f"  Path: {yaml_file}")
    print(f"  Template: {template_name}")
    print(f"  Next: python flxos.py validate {profile_id}")
    return 0


def cmd_diff(args):
    """Show differences between two profile.yaml files."""
    a_id = args.profile_a
    b_id = args.profile_b
    a_file = PROFILES_DIR / a_id / "profile.yaml"
    b_file = PROFILES_DIR / b_id / "profile.yaml"

    if not a_file.exists():
        print(f"{C_RED}Profile '{a_id}' not found.{C_RESET}")
        return 1
    if not b_file.exists():
        print(f"{C_RED}Profile '{b_id}' not found.{C_RESET}")
        return 1

    a_data = parse_yaml(a_file)
    b_data = parse_yaml(b_file)
    a_flat = flatten_profile(a_data)
    b_flat = flatten_profile(b_data)

    keys = sorted(set(a_flat.keys()) | set(b_flat.keys()))
    added = []
    removed = []
    changed = []

    for key in keys:
        in_a = key in a_flat
        in_b = key in b_flat
        if in_a and not in_b:
            removed.append({"key": key, "value": a_flat[key]})
        elif in_b and not in_a:
            added.append({"key": key, "value": b_flat[key]})
        elif a_flat[key] != b_flat[key]:
            changed.append({"key": key, "from": a_flat[key], "to": b_flat[key]})

    result = {
        "profile_a": a_id,
        "profile_b": b_id,
        "added": added,
        "removed": removed,
        "changed": changed,
    }

    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0

    def _fmt(value: Any) -> str:
        return json.dumps(value, ensure_ascii=False, sort_keys=True)

    print(f"\n{C_BOLD}Profile Diff: {a_id} → {b_id}{C_RESET}")
    print(f"{'─' * 60}")

    if not added and not removed and not changed:
        print(f"  {C_GREEN}No differences.{C_RESET}\n")
        return 0

    if added:
        print(f"\n{C_GREEN}Added in {b_id}:{C_RESET}")
        for item in added:
            print(f"  + {item['key']} = {_fmt(item['value'])}")

    if removed:
        print(f"\n{C_RED}Removed from {b_id}:{C_RESET}")
        for item in removed:
            print(f"  - {item['key']} = {_fmt(item['value'])}")

    if changed:
        print(f"\n{C_YELLOW}Changed:{C_RESET}")
        for item in changed:
            print(f"  ~ {item['key']}")
            print(f"      {a_id}: {_fmt(item['from'])}")
            print(f"      {b_id}: {_fmt(item['to'])}")

    print()
    return 0


def cmd_flash(args):
    """Flash current build to device via idf.py."""
    if not _require_idf_tooling():
        return 1

    if _assert_build_matches_selection() is None:
        return 1

    flasher_path = BUILD_DIR / "flasher_args.json"
    if not flasher_path.exists():
        print(f"{C_RED}Error: missing build artifact {flasher_path}{C_RESET}")
        print("Run: python flxos.py build")
        return 1

    cmd = ["idf.py"]
    if args.port:
        cmd.extend(["-p", args.port])
    cmd.append("flash")

    print(f"{C_BOLD}Flashing current build...{C_RESET}")
    result = subprocess.run(cmd, cwd=str(SCRIPT_DIR))
    return result.returncode


def cmd_release(args):
    """Package release binaries, symbols, and flashing scripts."""
    normalized = _normalize_version(args.version)
    if normalized is None:
        return 1
    version_raw, version_tag = normalized

    context = _assert_build_matches_selection()
    if context is None:
        return 1

    flasher_args = _load_flasher_args()
    if flasher_args is None:
        return 1

    try:
        parts = _collect_packaged_parts(flasher_args)
    except Exception as e:
        print(f"{C_RED}Error preparing release artifacts: {e}{C_RESET}")
        print("Run: python flxos.py build")
        return 1

    elf_src = BUILD_DIR / "FlxOS.elf"
    if not elf_src.exists():
        print(f"{C_RED}Error: missing build artifact {elf_src}{C_RESET}")
        print("Run: python flxos.py build")
        return 1

    profile_id = context["profile_id"]
    release_dir = RELEASES_DIR / f"flxos-{profile_id}-{version_tag}"
    symbols_dir = RELEASES_DIR / f"flxos-{profile_id}-{version_tag}-symbols"

    if not _ensure_output_not_exists(release_dir):
        return 1
    if not _ensure_output_not_exists(symbols_dir):
        return 1

    release_dir.mkdir(parents=True, exist_ok=False)
    symbols_dir.mkdir(parents=True, exist_ok=False)

    for part in parts:
        shutil.copy2(part["source_abs"], release_dir / part["name"])

    packaged_flasher_args = _rewrite_flasher_args_for_package(flasher_args, parts)
    (release_dir / "flasher_args.json").write_text(
        json.dumps(packaged_flasher_args, indent=2) + "\n", encoding="utf-8"
    )
    _write_flash_args_file(release_dir / "flash_args", packaged_flasher_args)
    _write_flash_shell_script(release_dir / "flash.sh")
    _write_flash_ps1_script(release_dir / "flash.ps1")
    (release_dir / "flash.sh").chmod(0o755)

    shutil.copy2(elf_src, symbols_dir / "FlxOS.elf")

    print(f"{C_GREEN}✓{C_RESET} Release packaged")
    print(f"  Version: {version_raw}")
    print(f"  Profile: {profile_id}")
    print(f"  Path:    {release_dir}")
    print(f"  Symbols: {symbols_dir}")
    print(f"  Parts:   {', '.join(part['name'] for part in parts)}")
    return 0


def cmd_cdn(args):
    """Package CDN bundle and generate ESP Web Tools manifests."""
    normalized = _normalize_version(args.version)
    if normalized is None:
        return 1
    version_raw, version_tag = normalized

    context = _assert_build_matches_selection()
    if context is None:
        return 1

    flasher_args = _load_flasher_args()
    if flasher_args is None:
        return 1

    try:
        parts = _collect_packaged_parts(flasher_args)
    except Exception as e:
        print(f"{C_RED}Error preparing CDN artifacts: {e}{C_RESET}")
        print("Run: python flxos.py build")
        return 1

    profile_id = context["profile_id"]
    profile_data = context["profile_data"]
    cdn_dir = RELEASES_DIR / f"flxos-{profile_id}-{version_tag}-cdn"

    if not _ensure_output_not_exists(cdn_dir):
        return 1

    cdn_dir.mkdir(parents=True, exist_ok=False)

    for part in parts:
        shutil.copy2(part["source_abs"], cdn_dir / part["name"])

    packaged_flasher_args = _rewrite_flasher_args_for_package(flasher_args, parts)
    (cdn_dir / "flasher_args.json").write_text(
        json.dumps(packaged_flasher_args, indent=2) + "\n", encoding="utf-8"
    )

    board_name, _aliases = canonicalize_name(profile_data.get("name"))
    vendor = str(profile_data.get("vendor", "")).strip()
    name_parts = ["FlxOS for"]
    if vendor:
        name_parts.append(vendor)
    if board_name:
        name_parts.append(board_name)
    manifest_name = " ".join(name_parts)

    manifest = {
        "name": manifest_name,
        "version": version_raw,
        "new_install_prompt_erase": "true",
        "builds": [
            {
                "chipFamily": _chip_family_from_target(context["target"]),
                "parts": [{"path": part["name"], "offset": part["offset_int"]} for part in parts],
            }
        ],
    }

    (cdn_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    _regenerate_cdn_index()

    print(f"{C_GREEN}✓{C_RESET} CDN bundle generated")
    print(f"  Version:  {version_raw}")
    print(f"  Profile:  {profile_id}")
    print(f"  Path:     {cdn_dir}")
    print(f"  Manifest: {cdn_dir / 'manifest.json'}")
    print(f"  Index:    {RELEASES_DIR / 'index.json'}")
    return 0


def cmd_doctor(args):
    """Check the build environment for issues."""
    issues = 0

    print(f"\n{C_BOLD}FlxOS Doctor{C_RESET}")
    print(f"{'─' * 50}\n")

    # Python version
    py_ver = sys.version_info
    if py_ver >= (3, 8):
        print(f"  {C_GREEN}✓{C_RESET} Python {py_ver.major}.{py_ver.minor}.{py_ver.micro}")
    else:
        print(f"  {C_RED}✗{C_RESET} Python {py_ver.major}.{py_ver.minor} (need ≥3.8)")
        issues += 1

    # ESP-IDF
    idf_path = os.environ.get("IDF_PATH", "")
    if idf_path and Path(idf_path).exists():
        # Try to get version
        ver_file = Path(idf_path) / "version.txt"
        if ver_file.exists():
            ver = ver_file.read_text().strip()
            print(f"  {C_GREEN}✓{C_RESET} ESP-IDF {ver} at {idf_path}")
        else:
            print(f"  {C_GREEN}✓{C_RESET} ESP-IDF at {idf_path}")
    else:
        print(f"  {C_RED}✗{C_RESET} ESP-IDF not found (IDF_PATH not set)")
        print(f"    Run: source /path/to/esp-idf/export.sh")
        issues += 1

    # idf.py
    idf_py = subprocess.run(
        ["which", "idf.py"], capture_output=True, text=True
    )
    if idf_py.returncode == 0:
        print(f"  {C_GREEN}✓{C_RESET} idf.py found")
    else:
        print(f"  {C_RED}✗{C_RESET} idf.py not in PATH")
        issues += 1

    # ccache
    ccache = subprocess.run(
        ["which", "ccache"], capture_output=True, text=True
    )
    if ccache.returncode == 0:
        print(f"  {C_GREEN}✓{C_RESET} ccache found (builds will be cached)")
    else:
        print(f"  {C_YELLOW}⚠{C_RESET} ccache not found (builds will be slower)")

    # CMake
    cmake = subprocess.run(
        ["cmake", "--version"], capture_output=True, text=True
    )
    if cmake.returncode == 0:
        ver_line = cmake.stdout.split("\n")[0]
        print(f"  {C_GREEN}✓{C_RESET} {ver_line}")
    else:
        print(f"  {C_RED}✗{C_RESET} CMake not found")
        issues += 1

    # Profiles
    profiles = discover_profiles()
    print(f"\n  {C_GREEN}✓{C_RESET} {len(profiles)} profile(s) discovered")

    # Current profile
    current = get_current_profile()
    if current:
        print(f"  {C_GREEN}✓{C_RESET} Current profile: {current}")
    else:
        print(f"  {C_YELLOW}⚠{C_RESET} No profile selected")

    # Build directory
    build_dir = SCRIPT_DIR / "build"
    if build_dir.exists():
        print(f"  {C_GREEN}✓{C_RESET} Build directory exists")
    else:
        print(f"  {C_DIM}  Build directory not yet created{C_RESET}")

    print()
    if issues == 0:
        print(f"  {C_GREEN}All checks passed!{C_RESET}")
    else:
        print(f"  {C_RED}{issues} issue(s) found.{C_RESET}")
    print()
    return 1 if issues else 0


def cmd_hwgen(args):
    """Generate hardware init scaffold from profile.yaml."""
    hwgen_script = SCRIPT_DIR / "Buildscripts" / "hwgen.py"
    if not hwgen_script.exists():
        print(f"{C_RED}Error: missing generator script {hwgen_script}{C_RESET}")
        return 1

    cmd = [sys.executable, str(hwgen_script)]
    if args.all:
        cmd.append("--all")
    elif args.profile_id:
        cmd.extend(["--profile", args.profile_id])

    if args.force:
        cmd.append("--force")
    if args.stdout:
        cmd.append("--stdout")
    if args.output:
        cmd.extend(["--output", args.output])

    result = subprocess.run(cmd, cwd=str(SCRIPT_DIR))
    return result.returncode


# ── Helpers ────────────────────────────────────────────────────────────────

def _normalize_version(version_text: str) -> Optional[tuple[str, str]]:
    """Normalize version input into raw/tag forms."""
    raw = version_text.strip()
    if raw.lower().startswith("v"):
        raw = raw[1:]

    # Accept semver-like values and optional prerelease/build suffixes.
    if not re.fullmatch(r"[0-9]+(?:\.[0-9]+){2}(?:[-+][0-9A-Za-z.-]+)?", raw):
        print(
            f"{C_RED}Invalid version '{version_text}'. Use 1.2.3 or v1.2.3 "
            "(semver-like prerelease/build suffixes allowed).{C_RESET}"
        )
        return None

    return raw, f"v{raw}"


def _require_idf_tooling() -> bool:
    """Validate ESP-IDF environment availability for flashing."""
    ok = True

    idf_path = os.environ.get("IDF_PATH", "")
    if not idf_path or not Path(idf_path).exists():
        print(f"{C_RED}Error: ESP-IDF not found (IDF_PATH is missing or invalid).{C_RESET}")
        print("Run: source /path/to/esp-idf/export.sh")
        ok = False

    if shutil.which("idf.py") is None:
        print(f"{C_RED}Error: idf.py not found in PATH.{C_RESET}")
        print("Run: source /path/to/esp-idf/export.sh")
        ok = False

    return ok


def _load_selected_profile_metadata() -> Optional[dict]:
    """Load selected profile id, target and YAML payload."""
    profile_id = get_current_profile()
    if not profile_id:
        print(f"{C_RED}Error: no profile selected.{C_RESET}")
        print("Run: python flxos.py select <id>")
        return None

    yaml_file = PROFILES_DIR / profile_id / "profile.yaml"
    if not yaml_file.exists():
        print(f"{C_RED}Error: selected profile file is missing: {yaml_file}{C_RESET}")
        print("Run: python flxos.py select <id>")
        return None

    try:
        profile_data = parse_yaml(yaml_file)
    except Exception as e:
        print(f"{C_RED}Error parsing {yaml_file}: {e}{C_RESET}")
        return None

    target = str(profile_data.get("target", "")).strip()
    if not target:
        print(f"{C_RED}Error: selected profile has no target: {yaml_file}{C_RESET}")
        return None

    return {
        "profile_id": profile_id,
        "target": target,
        "profile_data": profile_data,
        "path": yaml_file,
    }


def _read_built_profile_metadata() -> Optional[dict]:
    """Read built profile metadata from build/config/sdkconfig.json."""
    metadata_file = BUILD_DIR / "config" / "sdkconfig.json"
    if not metadata_file.exists():
        return None

    try:
        metadata = json.loads(metadata_file.read_text(encoding="utf-8"))
    except Exception:
        return None

    profile_id = metadata.get("FLXOS_PROFILE")
    target = metadata.get("IDF_TARGET")

    return {
        "profile_id": str(profile_id).strip() if profile_id else None,
        "target": str(target).strip() if target else None,
        "path": metadata_file,
    }


def _assert_build_matches_selection() -> Optional[dict]:
    """Ensure selected profile + target match current build metadata."""
    selected = _load_selected_profile_metadata()
    if selected is None:
        return None

    built = _read_built_profile_metadata()
    if built is None:
        print(f"{C_RED}Error: no build metadata found at {BUILD_DIR / 'config' / 'sdkconfig.json'}{C_RESET}")
        print("Run:")
        print(f"  python flxos.py select {selected['profile_id']}")
        print("  python flxos.py build")
        return None

    built_profile_id = built.get("profile_id")
    built_target = built.get("target")

    if not built_profile_id or not built_target:
        print(f"{C_RED}Error: incomplete build metadata in {built['path']}{C_RESET}")
        print("Run:")
        print(f"  python flxos.py select {selected['profile_id']}")
        print("  python flxos.py build")
        return None

    if built_profile_id != selected["profile_id"]:
        print(
            f"{C_RED}Error: selected profile ({selected['profile_id']}) does not match "
            f"build profile ({built_profile_id}).{C_RESET}"
        )
        print("Run:")
        print(f"  python flxos.py select {selected['profile_id']}")
        print("  python flxos.py build")
        return None

    if built_target != selected["target"]:
        print(
            f"{C_RED}Error: selected target ({selected['target']}) does not match "
            f"build target ({built_target}).{C_RESET}"
        )
        print("Run:")
        print(f"  python flxos.py select {selected['profile_id']}")
        print("  python flxos.py build")
        return None

    return {
        "profile_id": selected["profile_id"],
        "target": selected["target"],
        "profile_data": selected["profile_data"],
        "build_profile_id": built_profile_id,
        "build_target": built_target,
    }


def _load_flasher_args() -> Optional[dict]:
    """Load build/flasher_args.json."""
    flasher_file = BUILD_DIR / "flasher_args.json"
    if not flasher_file.exists():
        print(f"{C_RED}Error: missing build artifact {flasher_file}{C_RESET}")
        print("Run: python flxos.py build")
        return None

    try:
        data = json.loads(flasher_file.read_text(encoding="utf-8"))
    except Exception as e:
        print(f"{C_RED}Error parsing {flasher_file}: {e}{C_RESET}")
        return None

    if not isinstance(data.get("flash_files"), dict) or not data.get("flash_files"):
        print(f"{C_RED}Error: invalid flash_files map in {flasher_file}{C_RESET}")
        return None

    return data


def _offset_to_int(offset: Any) -> int:
    """Convert offset text (hex/dec) to integer."""
    text = str(offset).strip().lower()
    if text.startswith("0x"):
        return int(text, 16)
    return int(text, 10)


def _normalize_packaged_part_name(
    offset: str,
    source_rel: str,
    app_offset: str,
    app_file_basename: str,
) -> str:
    """Normalize part names for packaged outputs."""
    rel_norm = source_rel.replace("\\", "/").lower()
    basename = Path(source_rel).name
    basename_lower = basename.lower()
    offset_lower = str(offset).lower()

    if (app_offset and offset_lower == app_offset) or (app_file_basename and basename_lower == app_file_basename):
        return "flxos.bin"
    if "/bootloader/" in f"/{rel_norm}" or basename_lower == "bootloader.bin":
        return "bootloader.bin"
    if "/partition_table/" in f"/{rel_norm}" or "partition-table" in basename_lower:
        return "partition-table.bin"
    return basename


def _collect_packaged_parts(flasher_args: dict) -> list[dict]:
    """Collect binary parts from flasher_args and map to packaged names."""
    flash_files = flasher_args.get("flash_files", {})
    app_info = flasher_args.get("app", {})
    app_offset = str(app_info.get("offset", "")).lower()
    app_file_basename = Path(str(app_info.get("file", ""))).name.lower()

    ordered = sorted(flash_files.items(), key=lambda item: _offset_to_int(item[0]))
    parts: list[dict] = []
    used_names: set[str] = set()

    for offset, source_rel_raw in ordered:
        offset_str = str(offset)
        source_rel = str(source_rel_raw).replace("\\", "/")
        source_abs = BUILD_DIR / source_rel
        if not source_abs.exists():
            raise FileNotFoundError(f"missing part for offset {offset_str}: {source_abs}")

        name = _normalize_packaged_part_name(offset_str, source_rel, app_offset, app_file_basename)
        if name in used_names:
            stem = Path(name).stem
            suffix = Path(name).suffix
            name = f"{stem}-{offset_str.lower().replace('0x', '')}{suffix}"
        used_names.add(name)

        parts.append(
            {
                "offset": offset_str,
                "offset_int": _offset_to_int(offset_str),
                "source_rel": source_rel,
                "source_abs": source_abs,
                "name": name,
            }
        )

    return parts


def _rewrite_flasher_args_for_package(flasher_args: dict, parts: list[dict]) -> dict:
    """Rewrite flasher_args paths to packaged file names."""
    packaged = json.loads(json.dumps(flasher_args))
    offset_to_name = {str(part["offset"]).lower(): part["name"] for part in parts}

    packaged["flash_files"] = {str(part["offset"]): part["name"] for part in parts}

    for value in packaged.values():
        if isinstance(value, dict) and "offset" in value and "file" in value:
            off = str(value.get("offset", "")).lower()
            if off in offset_to_name:
                value["file"] = offset_to_name[off]

    return packaged


def _write_flash_args_file(path: Path, flasher_args: dict):
    """Write flash_args in ESP-IDF compatible two-line format."""
    write_args = flasher_args.get("write_flash_args", [])
    if not isinstance(write_args, list):
        write_args = []
    write_line = " ".join(str(arg) for arg in write_args)

    flash_files = flasher_args.get("flash_files", {})
    pairs = []
    for offset, filename in sorted(flash_files.items(), key=lambda item: _offset_to_int(item[0])):
        pairs.append(f"{offset} {filename}")
    files_line = " ".join(pairs)

    path.write_text(f"{write_line}\n{files_line}\n", encoding="utf-8")


def _write_flash_shell_script(path: Path):
    """Write portable bash flashing helper."""
    path.write_text(
        """#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="${1:-}"

if [[ -z "$PORT" ]]; then
  echo "Usage: ./flash.sh /dev/ttyUSB0" >&2
  exit 1
fi

if command -v esptool >/dev/null 2>&1; then
  ESPTOOL=(esptool)
elif command -v esptool.py >/dev/null 2>&1; then
  ESPTOOL=(esptool.py)
else
  ESPTOOL=(python3 -m esptool)
fi

WRITE_ARGS="$(sed -n '1p' "$SCRIPT_DIR/flash_args")"
FLASH_FILES="$(sed -n '2p' "$SCRIPT_DIR/flash_args")"

"${ESPTOOL[@]}" --port "$PORT" erase_flash
# shellcheck disable=SC2086
"${ESPTOOL[@]}" --port "$PORT" write_flash $WRITE_ARGS $FLASH_FILES
""",
        encoding="utf-8",
    )


def _write_flash_ps1_script(path: Path):
    """Write PowerShell flashing helper."""
    path.write_text(
        """param(
    [Parameter(Mandatory = $true)]
    [string]$Port
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

if (Get-Command "esptool" -ErrorAction SilentlyContinue) {
    $Tool = @("esptool")
} elseif (Get-Command "esptool.py" -ErrorAction SilentlyContinue) {
    $Tool = @("esptool.py")
} else {
    $Tool = @("python3", "-m", "esptool")
}

$FlashArgsPath = Join-Path $ScriptDir "flash_args"
$Lines = Get-Content -Path $FlashArgsPath
$WriteArgs = @()
$FileArgs = @()
if ($Lines.Length -gt 0) {
    $WriteArgs = $Lines[0] -split '\\s+' | Where-Object { $_ -ne "" }
}
if ($Lines.Length -gt 1) {
    $FileArgs = $Lines[1] -split '\\s+' | Where-Object { $_ -ne "" }
}

function Invoke-Tool {
    param([string[]]$ToolCmd, [string[]]$Args)
    $All = @($ToolCmd + $Args)
    if ($All.Length -gt 1) {
        & $All[0] @($All[1..($All.Length - 1)])
    } else {
        & $All[0]
    }
}

Invoke-Tool -ToolCmd $Tool -Args @("--port", $Port, "erase_flash")
$WriteFlashArgs = @("--port", $Port, "write_flash") + $WriteArgs + $FileArgs
Invoke-Tool -ToolCmd $Tool -Args $WriteFlashArgs
""",
        encoding="utf-8",
    )


def _chip_family_from_target(target: str) -> str:
    """Map profile target to ESP Web Tools chip family."""
    mapping = {
        "esp32": "ESP32",
        "esp32s3": "ESP32-S3",
        "esp32c6": "ESP32-C6",
        "esp32p4": "ESP32-P4",
    }
    return mapping.get(str(target).lower(), str(target).upper())


def _ensure_output_not_exists(path: Path) -> bool:
    """Fail fast on existing output directories."""
    if path.exists():
        print(f"{C_RED}Error: output path already exists: {path}{C_RESET}")
        print("Use a new version tag or remove the existing output directory.")
        return False
    return True


def _regenerate_cdn_index():
    """Rebuild releases/index.json from all *-cdn/manifest.json files."""
    entries = []
    if RELEASES_DIR.exists():
        for manifest_path in sorted(RELEASES_DIR.glob("flxos-*-cdn/manifest.json")):
            match = re.fullmatch(r"flxos-(.+)-v([^/]+)-cdn", manifest_path.parent.name)
            if not match:
                continue
            profile_id = match.group(1)
            version = match.group(2)

            try:
                manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            except Exception:
                continue

            profile_data: dict[str, Any] = {}
            profile_yaml = PROFILES_DIR / profile_id / "profile.yaml"
            if profile_yaml.exists():
                try:
                    profile_data = parse_yaml(profile_yaml)
                except Exception:
                    profile_data = {}

            distribution = profile_data.get("distribution", {})
            if not isinstance(distribution, dict):
                distribution = {}
            tags = distribution.get("tags", [])
            if not isinstance(tags, list):
                tags = []

            warning_message = distribution.get("warning_message")
            if warning_message is not None:
                warning_message = str(warning_message)

            entries.append(
                {
                    "profile": profile_id,
                    "target": str(profile_data.get("target", "")),
                    "version": version,
                    "name": str(manifest.get("name", "")),
                    "manifest": manifest_path.relative_to(RELEASES_DIR).as_posix(),
                    "tags": [str(tag) for tag in tags],
                    "incubating": bool(distribution.get("incubating", False)),
                    "warning_message": warning_message,
                }
            )

    entries.sort(key=lambda item: (item["profile"], item["version"]))
    RELEASES_DIR.mkdir(parents=True, exist_ok=True)
    (RELEASES_DIR / "index.json").write_text(json.dumps(entries, indent=2) + "\n", encoding="utf-8")


def _get_cached_idf_target() -> Optional[str]:
    """Read IDF_TARGET from CMake cache — the actual source of truth for compiler toolchain."""
    cache_file = BUILD_DIR / "CMakeCache.txt"
    if cache_file.exists():
        with open(cache_file) as f:
            for line in f:
                m = re.match(r'^IDF_TARGET:STRING=(.+)', line.strip())
                if m:
                    return m.group(1)
    return None


def _set_sdkconfig_value(filepath: Path, key: str, value: str, create_if_missing: bool = True):
    """Set or update a key=value in an sdkconfig-style file."""
    pattern = re.compile(rf'^{re.escape(key)}=')

    lines = []
    found = False
    if filepath.exists():
        with open(filepath) as f:
            for line in f:
                if pattern.match(line):
                    lines.append(f'{key}="{value}"\n')
                    found = True
                else:
                    lines.append(line)

    if not found and create_if_missing:
        lines.append(f'{key}="{value}"\n')

    if lines or create_if_missing:
        with open(filepath, "w") as f:
            f.writelines(lines)


# ── Main ──────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        prog="flxos",
        description="FlxOS — Unified Build System CLI",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command", help="Command to run")

    # list
    p_list = sub.add_parser("list", help="List all available profiles")
    p_list.add_argument("--json", action="store_true", help="Output machine-readable JSON")

    # select
    p_sel = sub.add_parser("select", help="Select a profile for building")
    p_sel.add_argument("profile_id", help="Profile ID (folder name under Profiles/)")

    # build
    p_build = sub.add_parser("build", help="Build the current profile")
    p_build.add_argument("--all", action="store_true", help="Build all profiles")
    p_build.add_argument("--dev", action="store_true", help="Dev mode: force 4MB partition for faster flash")

    # validate
    p_val = sub.add_parser("validate", help="Validate profile YAML files")
    p_val.add_argument("profile_id", nargs="?", default=None, help="Profile ID (or all)")

    # info
    p_info = sub.add_parser("info", help="Show detailed profile information")
    p_info.add_argument("profile_id", help="Profile ID")

    # new
    p_new = sub.add_parser("new", help="Scaffold a new profile")
    p_new.add_argument("profile_id", help="New profile ID (folder name under Profiles/)")
    p_new.add_argument("--target", choices=["esp32", "esp32s3", "esp32c6", "esp32p4"], default="esp32")
    p_new.add_argument("--flash-size", dest="flash_size", choices=["4MB", "8MB", "16MB"], default="4MB")
    p_new.add_argument("--headless", action="store_true", help="Create as headless profile")
    p_new.add_argument("--vendor", default=None, help="Vendor name")
    p_new.add_argument("--name", default=None, help="Board name (comma-separated for aliases)")
    p_new.add_argument(
        "--template",
        default=None,
        help="Base template name (e.g. base-headless, base-esp32-spi, _bases/base-headless)",
    )

    # diff
    p_diff = sub.add_parser("diff", help="Show side-by-side profile differences")
    p_diff.add_argument("profile_a", help="Left profile ID")
    p_diff.add_argument("profile_b", help="Right profile ID")
    p_diff.add_argument("--json", action="store_true", help="Output machine-readable JSON")

    # flash
    p_flash = sub.add_parser("flash", help="Flash current build to device")
    p_flash.add_argument("--port", default=None, help="Serial port (e.g. /dev/ttyUSB0)")

    # release
    p_release = sub.add_parser("release", help="Package release binaries and symbols")
    p_release.add_argument("version", help="Release version (e.g. 1.0.0 or v1.0.0)")

    # cdn
    p_cdn = sub.add_parser("cdn", help="Generate ESP Web Tools CDN manifests")
    p_cdn.add_argument("version", help="Release version (e.g. 1.0.0 or v1.0.0)")

    # doctor
    sub.add_parser("doctor", help="Check build environment")

    # hwgen
    p_hwgen = sub.add_parser("hwgen", help="Generate HWD init scaffold from profile.yaml")
    p_hwgen.add_argument("profile_id", nargs="?", default=None, help="Profile ID (defaults to selected profile)")
    p_hwgen.add_argument("--all", action="store_true", help="Generate scaffold for all profiles")
    p_hwgen.add_argument("--force", action="store_true", help="Overwrite non-generated output files")
    p_hwgen.add_argument("--stdout", action="store_true", help="Print generated scaffold instead of writing files")
    p_hwgen.add_argument("--output", default=None, help="Custom output path (single profile only)")

    args = parser.parse_args()

    if args.command is None:
        parser.print_help()
        return 0

    commands = {
        "list": cmd_list,
        "select": cmd_select,
        "build": cmd_build,
        "validate": cmd_validate,
        "info": cmd_info,
        "new": cmd_new,
        "diff": cmd_diff,
        "flash": cmd_flash,
        "release": cmd_release,
        "cdn": cmd_cdn,
        "doctor": cmd_doctor,
        "hwgen": cmd_hwgen,
    }

    return commands[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
