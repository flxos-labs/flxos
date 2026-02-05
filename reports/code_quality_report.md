# FlxOS Code Quality Report

Generated: Thu Feb  5 11:12:44 PM IST 2026

---

## Format Check

✅ **Passed** - No issues found

## Complexity Analysis

⚠️ **Issues Found**

```
=== FlxOS Code Complexity Analysis ===

⚠️  Complexity Issues Found:

  core/apps/settings/hotspot/HotspotSettings.cpp:279: function 'if' has nesting depth 5 (max: 4)
  core/apps/settings/hotspot/HotspotSettings.cpp:334: function 'HotspotSettings::createConfigPage' is 203 lines (max: 80)
  core/apps/settings/hotspot/HotspotSettings.cpp:81: function 'HotspotSettings::createMainPage' has cyclomatic complexity 25 (max: 15)
  core/apps/settings/hotspot/HotspotSettings.cpp:81: function 'HotspotSettings::createMainPage' has nesting depth 6 (max: 4)
  core/apps/settings/hotspot/HotspotSettings.cpp:81: function 'HotspotSettings::createMainPage' is 253 lines (max: 80)
  core/apps/settings/wifi/WiFiSettings.cpp:172: function 'WiFiSettings::refreshScan' has nesting depth 5 (max: 4)
  core/apps/settings/wifi/WiFiSettings.cpp:172: function 'WiFiSettings::refreshScan' is 86 lines (max: 80)
  core/apps/settings/wifi/WiFiSettings.cpp:38: function 'WiFiSettings::show' is 102 lines (max: 80)
  core/connectivity/ConnectivityManager.cpp:142: function 'ConnectivityManager::startHotspot' has 7 parameters (max: 6)
  core/connectivity/hotspot/HotspotManager.cpp:113: function 'HotspotManager::getConnectedClients' has nesting depth 5 (max: 4)
  core/connectivity/hotspot/HotspotManager.cpp:192: function 'HotspotManager::startUsageTimer' has nesting depth 5 (max: 4)
  core/connectivity/hotspot/HotspotManager.cpp:251: function 'HotspotManager::start' has 7 parameters (max: 6)
  core/connectivity/hotspot/HotspotManager.cpp:412: function 'HotspotManager::wifi_event_handler' has cyclomatic complexity 18 (max: 15)
  core/connectivity/hotspot/HotspotManager.cpp:412: function 'HotspotManager::wifi_event_handler' has nesting depth 5 (max: 4)
  core/services/system_info/SystemInfoService.cpp:109: function 'SystemInfoService::getSystemStats' is 96 lines (max: 80)
  core/services/system_info/SystemInfoService.cpp:337: function 'SystemInfoService::getTaskList' has nesting depth 5 (max: 4)
  core/services/system_info/SystemInfoService.cpp:337: function 'SystemInfoService::getTaskList' is 93 lines (max: 80)
  core/system/focus/FocusManager.cpp:175: function 'FocusManager::on_global_press' has cyclomatic complexity 17 (max: 15)
  core/system/focus/FocusManager.cpp:175: function 'FocusManager::on_global_press' has nesting depth 5 (max: 4)
  core/system/focus/FocusManager.cpp:62: function 'FocusManager::activateWindow' has cyclomatic complexity 17 (max: 15)
  core/tasks/TaskManager.cpp:155: function 'TaskManager::checkTasks' has nesting depth 5 (max: 4)
  core/tasks/gui/GuiTask.cpp:87: function 'GuiTask::run' has nesting depth 5 (max: 4)
  core/ui/desktop/Desktop.cpp:47: function 'Desktop::init' has cyclomatic complexity 17 (max: 15)
  core/ui/desktop/Desktop.cpp:47: function 'Desktop::init' has nesting depth 6 (max: 4)
  core/ui/desktop/Desktop.cpp:47: function 'Desktop::init' is 150 lines (max: 80)
  core/ui/desktop/modules/quick_access_panel/QuickAccessPanel.cpp:34: function 'QuickAccessPanel::create' is 102 lines (max: 80)
  core/ui/desktop/modules/status_bar/StatusBar.cpp:40: function 'StatusBar::create' has cyclomatic complexity 18 (max: 15)
  core/ui/desktop/modules/status_bar/StatusBar.cpp:40: function 'StatusBar::create' is 253 lines (max: 80)

=== Summary ===
Files analyzed: 36
Functions found: 535
Issues found: 28

=== Top 10 Most Complex Functions ===
  core/apps/settings/hotspot/HotspotSettings.cpp:81 - HotspotSettings::createMainPage: complexity=25, lines=253
  core/connectivity/hotspot/HotspotManager.cpp:412 - HotspotManager::wifi_event_handler: complexity=18, lines=78
  core/ui/desktop/modules/status_bar/StatusBar.cpp:40 - StatusBar::create: complexity=18, lines=253
  core/system/focus/FocusManager.cpp:62 - FocusManager::activateWindow: complexity=17, lines=57
  core/system/focus/FocusManager.cpp:175 - FocusManager::on_global_press: complexity=17, lines=55
  core/ui/desktop/Desktop.cpp:47 - Desktop::init: complexity=17, lines=150
  core/connectivity/hotspot/HotspotManager.cpp:251 - HotspotManager::start: complexity=15, lines=68
  core/connectivity/wifi/WiFiManager.cpp:224 - WiFiManager::handleStaDisconnected: complexity=15, lines=41
  core/services/system_info/SystemInfoService.cpp:337 - SystemInfoService::getTaskList: complexity=15, lines=93
  core/apps/settings/hotspot/HotspotSettings.cpp:279 - if: complexity=15, lines=54
```

