# FlxOS Code Quality Report

Generated: Sun Feb 15 02:53:34 PM IST 2026

---

## Format Check

✅ **Passed** - No issues found

## Complexity Analysis

⚠️ **Issues Found**

```
=== FlxOS Code Complexity Analysis ===

⚠️  Complexity Issues Found:

  Connectivity/Source/ConnectivityManager.cpp:148: function 'ConnectivityManager::startHotspot' has 7 parameters (max: 6)
  Connectivity/Source/hotspot/HotspotManager.cpp:108: function 'HotspotManager::getConnectedClients' has nesting depth 5 (max: 4)
  Connectivity/Source/hotspot/HotspotManager.cpp:187: function 'HotspotManager::startUsageTimer' has nesting depth 5 (max: 4)
  Connectivity/Source/hotspot/HotspotManager.cpp:246: function 'HotspotManager::start' has 7 parameters (max: 6)
  Connectivity/Source/hotspot/HotspotManager.cpp:407: function 'HotspotManager::wifi_event_handler' has cyclomatic complexity 18 (max: 15)
  Connectivity/Source/hotspot/HotspotManager.cpp:407: function 'HotspotManager::wifi_event_handler' has nesting depth 5 (max: 4)
  Kernel/Source/TaskManager.cpp:155: function 'TaskManager::checkTasks' has nesting depth 5 (max: 4)
  Services/Source/ServiceRegistry.cpp:34: function 'ServiceRegistry::topologicalSort' has nesting depth 5 (max: 4)
  System/Source/services/DeviceProfileService.cpp:280: function 'DeviceProfileService::scanI2CBus' has nesting depth 5 (max: 4)
  System/Source/services/SdCardService.cpp:50: function 'SdCardService::mount' is 86 lines (max: 80)
  System/Source/services/SystemInfoService.cpp:109: function 'SystemInfoService::getSystemStats' is 96 lines (max: 80)
  System/Source/services/SystemInfoService.cpp:340: function 'SystemInfoService::getTaskList' has nesting depth 5 (max: 4)
  System/Source/services/SystemInfoService.cpp:340: function 'SystemInfoService::getTaskList' is 93 lines (max: 80)
  UI/Source/app/AppManager.cpp:131: function 'AppManager::startAppForResult' has cyclomatic complexity 19 (max: 15)
  UI/Source/app/AppManager.cpp:131: function 'AppManager::startAppForResult' is 145 lines (max: 80)
  UI/Source/components/FileBrowser.cpp:125: function 'FileBrowser::refreshList' has nesting depth 5 (max: 4)
  UI/Source/desktop/Desktop.cpp:41: function 'Desktop::init' has nesting depth 5 (max: 4)
  UI/Source/desktop/Desktop.cpp:41: function 'Desktop::init' is 147 lines (max: 80)
  UI/Source/desktop/modules/quick_access_panel/QuickAccessPanel.cpp:148: function 'QuickAccessPanel::buildToggles' has cyclomatic complexity 16 (max: 15)
  UI/Source/desktop/modules/quick_access_panel/QuickAccessPanel.cpp:148: function 'QuickAccessPanel::buildToggles' has nesting depth 6 (max: 4)
  UI/Source/desktop/modules/quick_access_panel/QuickAccessPanel.cpp:148: function 'QuickAccessPanel::buildToggles' is 136 lines (max: 80)
  UI/Source/desktop/modules/status_bar/StatusBar.cpp:47: function 'StatusBar::create' has cyclomatic complexity 18 (max: 15)
  UI/Source/desktop/modules/status_bar/StatusBar.cpp:47: function 'StatusBar::create' is 284 lines (max: 80)
  UI/Source/managers/FocusManager.cpp:170: function 'FocusManager::on_global_press' has cyclomatic complexity 17 (max: 15)
  UI/Source/managers/FocusManager.cpp:170: function 'FocusManager::on_global_press' has nesting depth 5 (max: 4)
  UI/Source/managers/FocusManager.cpp:57: function 'FocusManager::activateWindow' has cyclomatic complexity 17 (max: 15)
  UI/Source/tasks/GuiTask.cpp:79: function 'GuiTask::run' has nesting depth 5 (max: 4)
  UI/Source/tasks/GuiTask.cpp:79: function 'GuiTask::run' is 117 lines (max: 80)
  main/core/apps/settings/hotspot/HotspotSettings.cpp:290: function 'if' has nesting depth 5 (max: 4)
  main/core/apps/settings/hotspot/HotspotSettings.cpp:345: function 'HotspotSettings::createConfigPage' is 191 lines (max: 80)
  main/core/apps/settings/hotspot/HotspotSettings.cpp:92: function 'HotspotSettings::createMainPage' has cyclomatic complexity 25 (max: 15)
  main/core/apps/settings/hotspot/HotspotSettings.cpp:92: function 'HotspotSettings::createMainPage' has nesting depth 6 (max: 4)
  main/core/apps/settings/hotspot/HotspotSettings.cpp:92: function 'HotspotSettings::createMainPage' is 253 lines (max: 80)
  main/core/apps/settings/wifi/WiFiSettings.cpp:280: function 'WiFiSettings::refreshScan' has nesting depth 5 (max: 4)
  main/core/apps/settings/wifi/WiFiSettings.cpp:280: function 'WiFiSettings::refreshScan' is 86 lines (max: 80)
  main/core/apps/settings/wifi/WiFiSettings.cpp:39: function 'WiFiSettings::createUI' has nesting depth 6 (max: 4)
  main/core/apps/settings/wifi/WiFiSettings.cpp:39: function 'WiFiSettings::createUI' is 186 lines (max: 80)
  main/core/apps/settings/wifi/WiFiSettings.cpp:418: function 'WiFiSettings::showConnectScreen' is 93 lines (max: 80)
  main/core/apps/system_info/SystemInfoApp.cpp:126: function 'SystemInfoApp::createSystemTab' is 116 lines (max: 80)

=== Summary ===
Files analyzed: 57
Functions found: 866
Issues found: 39

=== Top 10 Most Complex Functions ===
```

