#!/usr/bin/env python3
"""
FlxOS CLI — Unified build tool.

Usage:
    python flxos.py list                     List all profiles
    python flxos.py select <id>              Select profile for build
    python flxos.py build [--all] [--dev]    Build current/all profiles
    python flxos.py validate [id]            Validate profile YAMLs
    python flxos.py info <id>                Show profile details
    python flxos.py doctor                   Check build environment
"""

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any, Optional


# ── Constants ──────────────────────────────────────────────────────────────

SCRIPT_DIR = Path(__file__).parent.resolve()
PROFILES_DIR = SCRIPT_DIR / "Profiles"
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

    current = get_current_profile()

    # Table header
    print()
    header = f"{'':2} {'Profile':<25} {'Target':<10} {'Flash':<8} {'Headless':<10} {'Display':<12} {'Tags'}"
    print(f"{C_BOLD}{header}{C_RESET}")
    print(f"{'─' * 95}")

    for p in profiles:
        pid = p.get("id", "?")
        target = p.get("target", "?")
        flash = p.get("flash_size", "?")
        headless = p.get("headless", False)
        display_drv = get_nested(p, "hardware.display.driver", "—")
        tags = p.get("distribution", {}).get("tags", [])
        if isinstance(tags, dict):
            tags = []
        tags_str = ", ".join(str(t) for t in tags) if tags else "—"

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

    # Update sdkconfig.defaults with profile ID
    _set_sdkconfig_value(SDKCONFIG_DEFAULTS, "CONFIG_FLXOS_PROFILE", profile_id)

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

        # Update profile in sdkconfig.defaults (source of truth for profile.cmake)
        _set_sdkconfig_value(SDKCONFIG_DEFAULTS, "CONFIG_FLXOS_PROFILE", pid)

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
    required_fields = ["id", "vendor", "name", "target", "flash_size"]
    valid_targets = ["esp32", "esp32s3", "esp32c6", "esp32p4"]
    valid_flash = ["4MB", "8MB", "16MB"]
    valid_flash_modes = ["QIO", "DIO", "QOUT", "DOUT"]

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

        # Target validation
        target = p.get("target", "")
        if target not in valid_targets:
            errors.append(f"Invalid target '{target}'. Valid: {valid_targets}")

        # Flash size validation
        flash = p.get("flash_size", "")
        if flash not in valid_flash:
            errors.append(f"Invalid flash_size '{flash}'. Valid: {valid_flash}")

        # Flash mode validation
        flash_mode = p.get("flash_mode")
        if flash_mode and flash_mode not in valid_flash_modes and flash_mode is not None:
            errors.append(f"Invalid flash_mode '{flash_mode}'. Valid: {valid_flash_modes}")

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
    print(f"  Vendor:     {data.get('vendor', '?')}")
    print(f"  Name:       {data.get('name', '?')}")
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


# ── Helpers ────────────────────────────────────────────────────────────────

def _get_cached_idf_target() -> Optional[str]:
    """Read IDF_TARGET from CMake cache — the actual source of truth for compiler toolchain."""
    cache_file = SCRIPT_DIR / "build" / "CMakeCache.txt"
    if cache_file.exists():
        with open(cache_file) as f:
            for line in f:
                m = re.match(r'^IDF_TARGET:STRING=(.+)', line.strip())
                if m:
                    return m.group(1)
    return None


def _set_sdkconfig_value(filepath: Path, key: str, value: str):
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

    if not found:
        lines.append(f'{key}="{value}"\n')

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
    sub.add_parser("list", help="List all available profiles")

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

    # doctor
    sub.add_parser("doctor", help="Check build environment")

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
        "doctor": cmd_doctor,
    }

    return commands[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