[Full report](/home/akash/flxos-labs/flxos/reports/complexity.txt)

## Include Analysis

⚠️ **Issues Found**

```
=== FlxOS Include Dependency Analysis ===

Building dependency graph...
Analyzed 77 files

Checking for circular dependencies...
✅ No circular dependencies found!

Checking include order and header guards...

⚠️  Include Order Issues (160):
  main.cpp:16: system include <string_view> after local include
  hal/os/lv_os_custom.c:4: system include <string.h> after local include
  core/connectivity/ConnectivityManager.hpp:8: system include <memory> after local include
  core/connectivity/ConnectivityManager.hpp:9: system include <mutex> after local include
  core/connectivity/ConnectivityManager.hpp:10: system include <string> after local include
  core/connectivity/ConnectivityManager.hpp:11: system include <vector> after local include
  core/connectivity/ConnectivityManager.cpp:13: system include <cstdint> after local include
  core/connectivity/ConnectivityManager.cpp:14: system include <string_view> after local include
  core/connectivity/hotspot/HotspotManager.cpp:14: system include <cstdint> after local include
  core/connectivity/hotspot/HotspotManager.cpp:15: system include <cstring> after local include
  core/connectivity/hotspot/HotspotManager.cpp:16: system include <string_view> after local include
  core/connectivity/hotspot/HotspotManager.hpp:10: system include <mutex> after local include
  core/connectivity/hotspot/HotspotManager.hpp:11: system include <string> after local include
  core/connectivity/hotspot/HotspotManager.hpp:12: system include <vector> after local include
  core/connectivity/wifi/WiFiManager.hpp:7: system include <functional> after local include
  core/connectivity/wifi/WiFiManager.hpp:8: system include <vector> after local include
  core/connectivity/wifi/WiFiManager.cpp:13: system include <cstdint> after local include
  core/connectivity/wifi/WiFiManager.cpp:14: system include <cstring> after local include
  core/connectivity/wifi/WiFiManager.cpp:15: system include <string_view> after local include
  core/connectivity/bluetooth/BluetoothManager.cpp:5: system include <cstdint> after local include

=== Summary ===
Circular dependencies: 0
Include order issues: 160
Header guard issues: 0
Total issues: 160
```

[Full report](/home/akash/flxos-labs/flxos/reports/includes.txt)

## Naming Check

⚠️ **Issues Found**

```
=== FlxOS Naming Convention Check ===

⚠️  Naming Convention Issues (37):

Classes/Structs (22):
  core/connectivity/hotspot/HotspotManager.cpp:28: class/struct 'netif' should be PascalCase
  core/connectivity/hotspot/HotspotManager.cpp:36: class/struct 'pbuf' should be PascalCase
  core/connectivity/hotspot/HotspotManager.cpp:36: class/struct 'netif' should be PascalCase
  core/connectivity/hotspot/HotspotManager.cpp:38: class/struct 'netif' should be PascalCase
  core/connectivity/hotspot/HotspotManager.cpp:49: class/struct 'netif' should be PascalCase
  core/connectivity/hotspot/HotspotManager.cpp:49: class/struct 'pbuf' should be PascalCase
  core/connectivity/hotspot/HotspotManager.cpp:51: class/struct 'netif' should be PascalCase
  core/connectivity/hotspot/HotspotManager.cpp:82: class/struct 'netif' should be PascalCase
  core/system/time/TimeManager.cpp:18: class/struct 'timeval' should be PascalCase
  core/system/time/TimeManager.cpp:22: class/struct 'tm' should be PascalCase

Functions (13):
  core/connectivity/hotspot/HotspotManager.cpp:36: function 'netif_input_hook' should be camelCase
  core/connectivity/hotspot/HotspotManager.cpp:48: function 'netif_linkoutput_hook' should be camelCase
  core/system/time/TimeManager.cpp:17: function 'time_sync_notification_cb' should be camelCase
  core/common/Logger.hpp:45: function 'log_impl' should be camelCase
  core/apps/settings/SettingsCommon.hpp:8: function 'add_list_btn' should be camelCase
  core/apps/settings/SettingsCommon.hpp:16: function 'create_page_container' should be camelCase
  core/apps/settings/SettingsCommon.hpp:25: function 'create_header' should be camelCase
  core/apps/settings/SettingsCommon.hpp:50: function 'create_settings_list' should be camelCase
  core/apps/settings/SettingsCommon.hpp:58: function 'add_back_button_event_cb' should be camelCase
  core/apps/settings/SettingsCommon.hpp:71: function 'add_switch_item' should be camelCase
  core/ui/LvglObserverBridge.hpp:47: function 'async_cb' should be camelCase
  core/ui/LvglObserverBridge.hpp:105: function 'async_cb' should be camelCase
  core/ui/theming/StyleUtils.hpp:8: function 'apply_glass' should be camelCase

Constants/Macros (1):
  hal/os/lv_os_custom.c:6: macro 'globals' should be UPPER_SNAKE_CASE

Filenames (1):
  hal/display/lv_lgfx_user.hpp: filename 'lv_lgfx_user' should be PascalCase

=== Summary ===
Files checked: 84
Issues found: 37
```

