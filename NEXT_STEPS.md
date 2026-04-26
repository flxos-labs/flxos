# FlxOS — Next Steps

> Based on: 44 recent commits on `develop`, `MEMORY_OPTIMIZATION_PLAN.md`, roadmap in `README.md`, and current codebase state.

---

## Current State Summary

| Area | Status |
|---|---|
| Memory optimization (P1–P4) | **All done** except P4-B (deferred — dangling `FlxValueView` risk) |
| Working tree | Clean — all work committed on `develop` |
| Version | `0.1.0` |
| SPIRAM test profiles | Still disabled on `ili9341-xpt` and `lilygo-t-hmi` (commits `d855291`, `0354eae`) |
| `develop` → `main` merge | Pending — 44 commits ahead |
| Hardware validation | ⚠️ **Not yet done** — ESP-IDF not available in this environment |

---

## 🔴 Tier 1 — Immediate (This Week)

### 1. Hardware-Validate the Memory Optimizations
The entire memory optimization plan has a single critical gap: **no firmware has been built or tested on hardware**. The verification checklist in `MEMORY_OPTIMIZATION_PLAN.md` (line 479–500) is still unconfirmed.

**Action items:**
- Build for all 3 graphical profiles (`esp32s3-ili9341-xpt`, `lilygo-t-hmi`, `cyd-2432s028r`)
- Flash and run the verification commands (`heap`, `tasks`, `diagnostics`)
- Validate stack HWM headroom for the reduced stacks (GuiTask 16KB, draw thread 4KB, main 8KB)
- Run the **soak test**: open → close 5 apps in a loop for 5 minutes, check heap returns to idle baseline

### 2. Re-Enable SPIRAM on Test Profiles
Commits `d855291` and `0354eae` disabled SPIRAM on the two S3 boards **for testing only**. These must be reverted before any release.

```bash
git revert d855291 0354eae --no-commit
git commit -m "profile: Re-enable SPIRAM on S3 boards after memory testing"
```

### 3. Merge `develop` → `main`
`develop` is 44 commits ahead of `main`. Once hardware validation passes, this should be merged and tagged as `v0.1.1` (or whatever versioning scheme you prefer for this optimization milestone).

---

## 🟡 Tier 2 — Short-Term (Next 2–4 Weeks)

### 4. OTA Update Workflows
Listed in the README roadmap. This is a production-critical capability:
- ESP-IDF's `esp_ota_ops` provides the low-level primitives
- Need: an OTA service in `Services/`, UI integration in Settings, and CDN manifest support (the `cdn` CLI command already exists)
- Consider: rollback partition support, version checking, progress UI

### 5. Host-Based Simulator
Also in the README roadmap. This would **massively** accelerate UI and app iteration:
- LVGL has official SDL2/X11 backends for desktop simulation
- Would require a `host` profile type with mock HAL implementations
- The existing profile system (`Profiles/`) is well-suited for this — just add a `native-host` profile
- Blocks on: abstracting ESP-IDF dependencies in `Core`, `Kernel`, and `System`

### 6. FlxApp Declarative Framework Hardening
The declarative app framework (`FlxApp/`) is functional but young:
- **P4-B (deferred):** Investigate releasing `m_document` after `createUI()` to save 5–15 KB per app. Requires refactoring `EventBinding::actions` to copy values instead of holding `FlxValueView` references
- **Error handling:** `FlxApp::loadDocument()` silently fails on malformed apps — add user-visible error notifications
- **More widget types:** The renderer likely supports a subset of LVGL widgets; expand coverage based on app needs
- **Hot-reload in dev mode:** Re-parse and re-render the JSON/YAML source without restarting the app

### 7. Text Editor Improvements
The Text Editor app exists but is minimal. For a desktop OS feel:
- Syntax highlighting (at least for JSON/YAML — relevant for editing FlxApp files)
- Save/undo functionality
- Integration with the Files app for "Open With" intent handling

### 8. Theming Branch
There's a `remotes/origin/enhance_theming` branch that hasn't been merged. Review and integrate or close it. The project guidelines already recommend `Themes::GetConfig(...)` — this branch may add more comprehensive theming support.

---

## 🟢 Tier 3 — Medium-Term (1–2 Months)

### 9. CI Build Matrix
Currently CI has `build.yml` and `code-quality.yml`, but builds may not cover all 5 profiles. Add:
- Matrix builds for all graphical profiles
- Binary size tracking (flash + RAM delta per PR)
- Automated `flxos.py validate` for all profiles

### 10. Plugin Ecosystem
Listed in the README roadmap as "externally loaded applications":
- Could leverage the existing `FlxApp` JSON/YAML declarative format as the plugin format
- Need: app store / sideloading via SD card or HTTP
- Need: sandboxing and capability declarations (the manifest system already has a foundation)

### 11. Service Health Improvements
`ServiceRegistry` is 21 KB (substantial). Potential improvements:
- Health check dashboards visible in System Info app
- Watchdog recovery — auto-restart failed services
- Dependency graph visualization for debugging

### 12. Connectivity Hardening
Bluetooth and hotspot modules exist but maturity is unknown:
- Verify BLE scanning, pairing, and data transfer
- Hotspot mode for device-to-device configuration
- mDNS discovery for OTA and debug tooling

---

## Recommended Priority Order

| # | Task | Why First |
|---|---|---|
| 1 | Hardware validate memory work | **Everything else is blocked** until the 44 commits on `develop` are proven on hardware |
| 2 | Re-enable SPIRAM + merge to `main` | Clean up the test state, establish a stable baseline |
| 3 | Host simulator | Unlocks faster iteration for everything below |
| 4 | OTA updates | Production-critical — users need remote update capability |
| 5 | FlxApp hardening | Makes the declarative app system production-ready |
| 6 | CI build matrix | Prevents regressions as the codebase grows |

> [!IMPORTANT]
> The single most impactful next step is **building and flashing the firmware** to validate the memory optimizations. All code changes are committed and ready — it's purely a hardware-test gap.
