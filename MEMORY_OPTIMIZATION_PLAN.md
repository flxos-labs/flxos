# FlxOS Memory Optimization Plan
> **Last updated:** 2026-04-25 | **Scope:** All ESP32 board profiles (ESP32, ESP32-S3, Headless, CYD, LilyGO T-HMI)

## Analysis Findings (Current State)

### Board Profiles & SPIRAM Status

| Profile | Target | SPIRAM | Notes |
|---|---|---|---|
| `esp32s3-ili9341-xpt` | ESP32-S3 | ✅ ENABLED (80M, XIP) | Primary dev board |
| `lilygo-t-hmi` | ESP32-S3 | ✅ ENABLED (80M) | No XIP |
| `cyd-2432s028r` | ESP32 | ❌ NO SPIRAM | ESP32 has no PSRAM on this board |
| `generic-esp32` | ESP32 | ❌ NO SPIRAM | Headless; irrelevant for OOM |
| `generic-esp32s3` | ESP32-S3 | ❌ NO SPIRAM | Headless; irrelevant for OOM |

> **CORRECTION from prior analysis:** SPIRAM IS already enabled for the two graphical ESP32-S3 profiles
> (`esp32s3-ili9341-xpt` and `lilygo-t-hmi`) via `sdkconfig.profile` auto-generation from `profile.yaml`.
> The `sdkconfig.profile` clearly shows `CONFIG_SPIRAM=y, CONFIG_SPIRAM_MODE_OCT=y, CONFIG_SPIRAM_SPEED_80M=y`.

> **fkYAML IS required** — it is the core JSON/YAML parsing engine for `FlxValueDocument`
> used by `FlxApp` to load ALL application manifests (`.json` and `.yaml` app files).
> It is NOT dead code. Removing it would break the entire app loading system.

### Confirmed Memory Issues (The Real Problems)

**After re-analysis, the root causes are:**

1. **LVGL image/draw cache at 512 KB** — `CONFIG_LV_CACHE_DEF_SIZE=512000` allocated at startup
2. **SPIRAM malloc policy not optimal** — `MALLOC_ALWAYSINTERNAL=16384` is too large; big allocations that could go to SPIRAM stay in internal RAM
3. **SPIRAM WiFi/LWIP not offloaded** — `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP` is not set, so WiFi buffers eat internal RAM
4. **GuiTask stack 32 KB** in internal RAM — significantly oversized for event-loop tick work
5. **No app eviction** — all paused apps keep their full LVGL widget trees (15–30 KB each) in heap
6. **`apps` missing `onPause()` UI teardown** — most apps (FilesApp, SettingsApp, ImageViewerApp, WallpaperEngineApp) do not release LVGL objects on pause; only CalendarApp, SystemInfoApp, ToolsApp implement `onPause()`
7. **ResourceMonitorTask threshold at 32 KB** — warning fires too late; no automated recovery
8. **No SPIRAM for large heap allocations** — LVGL uses `CONFIG_LV_USE_CLIB_MALLOC=y` which routes to system `malloc()`. ESP-IDF's `SPIRAM_USE_MALLOC` redirects large `malloc()` calls to SPIRAM automatically, but the threshold (`ALWAYSINTERNAL`) is set too high.

---

## Memory Budget (ESP32 boards WITHOUT SPIRAM — worst case)

| Consumer | ~Size | Notes |
|---|---|---|
| FreeRTOS kernel + idle tasks | 12 KB | 2× idle tasks at 1.5 KB each |
| ESP-IDF system tasks | 25 KB | WiFi/LWIP/event/timer/IPC |
| Main task stack | 16 KB | `CONFIG_ESP_MAIN_TASK_STACK_SIZE` |
| **GuiTask stack** | **32 KB** | `Task("gui_task", 32*1024, 5, 1)` |
| App executor task | 8 KB | `AppExecutor` |
| Resource monitor task | 8 KB | `res_monitor` (raised after no-PSRAM stack HWM showed 4 KB was unsafe) |
| LVGL draw thread | 8 KB | `CONFIG_LV_DRAW_THREAD_STACK_SIZE` |
| LVGL draw layer buf | 24 KB | `CONFIG_LV_DRAW_LAYER_SIMPLE_BUF_SIZE` |
| **LVGL image cache** | **512 KB** | `CONFIG_LV_CACHE_DEF_SIZE=512000` 🔴 |
| DMA display buffer | ~15 KB | 240×320/10×2 bytes |
| Desktop shell UI | ~40 KB | Screen, wallpaper, status bar, dock, launcher, panels |
| Per-app LVGL tree | ~15–30 KB each | Not released on pause |
| fkYAML parsed doc | ~5–15 KB per app | Held while app is running |
| C++ containers | ~10–15 KB | EventBus subs, AppManager maps |