[Full report](/home/akash/flxos-labs/flxos/reports/complexity.txt)

## Include Analysis

⚠️ **Issues Found**

```
=== FlxOS Include Dependency Analysis ===

Building dependency graph...
Analyzed 103 files

Checking for circular dependencies...
✅ No circular dependencies found!

Checking include order and header guards...

⚠️  Include Order Issues (493):
  System/Include/flx/system/SystemManager.hpp:6: system include <flx/core/Observable.hpp> after local include
  System/Include/flx/system/SystemManager.hpp:7: system include <flx/core/Singleton.hpp> after local include
  System/Include/flx/system/SystemManager.hpp:8: system include <memory> after local include
  System/Include/flx/system/services/ScreenshotService.hpp:4: system include <flx/core/Singleton.hpp> after local include
  System/Include/flx/system/services/ScreenshotService.hpp:5: system include <flx/services/IService.hpp> after local include
  System/Include/flx/system/services/ScreenshotService.hpp:6: system include <flx/services/ServiceManifest.hpp> after local include
  System/Include/flx/system/services/ScreenshotService.hpp:7: system include <functional> after local include
  System/Include/flx/system/services/ScreenshotService.hpp:8: system include <string> after local include
  System/Include/flx/system/services/SdCardService.hpp:4: system include <flx/services/IService.hpp> after local include
  System/Include/flx/system/services/SdCardService.hpp:5: system include <flx/services/ServiceManifest.hpp> after local include
  System/Include/flx/system/services/SdCardService.hpp:6: system include <string> after local include
  System/Include/flx/system/services/SystemInfoService.hpp:5: system include <cstdint> after local include
  System/Include/flx/system/services/SystemInfoService.hpp:6: system include <string> after local include
  System/Include/flx/system/services/SystemInfoService.hpp:7: system include <unordered_map> after local include
  System/Include/flx/system/services/SystemInfoService.hpp:8: system include <vector> after local include
  System/Include/flx/system/services/CliService.hpp:4: system include <flx/services/IService.hpp> after local include
  System/Include/flx/system/services/CliService.hpp:5: system include <flx/services/ServiceManifest.hpp> after local include
  System/Include/flx/system/services/CliService.hpp:6: system include <string> after local include
  System/Include/flx/system/device/DeviceProfiles.hpp:4: system include <string> after local include
  System/Include/flx/system/device/DeviceProfiles.hpp:5: system include <vector> after local include

=== Summary ===
Circular dependencies: 0
Include order issues: 493
Header guard issues: 0
Total issues: 493
```