[Full report](/home/akash/flxos-labs/flxos/reports/naming.txt)

## Documentation Check

⚠️ **Issues Found**

```
=== FlxOS Documentation Coverage Check ===

⚠️  Files Without Headers (84):
  main.cpp
  hal/os/lv_os_custom.c
  hal/display/lv_lgfx_user.hpp
  core/connectivity/ConnectivityManager.hpp
  core/connectivity/ConnectivityManager.cpp
  core/connectivity/hotspot/HotspotManager.cpp
  core/connectivity/hotspot/HotspotManager.hpp
  core/connectivity/wifi/WiFiManager.hpp
  core/connectivity/wifi/WiFiManager.cpp
  core/connectivity/bluetooth/BluetoothManager.hpp

📝 TODO/FIXME Comments (4):
  TODO: 1, FIXME: 0, HACK/XXX: 3

  Recent items:
    [XXX] hal/display/lv_lgfx_user.hpp:105: )
    [XXX] hal/display/lv_lgfx_user.hpp:123: )
    [XXX] hal/display/lv_lgfx_user.hpp:124: 
    [TODO] core/services/cli/CliService.cpp:446: Add confirmation?

⚠️  Undocumented Public Functions (161):
  core/connectivity/ConnectivityManager.hpp:32: 'getWifiMutex' lacks documentation
  core/connectivity/ConnectivityManager.hpp:49: 'getHotspotClientsList' lacks documentation
  core/connectivity/ConnectivityManager.hpp:63: 'getWiFiEnabledObservable' lacks documentation
  core/connectivity/ConnectivityManager.hpp:64: 'getWiFiStatusObservable' lacks documentation
  core/connectivity/ConnectivityManager.hpp:65: 'getWiFiConnectedObservable' lacks documentation
  core/connectivity/ConnectivityManager.hpp:66: 'getWiFiSsidObservable' lacks documentation
  core/connectivity/ConnectivityManager.hpp:67: 'getWiFiIpObservable' lacks documentation
  core/connectivity/ConnectivityManager.hpp:68: 'getHotspotEnabledObservable' lacks documentation
  core/connectivity/ConnectivityManager.hpp:69: 'getHotspotClientsObservable' lacks documentation
  core/connectivity/ConnectivityManager.hpp:70: 'getHotspotUsageSentSubject' lacks documentation
  core/connectivity/ConnectivityManager.hpp:71: 'getHotspotUsageReceivedSubject' lacks documentation
  core/connectivity/ConnectivityManager.hpp:72: 'getHotspotUploadSpeedSubject' lacks documentation
  core/connectivity/ConnectivityManager.hpp:73: 'getHotspotDownloadSpeedSubject' lacks documentation
  core/connectivity/ConnectivityManager.hpp:74: 'getHotspotUptimeSubject' lacks documentation
  core/connectivity/ConnectivityManager.hpp:75: 'getBluetoothEnabledObservable' lacks documentation

📁 Module README Status:
  ✅ core/README.md
  ❌ hal/README.md

=== Summary ===
Files checked: 84
Files without headers: 84
TODO/FIXME comments: 4
Undocumented functions: 161
Missing READMEs: 1
```

[Full report](/home/akash/flxos-labs/flxos/reports/docs.txt)

## Hardcoded Values

✅ **Passed** - No issues found

---

## Summary

| Check | Status |
|-------|--------|
| Checks with Issues | ⚠️ 4 |