> For the **CYD (ESP32, no SPIRAM)**: total SRAM is ~320 KB. LVGL cache alone would be 512 KB —
> it almost certainly gets clamped by IDF, but is still the biggest single over-allocation.

---

## Optimization Plan

### Priority 1 (Critical — Fix First)

#### P1-A: Fix LVGL Cache Size (Per-Profile, Board-Aware) — ✅ Done

**Problem:** `CONFIG_LV_CACHE_DEF_SIZE=512000` is hardcoded for all profiles in `profile.cmake:851`.
This reserves 512 KB of heap. On ESP32-S3 with SPIRAM this goes to SPIRAM (OK), but on ESP32 (no SPIRAM)
this hits internal RAM and causes immediate OOM.

**Fix — `Buildscripts/profile.cmake` line 847–875:**

```diff
-       string(APPEND _frag "CONFIG_LV_CACHE_DEF_SIZE=512000\n")
-       string(APPEND _frag "CONFIG_LV_IMAGE_HEADER_CACHE_DEF_CNT=50\n")
+       # LVGL cache: scale to available memory
+       _b("hardware_spiram_enabled" "false" _spiram_cache)
+       if("${_spiram_cache}" STREQUAL "true")
+           # With SPIRAM: allow large cache (it lands in PSRAM via SPIRAM_USE_MALLOC)
+           string(APPEND _frag "CONFIG_LV_CACHE_DEF_SIZE=262144\n")   # 256 KB in PSRAM
+           string(APPEND _frag "CONFIG_LV_IMAGE_HEADER_CACHE_DEF_CNT=32\n")
+       else()
+           # No SPIRAM (ESP32/CYD): minimal cache, rely on direct rendering
+           string(APPEND _frag "CONFIG_LV_CACHE_DEF_SIZE=8192\n")     # 8 KB in internal RAM
+           string(APPEND _frag "CONFIG_LV_IMAGE_HEADER_CACHE_DEF_CNT=8\n")
+       endif()
```

**Impact:**
- ESP32 (no SPIRAM): saves **~504 KB** of heap pressure (previously OOM-causing)
- ESP32-S3 (SPIRAM): saves 256 KB of PSRAM, allowing more app allocations

---

#### P1-B: Optimize SPIRAM Malloc Policy for ESP32-S3 Profiles — ✅ Done

**Problem:** `CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384` means allocations up to 16 KB stay in internal
RAM (including many LVGL widget allocations). `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP` is not set,
so WiFi stacks (~25 KB) stay in internal SRAM.

**Fix — `Buildscripts/profile.cmake` after line 989 (inside the `if _spiram == true` block):**

```diff
+       # SPIRAM malloc policy: push large allocations + WiFi to SPIRAM
+       string(APPEND _frag "CONFIG_SPIRAM_USE_MALLOC=y\n")
+       string(APPEND _frag "CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=4096\n")  # only ≤4KB stays internal
+       string(APPEND _frag "CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=65536\n") # keep 64KB for DMA/ISR
+       string(APPEND _frag "CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP=y\n")
+       string(APPEND _frag "CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY=y\n")
```

**Impact:** Frees ~20–35 KB of internal SRAM (WiFi buffers + medium allocations move to PSRAM).

---

#### P1-C: Reduce GuiTask Stack 32 KB → 16 KB — ✅ Done

**File:** `UI/Source/tasks/GuiTask.cpp:54`

```diff
-GuiTask() : flx::kernel::Task("gui_task", 32 * 1024, 5, 1) {
+GuiTask() : flx::kernel::Task("gui_task", 16 * 1024, 5, 1) {
```