[Full report](/home/akash/flxos-labs/flxos/reports/includes.txt)

## Naming Check

⚠️ **Issues Found**

```
=== FlxOS Naming Convention Check ===

⚠️  Naming Convention Issues (64):

Classes/Structs (48):
  System/Include/flx/system/SystemManager.hpp:15: class/struct 'flx' should be PascalCase
  System/Include/flx/system/services/DeviceProfileService.hpp:28: class/struct 'flx' should be PascalCase
  System/Include/flx/system/services/ScreenshotService.hpp:19: class/struct 'flx' should be PascalCase
  System/Include/flx/system/managers/NotificationManager.hpp:27: class/struct 'flx' should be PascalCase
  System/Include/flx/system/managers/TimeManager.hpp:15: class/struct 'flx' should be PascalCase
  System/Include/flx/system/managers/SettingsManager.hpp:17: class/struct 'flx' should be PascalCase
  System/Include/flx/system/managers/DisplayManager.hpp:12: class/struct 'flx' should be PascalCase
  System/Include/flx/system/managers/PowerManager.hpp:16: class/struct 'flx' should be PascalCase
  System/Include/flx/system/managers/ThemeManager.hpp:13: class/struct 'flx' should be PascalCase
  System/Source/services/CliService.cpp:559: class/struct 'tm' should be PascalCase

Functions (14):
  System/Source/managers/TimeManager.cpp:24: function 'time_sync_notification_cb' should be camelCase
  Core/Include/flx/core/Logger.hpp:47: function 'log_impl' should be camelCase
  UI/Include/flx/ui/LvglObserverBridge.hpp:47: function 'async_cb' should be camelCase
  UI/Include/flx/ui/LvglObserverBridge.hpp:105: function 'async_cb' should be camelCase
  UI/Include/flx/ui/theming/StyleUtils.hpp:10: function 'apply_glass' should be camelCase
  UI/Include/flx/ui/common/SettingsCommon.hpp:8: function 'add_list_btn' should be camelCase
  UI/Include/flx/ui/common/SettingsCommon.hpp:16: function 'create_page_container' should be camelCase
  UI/Include/flx/ui/common/SettingsCommon.hpp:26: function 'create_header' should be camelCase
  UI/Include/flx/ui/common/SettingsCommon.hpp:51: function 'create_settings_list' should be camelCase
  UI/Include/flx/ui/common/SettingsCommon.hpp:59: function 'add_back_button_event_cb' should be camelCase
  UI/Include/flx/ui/common/SettingsCommon.hpp:72: function 'add_switch_item' should be camelCase
  UI/Include/flx/ui/common/SettingsCommon.hpp:80: function 'add_slider_item' should be camelCase
  Connectivity/Source/hotspot/HotspotManager.cpp:36: function 'netif_input_hook' should be camelCase
  Connectivity/Source/hotspot/HotspotManager.cpp:48: function 'netif_linkoutput_hook' should be camelCase

Constants/Macros (1):
  main/hal/os/lv_os_custom.c:6: macro 'globals' should be UPPER_SNAKE_CASE

Filenames (1):
  main/hal/display/lv_lgfx_user.hpp: filename 'lv_lgfx_user' should be PascalCase

=== Summary ===
Files checked: 134
Issues found: 64
```