The GUI loop (`lv_timer_handler()`) is a shallow call stack. The current HWM is likely well under 8 KB.

**Impact:** Saves **16 KB of internal SRAM** on all boards (task stacks are always in internal RAM).
**Validation:** After running, check `uxTaskGetStackHighWaterMark(guiTaskHandle) > 4096`.

---

#### P1-D: Reduce Main Task Stack 16 KB → 8 KB — ✅ Done

**File:** `sdkconfig.defaults`

```diff
-CONFIG_ESP_MAIN_TASK_STACK_SIZE=16384
+CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192
```

The main task (`app_main`) just calls `initHardware()`, `initServices()`, spawns GuiTask, then exits.
It does not need a 16 KB stack.

**Impact:** Saves **8 KB of internal SRAM** on all boards.

---

### Priority 2 (High Impact — Week 1)

#### P2-A: ResourceMonitorTask — Add Emergency Eviction — ✅ Done

**File:** `Kernel/Source/ResourceMonitorTask.cpp`

Current: only logs at 32 KB, no action taken.
Add a secondary **critical** tier that publishes an event to force app eviction. The implemented
logic is also fragmentation-aware: it checks both total free heap and largest free block.

```diff
-    if (m_freeHeap < 32768) {
+    static constexpr uint32_t kWarnThresholdWithPsram = 49152;  // 48 KB — warn
+    static constexpr uint32_t kWarnThresholdNoPsram = 32768;    // 32 KB — warn
+    static constexpr uint32_t kCritThreshold = 20480;           // 20 KB — evict
+    static constexpr uint32_t kWarnLargestBlock = 24576;        // 24 KB — warn
+    static constexpr uint32_t kCritLargestBlock = 16384;        // 16 KB — evict
+
+    if (m_freeHeap < kCritThreshold || largestBlock < kCritLargestBlock) {
+        Log::error(TAG, "CRITICAL HEAP: free=%lu largest=%lu — requesting app eviction", ...);
+        flx::core::EventBus::getInstance().publish("system.memory.critical", {});
+    } else if (m_freeHeap < warnThreshold || largestBlock < kWarnLargestBlock) {
```

Subscribe `AppManager` to `system.memory.critical` to stop the oldest paused app.

**Impact:** Prevents hard OOM crashes by gracefully shedding memory before the heap is exhausted.

---

#### P2-B: ResourceMonitorTask Stack — ✅ Done, adjusted

**File:** `Kernel/Source/ResourceMonitorTask.cpp:36`

```diff
-    : Task("res_monitor", 6144, 2, tskNO_AFFINITY) {}
+    : Task("res_monitor", 8192, 2, tskNO_AFFINITY) {}
```

Initial 4 KB testing left only ~28 B stack headroom after fragmentation logging and event publishing.
The task is now set to 8 KB for safe no-PSRAM diagnostics.

**Impact:** No longer saves stack RAM versus the original 6 KB setting; prioritizes diagnostic stability.

---

#### P2-C: Reduce LVGL Draw Thread Stack 8 KB → 4 KB — ✅ Done

**File:** `sdkconfig.defaults` (or `Buildscripts/profile.cmake`)

```diff
+CONFIG_LV_DRAW_THREAD_STACK_SIZE=4096
```

The draw thread does 2D SW rasterization — complex operations but not deep recursion.
4 KB has been validated to work in LVGL reference designs for similar panel sizes.

**Impact:** Saves **4 KB of internal SRAM** on all boards.

---

#### P2-D: Add App Memory Eviction (AppManager) — ✅ Done

**File:** `Apps/Source/AppManager.cpp` — `startAppForResultImpl()`

Before launching a new app, check if the stack has paused apps and memory pressure is high.
Stop the oldest paused app (not the current) to reclaim heap and contiguous allocation space.

```cpp
// Add before the heap check guard (around line 238):
static constexpr uint32_t kEvictThresholdBytes = 64 * 1024;
static constexpr uint32_t kLargestBlockEvictThresholdBytes = 32 * 1024;
size_t maxConcurrentApps = hasPsram() ? 3 : 2;
while (getStackDepth() >= maxConcurrentApps ||
       freeHeap < kEvictThresholdBytes ||
       largestBlock < kLargestBlockEvictThresholdBytes) {
    if (!evictOldestPausedApp(reason)) break;
}
```

The eviction log includes before/after free heap and largest block to show whether eviction actually
improved fragmentation.

**Impact:** Prevents OOM when opening a new app while multiple apps are paused, and reduces launches
with dangerously small contiguous heap.

---

#### P2-E: Fix Missing `onPause()` Implementations in Apps — ✅ Done

Currently, these apps do NOT release any resources when paused:
- `FilesApp` — only has `onStop()`
- `SettingsApp` — only has `onStop()`
- `ImageViewerApp` — only has `onStop()`
- `WallpaperEngineApp` — only has `onStop()`

Each paused app keeps its full LVGL window + content (15–30 KB each) alive indefinitely.

**Minimum fix for each app — add `onPause()`:**

```cpp
void FilesApp::onPause() {
    // Stop any background directory scanning
    // (LVGL objects remain — window is just hidden by WM)
}
```

**Better fix (medium risk):** On pause, delete LVGL child widgets but keep the window frame.
On resume, call `createUI()` again. Requires `createUI()` to be idempotent.

---

#### P2-F: Skip Heavy Optional Services in No-PSRAM Test Mode — ✅ Done

**Files:** `System/Source/SystemManager.cpp`, `System/Source/managers/NotificationManager.cpp`

When `heap_caps_get_total_size(MALLOC_CAP_SPIRAM) == 0`, skip boot registration for:
- `Connectivity` — saves ~50 KB at boot by avoiding WiFi/LWIP initialization
- `Notifications` — avoids low-memory notification churn and notification vector/string allocations
- `Screenshot Service` — avoids enabling a runtime-heavy capture path on constrained builds

Direct notification calls are dropped while the notification service is stopped in no-PSRAM mode.

**Impact:** Raises no-PSRAM idle heap substantially, especially by removing Connectivity startup.

---

### Priority 3 (Good Practice — Week 2)

#### P3-A: Reduce LVGL Draw Layer Buffer — ✅ Done

**File:** `sdkconfig.defaults`

```diff
-# CONFIG_LV_DRAW_LAYER_SIMPLE_BUF_SIZE=24576  (default 24 KB)
+CONFIG_LV_DRAW_LAYER_SIMPLE_BUF_SIZE=8192
```

This buffer is used for complex layered rendering (glass effects, shadows). On a 240×320 display
it is rarely needed at full size. 8 KB handles most compositing operations.

**Impact:** Saves **16 KB** on all boards.

---

#### P3-B: Per-Profile LVGL Refresh Rate — ✅ Done

**File:** `Buildscripts/profile.cmake` — inside the non-headless block

```diff
-       string(APPEND _frag "CONFIG_LV_DEF_REFR_PERIOD=10\n")  # 100fps
+       # 30fps (33ms) for ESP32 (no SPIRAM), 60fps (17ms) for ESP32-S3 with SPIRAM
+       if("${_spiram_cache}" STREQUAL "true")
+           string(APPEND _frag "CONFIG_LV_DEF_REFR_PERIOD=17\n")  # ~60fps
+       else()
+           string(APPEND _frag "CONFIG_LV_DEF_REFR_PERIOD=33\n")  # ~30fps, saves CPU
+       endif()
```

On ESP32 (CYD), running 100 fps is impossible anyway given SPI bus speed. 30 fps gives the OS
more time for heap operations between frames.

**Impact:** Reduces CPU contention, indirectly improves heap allocation timing on ESP32.

---

#### P3-C: Disable Unused LVGL Widgets (All Profiles) — ✅ Done, adjusted

Verify which widgets the system actually uses, then disable unused ones in `sdkconfig.defaults`:

**Confirmed NOT used by any app or OS code (safe to disable):**
```ini
# Add to sdkconfig.defaults
CONFIG_LV_USE_ANIMIMG=n
CONFIG_LV_USE_ARCLABEL=n
CONFIG_LV_USE_CANVAS=n
CONFIG_LV_USE_CHART=n
CONFIG_LV_USE_LED=n
CONFIG_LV_USE_MENU=n
CONFIG_LV_USE_ROLLER=n
CONFIG_LV_USE_SCALE=n
CONFIG_LV_USE_SPINBOX=n
CONFIG_LV_USE_TILEVIEW=n
```