[Full report](/home/akash/flxos-labs/flxos/reports/naming.txt)

## Documentation Check

⚠️ **Issues Found**

```
=== FlxOS Documentation Coverage Check ===

⚠️  Files Without Headers (134):
  System/Include/flx/system/SystemManager.hpp
  System/Include/flx/system/services/DeviceProfileService.hpp
  System/Include/flx/system/services/ScreenshotService.hpp
  System/Include/flx/system/services/SdCardService.hpp
  System/Include/flx/system/services/SystemInfoService.hpp
  System/Include/flx/system/services/CliService.hpp
  System/Include/flx/system/services/FileSystemService.hpp
  System/Include/flx/system/device/DeviceProfile.hpp
  System/Include/flx/system/device/DeviceProfiles.hpp
  System/Include/flx/system/managers/NotificationManager.hpp

📝 TODO/FIXME Comments (6):
  TODO: 2, FIXME: 0, HACK/XXX: 4

  Recent items:
    [XXX] System/Source/services/DeviceProfileService.cpp:274: "}, // Hynitron capacitive touch
    [TODO] System/Source/services/CliService.cpp:459: Add confirmation?
    [XXX] main/hal/display/lv_lgfx_user.hpp:105: )
    [XXX] main/hal/display/lv_lgfx_user.hpp:123: )
    [XXX] main/hal/display/lv_lgfx_user.hpp:124: 
    [TODO] Kernel/Source/ResourceMonitorTask.cpp:37: Implement self-updating mechanism in PowerManager or use Eve

⚠️  Undocumented Public Functions (260):
  System/Include/flx/system/SystemManager.hpp:20: 'getUptimeObservable' lacks documentation
  System/Include/flx/system/SystemManager.hpp:21: 'isSafeMode' lacks documentation
  System/Include/flx/system/services/DeviceProfileService.hpp:35: 'getManifest' lacks documentation
  System/Include/flx/system/services/DeviceProfileService.hpp:37: 'onStart' lacks documentation
  System/Include/flx/system/services/DeviceProfileService.hpp:38: 'onStop' lacks documentation
  System/Include/flx/system/services/ScreenshotService.hpp:26: 'getManifest' lacks documentation
  System/Include/flx/system/services/ScreenshotService.hpp:28: 'onStart' lacks documentation
  System/Include/flx/system/services/ScreenshotService.hpp:29: 'onStop' lacks documentation
  System/Include/flx/system/services/SdCardService.hpp:21: 'getManifest' lacks documentation
  System/Include/flx/system/services/SdCardService.hpp:24: 'onStart' lacks documentation
  System/Include/flx/system/services/SdCardService.hpp:25: 'onStop' lacks documentation
  System/Include/flx/system/services/SdCardService.hpp:27: 'isMounted' lacks documentation
  System/Include/flx/system/services/SdCardService.hpp:28: 'getMountPoint' lacks documentation
  System/Include/flx/system/services/CliService.hpp:24: 'getManifest' lacks documentation
  System/Include/flx/system/services/CliService.hpp:27: 'onStart' lacks documentation

📁 Module README Status:
  ❌ .cache/README.md
  ❌ .devcontainer/README.md
  ❌ .git/README.md
  ❌ .github/README.md
  ❌ .vscode/README.md
  ❌ Buildscripts/README.md
  ❌ Connectivity/README.md
```

[Full report](/home/akash/flxos-labs/flxos/reports/docs.txt)

## Hardcoded Values

✅ **Passed** - No issues found

---

## Summary

| Check | Status |
|-------|--------|
| Checks with Issues | ⚠️ 4 |