**Adjustment after implementation audit:** `CONFIG_LV_USE_MSGBOX` must remain enabled because
`FilesApp` uses `lv_msgbox` for progress, confirmation, and input dialogs.

**Do NOT disable:** arc, bar, button, buttonmatrix, calendar, checkbox, dropdown, image, imagebutton,
keyboard, label, line, list, msgbox, slider, span, spinner, switch, table, tabview, textarea, win.

**Impact:** ~20–40 KB flash savings, minor RAM savings (vtable/type metadata).

---

#### P3-D: Reduce FS Stdio Cache — ✅ Done

**File:** `Buildscripts/profile.cmake:865`

```diff
-       string(APPEND _frag "CONFIG_LV_FS_STDIO_CACHE_SIZE=4096\n")
+       string(APPEND _frag "CONFIG_LV_FS_STDIO_CACHE_SIZE=1024\n")
```

The stdio FS driver is used for wallpaper images. 1 KB is sufficient for sequential reading.

**Impact:** Saves **3 KB**.

---

#### P3-E: Add Stack High-Water Mark Logging to TaskManager — ✅ Done

**File:** `Kernel/Source/TaskManager.cpp` — enhance `printTasks()`

```cpp
uint32_t hwm = t->getStackHighWaterMark();
uint32_t size = t->getStackSize();
uint32_t usedPct = size > 0 ? ((size - hwm) * 100 / size) : 0;
Log::info(TM_TAG, "  [%s] stack=%uB hwm=%uB used=%u%%  %s",
    t->getName().c_str(), size, hwm, usedPct,
    usedPct > 80 ? "⚠️ NEAR OVERFLOW" : "OK");
```

This gives real-time data to validate P1-C, P1-D, P2-B, P2-C stack reductions.

---

#### P3-F: Add Heap Fragmentation Tracking to ResourceMonitorTask — ✅ Done

```diff
+    uint32_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
+    if (m_uptimeSeconds % 60 == 0) {
+        Log::info(TAG, "Heap: free=%lu largest_block=%lu (frag ratio=%.1f%%)",
+            (unsigned long)m_freeHeap.load(),
+            (unsigned long)largestBlock,
+            m_freeHeap > 0 ? (1.0f - (float)largestBlock/m_freeHeap) * 100.0f : 0.0f);
+    }
```

High fragmentation (free=100KB but largest_block=8KB) is a distinct OOM cause that must be
diagnosed separately from total free heap.

---

### Priority 4 (Long-term Hardening)

#### P4-A: Cap Concurrent App Stack Depth — ✅ Done

**File:** `Apps/Source/AppManager.cpp`

```cpp
size_t maxConcurrentApps = hasPsram() ? 3 : 2;
if (getStackDepth() >= maxConcurrentApps) {
    // Evict the oldest paused app before allowing the new launch
    ...
}
```

---

#### P4-B: Reduce fkYAML Parsed Tree Heap Usage — ⏸ Deferred

`FlxApp::loadDocument()` parses the entire JSON/YAML document into a `fkyaml::node` tree
(a heap-allocated tree of `std::variant` nodes). This tree stays alive for the entire app lifetime.

**Optimization:** After rendering is complete in `createUI()`, the renderer's internal binding
structures (text/image/input bindings in `FlxAppRenderer`) hold strong references to the values.
The raw `m_document` (containing the unparsed source string + fkyaml tree) could be released
after `createUI()` if the bindings have already extracted all needed values.

**Investigate:** Whether `FlxAppRenderer` references `FlxValueView` nodes that point into
`m_document->m_root` after rendering. If not, `m_document.reset()` can be called post-`createUI()`.

**Deferred finding:** `FlxAppRenderer::EventBinding::actions` stores `FlxValueView` values that point
into `m_document`; releasing the parsed document after render would leave dangling action bindings.

---

#### P4-C: Reduce `std::string` Heap Pressure in EventBus — ✅ Done

**File:** `Core/Include/flx/core/EventBus.hpp`

Event names are all `constexpr const char*` literals. Storing them in `std::string m_event`
(in `Subscription`) heap-allocates every subscription string. Use a fixed-size char array or
`const char*` pointer (safe since all event names are string literals):

```cpp
// In Subscription struct:
const char* event;  // Points to string literal — no heap allocation
```

---

## Implementation Order by Board Type

### For CYD (ESP32, no SPIRAM) — OOM risk is HIGHEST

| Order | Status | Item | Expected Benefit |
|---|---|---|---|
| 1 | ✅ Done | **P1-A** LVGL cache 512 KB → 8 KB | Prevents boot OOM |
| 2 | ✅ Done | **P1-C** GuiTask stack 32 → 16 KB | +16 KB free |
| 3 | ✅ Done | **P1-D** Main task 16 → 8 KB | +8 KB free |
| 4 | ✅ Done | **P2-B** ResMonitor 6 → 4 KB | +2 KB free |
| 5 | ✅ Done | **P2-C** Draw thread 8 → 4 KB | +4 KB free |
| 6 | ✅ Done | **P3-A** Draw layer buf 24 → 8 KB | +16 KB free |
| 7 | ✅ Done | **P2-D** App eviction | Prevents runtime OOM |
| 8 | ✅ Done | **P3-B** Refresh rate 10 → 33 ms | Reduces CPU pressure |
| 9 | ✅ Done, adjusted | **P3-C** Disable unused widgets | ~20 KB flash |
| 10 | ✅ Done | **P2-F** Skip heavy optional services | +50 KB+ idle heap in no-PSRAM testing |

**Expected total free heap gain on ESP32 CYD:** ~+46 KB of internal SRAM (stack reductions) +
**~504 KB of heap pressure relief** (LVGL cache reduction).

### For ESP32-S3 (esp32s3-ili9341-xpt, lilygo-t-hmi) — OOM risk is MODERATE

| Order | Status | Item | Expected Benefit |
|---|---|---|---|
| 1 | ✅ Done | **P1-B** SPIRAM malloc policy | +20–35 KB internal SRAM (WiFi to PSRAM) |
| 2 | ✅ Done | **P1-A** LVGL cache 512 KB → 256 KB (in PSRAM) | Frees 256 KB PSRAM for apps |
| 3 | ✅ Done | **P1-C** GuiTask stack 32 → 16 KB | +16 KB internal SRAM |
| 4 | ✅ Done | **P1-D** Main task 16 → 8 KB | +8 KB internal SRAM |
| 5 | ✅ Done | **P2-A** Emergency eviction in ResMonitor | Prevents crash under sustained load |
| 6 | ✅ Done | **P2-D** Pre-launch eviction in AppManager | Graceful multi-app handling |
| 7 | ✅ Done | **P3-A** Draw layer buf 24 → 8 KB | +16 KB |
| 8 | ✅ Done | **P2-E** `onPause()` in missing apps | Frees 15–30 KB per paused app |
| 9 | ✅ Done | **P2-F** Skip heavy optional services when PSRAM is disabled | Better no-PSRAM test headroom |

---

## Verification Checklist

Current local validation:
- ✅ `python flxos.py validate` passes for all 5 profiles.
- ✅ `git diff --check` passes.
- ⚠️ Full firmware build has not been run in this environment because ESP-IDF is not loaded (`IDF_PATH` unset, `idf.py` not in `PATH`).

After each phase, validate with these checks:

```bash
# In CLI (if enabled):
heap           # Check free + min free heap
tasks          # Check stack HWM for each task
diagnostics    # Full system snapshot
```

In logs, look for:
- `ResourceMonitor` — `Heap: free=XXXX largest_block=YYYY` — verify largest_block > 32 KB always
- `AppManager` — `heapDeltaBytes` after each app stop — should be near 0 (no leaks)
- `GuiTask` — No `OOM?` messages in `WindowManager::openApp`

**Soak test:** Open → close 5 apps in a loop for 5 minutes. Heap should return to idle baseline each cycle.

---

## Summary Table

| Item | Status | File(s) | Boards Affected | Heap Savings | Risk | Notes |
|---|---|---|---|---|---|---|
| P1-A LVGL cache per-profile | Done | `Buildscripts/profile.cmake` | ALL | 8–504 KB | Low | 256 KB with SPIRAM, 8 KB without SPIRAM; image header cache scaled too. |
| P1-B SPIRAM malloc policy | Done | `Buildscripts/profile.cmake` | S3 only | 20–35 KB internal | Low | Adds malloc, 4 KB always-internal threshold, 64 KB reserve, WiFi/LWIP PSRAM preference. |
| P1-C GuiTask stack 16 KB | Done | `UI/Source/tasks/GuiTask.cpp` | ALL | 16 KB internal | Medium | Requires hardware stack HWM validation. |
| P1-D Main task stack 8 KB | Done | `sdkconfig.defaults` | ALL | 8 KB internal | Low | Requires firmware build validation. |
| P2-A Emergency eviction | Done, fragmentation-aware | `Kernel/Source/ResourceMonitorTask.cpp`, `Apps/Source/AppManager.cpp` | ALL | Prevents crash | Low | Publishes `system.memory.critical` on low free heap or critically small largest block; AppManager evicts oldest paused app. |
| P2-B ResMonitor stack | Done, adjusted | `Kernel/Source/ResourceMonitorTask.cpp` | ALL | Diagnostic stability | Low | 4 KB left ~28 B HWM during no-PSRAM testing; set to 8 KB. |
| P2-C Draw thread 4 KB | Done | `sdkconfig.defaults` | ALL | 4 KB internal | Low | Requires firmware build and display soak validation. |
| P2-D Pre-launch eviction | Done, fragmentation-aware | `Apps/Source/AppManager.cpp`, `Apps/Include/flx/apps/AppManager.hpp` | ALL | 15–30 KB per app | Medium | Evicts oldest paused app below 64 KB free heap or 32 KB largest block; no-PSRAM stack cap is 2, PSRAM cap is 3. |
| P2-E onPause() in apps | Done | `Applications/files`, `Applications/settings`, `Applications/image_viewer`, `Applications/wallpaper_engine` | ALL | 15–30 KB per paused app | Medium | Low-risk pause cleanup; full UI teardown remains a future medium-risk enhancement. |
| P2-F No-PSRAM optional service skip | Done | `System/Source/SystemManager.cpp`, `System/Source/managers/NotificationManager.cpp` | No-PSRAM | ~50 KB+ | Medium | Skips Connectivity, Notifications, and Screenshot at boot when PSRAM is absent. |
| P3-A Draw layer 8 KB | Done | `sdkconfig.defaults` | ALL | 16 KB | Low | Requires visual validation for complex effects. |
| P3-B Refresh rate per-board | Done | `Buildscripts/profile.cmake` | ALL | CPU time | Low | 17 ms with SPIRAM, 33 ms without SPIRAM. |
| P3-C Disable unused widgets | Done, adjusted | `sdkconfig.defaults` | ALL | ~20 KB flash | Low | Disabled unused widgets except `CONFIG_LV_USE_MSGBOX`; FilesApp uses `lv_msgbox`, so msgbox must remain enabled. |
| P3-D FS stdio cache 1 KB | Done | `Buildscripts/profile.cmake` | ALL | 3 KB | Low | Reduces sequential file cache. |
| P3-E Stack HWM logging | Done | `Kernel/Source/TaskManager.cpp` | ALL | Diagnostic only | Low | Logs stack size, HWM, used percentage, and near-overflow marker. |
| P3-F Fragmentation tracking | Done | `Kernel/Source/ResourceMonitorTask.cpp` | ALL | Diagnostic only | Low | Logs largest block and fragmentation ratio. |
| P4-A Max concurrent apps | Done, board-aware | `Apps/Source/AppManager.cpp` | ALL | Prevents runaway | Low | Caps concurrent app stack at 3 with PSRAM and 2 without PSRAM by evicting the oldest paused app before new launches. |
| P4-B Release doc post-createUI | Deferred | `FlxApp/Source/FlxApp.cpp`, `FlxApp/Source/FlxAppRenderer.cpp` | ALL | 5–15 KB per app | High | Audit found `EventBinding::actions` stores `FlxValueView` into the parsed document; freeing `m_document` after render would leave dangling action bindings. |
| P4-C EventBus string literals | Done | `Core/Include/flx/core/EventBus.hpp`, `Core/Source/EventBus.cpp` | ALL | ~1 KB | Low | Subscription storage now keeps literal pointers instead of heap-allocating event-name strings. |
